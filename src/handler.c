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
    log("handle request");
    /* Parse request */
    if(parse_request(r)<0){
        debug("Unable to parse request: %s\n",strerror(errno));
        return handle_error(r,HTTP_STATUS_BAD_REQUEST);
    }

    r->path = determine_request_path(r->uri);
    if(r->path<0){
        debug("Could not determine request: %s\n",strerror(errno));
        return HTTP_STATUS_BAD_REQUEST;
    }

    // Determine request path
    debug("HTTP REQUEST PATH: %s", r->path);

    // Dispatch to appropriate request handler type based on file type
    struct stat s;
    debug("r->path before entering conditional: %s\n",r->path);
    if((r->path)==NULL){
        debug("It passed the if");
        if(!stat(RootPath,&s)){
            result = handle_error(r,HTTP_STATUS_NOT_FOUND);
        }
    }
    else{
        debug("It pass the else");
        if(!stat(r->path, &s)){
            result = HTTP_STATUS_BAD_REQUEST;
        }
    }
    if (S_ISDIR(s.st_mode)){
        debug("Entered browse request");
        result = handle_browse_request(r);
        if (result)
            result = handle_error(r,result);
    }
    else if ((access(r->path,X_OK) == 0) && S_ISREG(s.st_mode)){
        debug("Entered cgi request");
        result = handle_cgi_request(r);
        if (result)
            result = handle_error(r,result);
    }
    else if (S_ISREG(s.st_mode)){
        debug("Entered file request");
        result = handle_file_request(r);
        if (result)
            result = handle_error(r,result);
    }
    else{
        result = HTTP_STATUS_BAD_REQUEST;
    }
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
    if(r->uri==NULL){
        return HTTP_STATUS_BAD_REQUEST;
    }
    debug("Entering browse request");
    if(r->path){
        n = scandir(r->path, &entries, 0, alphasort);
    }
    else{
        n = scandir(RootPath, &entries, 0, alphasort);
    }
    if (n < 0){
        debug("Unable to scan directory: %s\n", strerror(errno));
        return HTTP_STATUS_NOT_FOUND;
    }

    /* Write HTTP Header with OK Status and text/html Content-Type */
    fprintf(r->stream, "HTTP/1.0 200 OK\r\n");
    fprintf(r->stream, "Content-Type: text/html\r\n");
    fprintf(r->stream, "\r\n");

    /* For each entry in directory, emit HTML list item */
    fprintf(r->stream, "<ul>\n");
    for (int i = 0; i < n; i++){
        if (strcmp(entries[i]->d_name,".")==0){
            free(entries[i]);
            continue;
        }
        debug("URI during browse request: %s",r->uri);
        if (strcmp(r->uri,"")==0){
            fprintf(r->stream, "<li><a href=\"%s/%s\">%s</a></li>\n", "", entries[i]->d_name, entries[i]->d_name);
        }
        else{
            fprintf(r->stream, "<li><a href=\"/%s/%s\">%s</a></li>\n", r->uri, entries[i]->d_name, entries[i]->d_name);
        }
        free(entries[i]);
    }
    free(entries);
    fprintf(r->stream, "<ul>\n");

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
    fs = fopen(r->path,"r");
    if (!fs){
        goto fail;
    }

    log("Get to mimetype");
    /* Determine mimetype */
    mimetype = determine_mimetype(r->path);
    debug("This is the mimetype: %s\n", mimetype);

    /* Write HTTP Headers with OK status and determined Content-Type */
    fprintf(r->stream, "HTTP/1.0 200 OK\r\n");
    fprintf(r->stream, "Content-Type: %s\r\n",mimetype);
    fprintf(r->stream, "\r\n");
    /* Read from file and write to socket in chunks */
    nread = fread(buffer,1,BUFSIZ,fs);
    while (nread > 0){
        fwrite(buffer,1,nread,r->stream);
        nread = fread(buffer,1,BUFSIZ,fs);
    }
    /* Close file, deallocate mimetype, return OK */
    fclose(fs);
    if(mimetype!=DefaultMimeType){
        free(mimetype);
    }
    return HTTP_STATUS_OK;

fail:
    /* Close file, free mimetype, return INTERNAL_SERVER_ERROR */
    fclose(fs);
    free(mimetype);
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
    debug("entered handle cgi request!!");
    Header *header = r->headers;

    /* Export CGI environment variables from request:
     * http://en.wikipedia.org/wiki/Common_Gateway_Interface */

    /* Export CGI environment variables from request headers */
    if(setenv("DOCUMENT_ROOT", RootPath,1))
        fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    if(r->query){
        if(setenv("QUERY_STRING", r->query, 1))
            fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    }
    else
        setenv("QUERY_STRING","",1);
    if(setenv("REMOTE_ADDR", r->host,1))
        fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    if(setenv("REMOTE_PORT", r->port, 1))
        fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    if(setenv("REQUEST_METHOD", r->method,1))
        fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    if(setenv("REQUEST_URI", r->uri, 1))
        fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    if(setenv("SCRIPT_FILENAME", r->path,1))
        fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    if(setenv("SERVER_PORT", Port, 1))
        fprintf(stderr,"Can't setenv %s\n",strerror(errno));
    while (header!=NULL){
        if (streq(header->name,"Accept"))
            setenv("HTTP_ACCEPT", header->data,1);
        if (streq(header->name,"Accept-Encoding"))
            setenv("HTTP_ACCEPT_ENCODING", header->data, 1);
        if (streq(header->name,"Accept-Language"))
            setenv("HTTP_ACCEPT_LANGUAGE", header->data,1);
        if (streq(header->name,"Connection"))
            setenv("HTTP_CONNECTION", header->data, 1);
        if (streq(header->name,"Host"))
            setenv("HTTP_HOST", header->data,1);
        if (streq(header->name,"User-Agent"))
            setenv("HTTP_USER_AGENT", header->data, 1);
        header = header->next;
    }
    /* POpen CGI Script */
    if(r->path)
        pfs = popen(r->path, "r");
    else
        pfs = popen(RootPath, "r");
    if(!pfs){
        pclose(pfs);
        return HTTP_STATUS_BAD_REQUEST;
    }
    debug("opened the pfs");
    /* Copy data from popen to socket */
    size_t nread = fread(buffer, 1, BUFSIZ, pfs);
    while(nread>0){
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
    fprintf(r->stream, "HTTP/1.0 %s\r\n",status_string);
    fprintf(r->stream, "Content-Type: text/html\r\n");
    fprintf(r->stream, "\r\n");
    /* Write HTML Description of Error*/
    fprintf(r->stream,"<html><body>");
    fprintf(r->stream, "<h1>Error: %s</h1>\n",status_string);
    fprintf(r->stream,"<html><body>");

    /* Return specified status */
    fflush(r->stream);
    return status;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
