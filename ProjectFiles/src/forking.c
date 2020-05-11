/* forking.c: Forking HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <signal.h>
#include <string.h>

#include <unistd.h>

/**
 * Fork incoming HTTP requests to handle the concurrently.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Exit status of server (EXIT_SUCCESS).
 *
 * The parent should accept a request and then fork off and let the child
 * handle the request.
 **/
int forking_server(int sfd) {
    /* Accept and handle HTTP request */
    while (true) {
    	/* Accept request */
        Request * r = accept_request(sfd);
        if(!r)
        {
            free_request(r);
            return EXIT_FAILURE;
        }

        pid_t pid = fork();
	    /* Ignore children */
        if(pid == 0)
        {
            debug("Handling Child");
            handle_request(r);
            free_request(r);
            exit(EXIT_SUCCESS);

        }
	     /* Fork off child process to handle request */
        else
        {
            free_request(r);
        }
    }

    /* Close server socket */
    close(sfd);
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
