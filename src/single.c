/* single.c: Single User HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

/**
 * Handle one HTTP request at a time.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Exit status of server (EXIT_SUCCESS).
 **/
int single_server(int sfd) {
    /* Accept and handle HTTP request */
    while (true) {
    	/* Accept request */
      Request *request = accept_request(sfd);
      if(! request){
        log("unable to accept request: %s", strerror(errno));
        continue;
      }


	/* Handle request */
      log("before handle request");
      handle_request(request);
   //       fprintf(stderr,"Unable to handle request %s\n", strerror(errno));
      log("after handle request");
	/* Free request */
      free_request(request);
      log("after free request");
    }

    /* Close server socket */
    close(sfd);
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
