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
    Status result;
    while (true) {
    	/* Accept request */
        Request *request = accept_request(sfd);
        if(!request){
          log("unable to accept request: %s", strerror(errno));
          continue;
        }
	/* Ignore children */
        signal(SIGCHLD,SIG_IGN);
        pid_t pid = fork();
	/* Fork off child process to handle request */
        if (pid == 0){
            debug("starting child process");
            result = handle_request(request);
            free_request(request);
            exit(result);
        }

        else{
            free_request(request);
        }
    }

    /* Close server socket */
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
