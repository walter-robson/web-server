#!/usr/bin/env python3

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
    tottime = 0

    for throw in range(throws):
        timein = time.time()
        response = requests.get(url)
        timeout = time.time()
        tottime = tottime + (timeout - timein)
        if verbose:
            print(f"{response.text}")
        print(f"Hammer: {hid}, Throw: {throw}, Elapsed Time: {timeout - timein}" )

    avtime = tottime/throws
    print(f"Hammer: {hid}, AVERAGE , Elapsed Time: {avtime}")


    return avtime

def do_hammer(args):
    ''' Use args tuple to call `hammer` '''
    return hammer(*args)

def main():
    arguments = sys.argv[1:]
    hammers = 1
    throws  = 1
    verbose = False
    flags = [ '-h' , '-t' , '-v']
    URL = "http://example.com"
    # Parse command line arguments
    numArg = int(len(sys.argv))
    if numArg == 1:
        usage(1)

    for x in range(len(arguments)):
        if arguments[x] == '-h':
            hammers = int(arguments[x+1])
        elif arguments[x] == '-t':
            throws = int(arguments[x+1])
        elif arguments[x] == '-v':
            verbose = True
        elif arguments[x].startswith('-'):
            usage(1)

    URL = arguments.pop()
# Create pool of workers and perform throws


    args = ((URL, throws, verbose, hid) for hid in range(hammers))
    #hid = set(map(do_hammer(), args))
    total = 0
    size = 0
    with concurrent.futures.ProcessPoolExecutor(max_workers = hammers) as executor:
        avtime = executor.map(do_hammer, args)
        for i in avtime:
            total = total + i
            size = size + 1
    average = total / size

    print(f"TOTAL AVERAGE ELAPSED TIME: {average}")

    return 0

# Main execution

if __name__ == '__main__':
    main()

# vim: set sts=4 sw=4 ts=8 expandtab ft=python:
