/* handler.c: HTTP Request Handlers */

#include "spidey.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Internal Declarations */
Status handle_browse_request(Request *request);
Status handle_file_request(Request *request);
Status handle_cgi_request(Request *request);
Status handle_error(Request *request, Status status);

/**
 * Handle HTTP Request.
 *
 * @param   r           HTTP Request structure
 * @return  Status of the HTTP request.
 *
 * This parses a request, determines the request path, determines the request
 * type, and then dispatches to the appropriate handler type.
 *
 * On error, handle_error should be used with an appropriate HTTP status code.
 **/
Status  handle_request(Request *r) {
    Status result;

    /* Parse request */
    if(parse_request(r) == -1)
    {
        debug("Parse request failed: %s", strerror(errno));
        result = handle_error(r, HTTP_STATUS_BAD_REQUEST);
        return result;
    }

    /* Determine request path */
    r->path = determine_request_path(r->uri);

    // Open stat on request path
    struct stat s;
    if(stat(r->path, &s) < 0)
    {
        result = handle_error(r, HTTP_STATUS_NOT_FOUND);
        debug("Could not stat path: %s", r->path);
        return result;
    }



    // call appropriate handler
    if(S_ISDIR(s.st_mode))
    {
        result = handle_browse_request(r);
        debug("Handling Browser");
    }
    else if(access(r->path, X_OK) == 0)
    {
        result = handle_cgi_request(r);
        debug("Handling CGI");
    }
    else if(access(r->path, R_OK) == 0)
    {
        result = handle_file_request(r);
        debug("Handling File");
    }
    else
    {
        result = handle_error(r, HTTP_STATUS_BAD_REQUEST);
    }

    /* Dispatch to appropriate request handler type based on file type */
    log("HTTP REQUEST STATUS: %s", http_status_string(result));

    return result;
}

/**
 * Handle browse request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP browse request.
 *
 * This lists the contents of a directory in HTML.
 *
 * If the path cannot be opened or scanned as a directory, then handle error
 * with HTTP_STATUS_NOT_FOUND.
 **/
Status  handle_browse_request(Request *r) {
    struct dirent **entries;
    int n;

    /* Open a directory for reading or scanning */
    n = scandir(r->path, &entries, 0, alphasort);
    if(n < 0)
    {
        debug("scandir failed: %s", strerror(errno));
        free(entries);
        return handle_error(r, HTTP_STATUS_NOT_FOUND);
    }


    /* Write HTTP Header with OK Status and text/html Content-Type */
    fprintf(r->stream, "HTTP/1.0 200 OK\r\n");
    fprintf(r->stream, "Content-Type: text/html\r\n");
    fprintf(r->stream, "\r\n");

    /* For each entry in directory, emit HTML list item */
    fprintf(r->stream, "<ul>\r\n");
    for(int i = 0; i < n; i++)
    {
        if(strcmp(entries[i]->d_name, ".") == 0)
        {
            free(entries[i]);
            continue;
        }

        if(strcmp(r->uri,"/") == 0){
          //debug("href: %s%s", r->uri,entries[i]->d_name);
          fprintf(r->stream, "<li><a href=\"%s%s\">%s</a></li>\n", r->uri,entries[i]->d_name,entries[i]->d_name);
        }
        else{
          //debug("href: %s/%s", r->uri,entries[i]->d_name);
          fprintf(r->stream, "<li><a href=\"%s/%s\">%s</a></li>\n", r->uri,entries[i]->d_name,entries[i]->d_name);
        }


        free(entries[i]);
    }
    fprintf(r->stream, "</ul>\r\n");
    free(entries);
    /* Return OK */
    return HTTP_STATUS_OK;
}

/**
 * Handle file request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This opens and streams the contents of the specified file to the socket.
 *
 * If the path cannot be opened for reading, then handle error with
 * HTTP_STATUS_NOT_FOUND.
 **/
Status  handle_file_request(Request *r) {
    FILE *fs;
    char buffer[BUFSIZ];
    char *mimetype = NULL;
    size_t nread;

    /* Open file for reading */
    fs = fopen(r->path, "r");
    if(!fs)
    {
        debug("Failed to open file from path: %s", strerror(errno));
        fclose(fs);
        goto fail;
    }

    /* Determine mimetype */
    mimetype = determine_mimetype(r->path);

    /* Write HTTP Headers with OK status and determined Content-Type */
    fprintf(r->stream, "HTTP/1.0 200 OK\r\n");
    fprintf(r->stream, "Content-Type: %s\r\n", mimetype);
    fprintf(r->stream, "\r\n");

    /* Read from file and write to socket in chunks */
    nread = fread(buffer, 1, BUFSIZ, fs);
    while(nread > 0)
    {
        fwrite(buffer, 1, nread, r->stream);
        nread = fread(buffer, 1, BUFSIZ, fs);
    }

    /* Close file, deallocate mimetype, return OK */
    fclose(fs);
    free(mimetype);
    return HTTP_STATUS_OK;

fail:
    /* Close file, free mimetype, return INTERNAL_SERVER_ERROR */
    free(mimetype);
    fclose(fs);
    return HTTP_STATUS_INTERNAL_SERVER_ERROR;
}

/**
 * Handle CGI request
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This popens and streams the results of the specified executables to the
 * socket.
 *
 * If the path cannot be popened, then handle error with
 * HTTP_STATUS_INTERNAL_SERVER_ERROR.
 **/
Status  handle_cgi_request(Request *r) {
    FILE *pfs;
    char buffer[BUFSIZ];

    /* Export CGI environment variables from request:
     * http://en.wikipedia.org/wiki/Common_Gateway_Interface */
    setenv("REQUEST_METHOD", r->method, 1);
    setenv("REQUEST_URI",r->uri, 1);
    setenv("SCRIPT_FILENAME",r->path, 1);
    setenv("QUERY_STRING",r->query, 1);
    setenv("REMOTE_ADDR",r->host, 1);
    setenv("REMOTE_PORT",r->port, 1);

    setenv("DOCUMENT_ROOT", RootPath, 1);

    /* Export CGI environment variables from request headers */
    Header  *h = r->headers;

    while(h){
        // Set all environment variables
        if(h->name && strcmp(h->name,"Host") == 0){
          char *copyData = strdup(h->data);
          if(!copyData){
            return handle_error(r,HTTP_STATUS_INTERNAL_SERVER_ERROR);
          }
          char *portNum = strchr(copyData, ':');
          *portNum = '\0';
          portNum++;
          char *hostName = copyData;
          setenv("HTTP_HOST",hostName, 1);
          setenv("SERVER_PORT",portNum,1);
          debug("HTTP_HOST: %s", hostName);
          debug("SERVER_PORT: %s", hostName);
          free(copyData);

        }
        else if(h->name && strcmp(h->name,"Connection") == 0){
          setenv("HTTP_CONNECTION", h->data,1);
        }
        else if(h->name && strcmp(h->name,"Acccept") == 0){
          setenv("HTTP_ACCEPT", h->data,1);
        }
        else if(h->name && strcmp(h->name,"Accept-Language") == 0){
          setenv("HTTP_ACCEPT_LANGUAGE", h->data,1);
        }
        else if(h->name && strcmp(h->name,"Accept-Encoding") == 0){
          setenv("HTTP_ACCEPT_ENCODING", h->data,1);
        }
        else if(h->name && strcmp(h->name,"User-Agent") == 0){
          setenv("HTTP_USER_AGENT", h->data,1);
        }

        h = h->next;
    }


    /* POpen CGI Script */
    pfs = popen(r->path, "r");
    if(!pfs){
      return handle_error(r,HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    /* Copy data from popen to socket */
    size_t nread = fread(buffer, 1, BUFSIZ, pfs);
    while(nread > 0){
      fwrite(buffer, 1, nread, r->stream);
      nread = fread(buffer, 1, BUFSIZ, pfs);
    }

    /* Close popen, return OK */
    pclose(pfs);

    return HTTP_STATUS_OK;
}

/**
 * Handle displaying error page
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP error request.
 *
 * This writes an HTTP status error code and then generates an HTML message to
 * notify the user of the error.
 **/
Status  handle_error(Request *r, Status status) {
    const char *status_string = http_status_string(status);


    /* Write HTTP Header */
    fprintf(r->stream, "HTTP/1.0 %s\r\n", status_string);
    fprintf(r->stream, "Content-Type: text/html\r\n");
    fprintf(r->stream, "\r\n");

    /* Write HTML Description of Error*/
    fprintf(r->stream, "<html>\n<h1>%s</h1>\n", status_string);
    fprintf(r->stream, "<h2>You played yourself</h2>\r\n</html>\r\n");
    fprintf(r->stream, "<img src='https://i.kym-cdn.com/entries/icons/facebook/000/019/954/khaled.jpg' style='width:400px;height:400px;'>");

    /* Return specified status */
    return status;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
