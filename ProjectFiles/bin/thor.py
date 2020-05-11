#!/usr/bin/env python3

# Title: thor.py
# Author: Christopher Reynders, Reno Domel
# Description: Making get requests to servers

import concurrent.futures
import os
import requests
import sys
import time

# Functions

def usage(status=0):
    progname = os.path.basename(sys.argv[0])
    print(f'''Usage: {progname} [-h HAMMERS -t THROWS] URL
    -h  HAMMERS     Number of hammers to utilize (1)
    -t  THROWS      Number of throws per hammer  (1)
    -v              Display verbose output
    ''')
    sys.exit(status)


def hammer(url, throws, verbose, hid):
    ''' Hammer specified url by making multiple throws (ie. HTTP requests).

    - url:      URL to request
    - throws:   How many times to make the request
    - verbose:  Whether or not to display the text of the response
    - hid:      Unique hammer identifier

    Return the average elapsed time of all the throws.
    '''

    # make get request
    try:

        # perform throws number of request
        ham_time = 0
        for i in range(throws):
            req_start_time = time.time()
            response = requests.get(url)
            req_time = time.time() - req_start_time
            ham_time += req_time

            # print response
            if(verbose):
                print(response.text)

            # print information about throw
            print(f'Hammer: {hid:>2}, Throw:{i:>4}, Elapsed Time: {req_time:>2.2f}')

        # average per hammer
        avg_per_ham = ham_time/throws
        print(f'Hammer: {hid:>2}, AVERAGE   , Elapsed Time: {avg_per_ham:>2.2f}')

    except requests.ConnectionError as c:
        print(c)
        usage(1)

    # return average per hammer
    return avg_per_ham

def do_hammer(args):
    ''' Use args tuple to call `hammer` '''
    return hammer(*args)

def main():
    # Parse command line arguments
    arguments = sys.argv[1:]
    hammers = 1;
    throws = 1;
    verbose = False


    # loop through flags
    while len(arguments) and arguments[0].startswith('-'):
        argument = arguments.pop(0)
        if argument == '-h':
            hammers = int(arguments.pop(0))
        elif argument == '-t':
            throws = int(arguments.pop(0))
        elif argument == '-v':
            verbose = True;
        elif argument == '-p':
            usage(0)
        else:
            usage(1)

    # get url and check its validity
    if len(arguments):
        url = arguments.pop(0)
        if url.startswith("http"):
            pass
        else:
            url = "https://"+url
    else:
        usage(1)


    # Create pool of workers and perform throws
    total_time = 0
    if(hammers > 1):

        # make arguments for do_hammer
        ham_args = ((url, throws, verbose, hid) for hid in range(hammers))

        # perform hammers with multiple processes
        try:
            with concurrent.futures.ProcessPoolExecutor(hammers) as executor:
                time_generator = executor.map(do_hammer, ham_args)

                # calculate total time
                for time in time_generator:
                    total_time += time

        except ValueError as e:
            print(e)
            exit(1)


    # perform one hammer
    else:
        total_time = hammer(url, throws, verbose, 0)

    # print total avg time
    total_avg = total_time/hammers
    print(f'TOTAL AVERAGE ELAPSED TIME: {total_avg:>2.2f}')

# Main execution

if __name__ == '__main__':
    main()

# vim: set sts=4 sw=4 ts=8 expandtab ft=python:
