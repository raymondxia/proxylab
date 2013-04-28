/*
 * A Web proxy that acts as intermediate between Web browser and the Web.
 *
 * Note: handles only GET requests
 *
 * Authors: Alex Lucena & Saumya Dalal
 *
 */
#include <stdio.h>
#include "csapp.h"
 #include <cache.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Debug output macro taken from malloc lab */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

static const char *user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_ = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";



void serve(int file_d);
void read_headers(rio_t *rp);
int parse_url(char *url, char *host, char *path, char *cgiargs);
void make_request(int fd, char *host, char *path, char *reply);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void terminate(int param);

cache_LL cache = Calloc(1, sizeof(cache_LL));
/*
 * Main function
 */
int main(int argc, char **argv)
{
    printf("-------- START PROXY INFO --------\r\n");
    printf("%s%s%s", user_agent, accept_, accept_encoding);
    printf("--------- END PROXY INFO ---------\r\n");

    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    signal(SIGPIPE, terminate);


    if (argc !=  2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
	dbg_printf("\nDEBUG CHECK\n");
        serve(connfd);
        Close(connfd);
    }

    return 0;
}


/*
 * Serves a client's request
 */
 void serve(int file_d)
 {
    int is_static;
    //    struct stat sbuf;
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char buf[MAXLINE], host[MAXLINE], path[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read in a request from client */
    Rio_readinitb(&rio, file_d); // initialize rio buffer with file descriptor
    Rio_readlineb(&rio, buf, MAXLINE); // read in line
    // move from buffer into string format
    sscanf(buf, "%s %s %s", method, url, version);

    // figure this out
    if (strcasecmp(method, "GET"))
    {
        // use CS:APP error functions?
        clienterror(file_d, method, "501", "Request not implemented", "Nope");
        return;
    }

    read_headers(&rio);

    // Parse URL out of request
    dbg_printf("PRE-PARSE\n");
    is_static = parse_url(url, host, path, cgiargs);
    dbg_printf("POST-PARSE\n");



    /* Serve static content */
    if (is_static)
    {
        // check if file requested is legal
        /*
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            dbg_printf("Status read error!\n");
            clienterror(file_d, path, "403", "Forbidden", "Can't read the file");
            return;
	    }*/

        strcpy(buf, "");
        make_request(file_d, host, path, buf);
    }

 }


 /*
  * Sends error to proxy's client as html file
  */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];

    /* Build body */
    sprintf(body, "<html><title>Web Proxy Error</title>");
    sprintf(body, "%s<body bgcolor =\"#FF8680\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Alex & Saumya's Web Proxy</em>\r\n", body);

    /* Print out response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

//review later
void terminate (int param)
{
    printf ("SIGPIPE, quitting ...\n");
    exit(1);
}



/*
 * Reads in an ignores headers of requests
 */
void read_headers(rio_t *rp)
{
   char buf[MAXLINE];

   Rio_readlineb(rp, buf, MAXLINE);
   while(strcmp(buf, "\r\n"))
   {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
   }

   return;
}


/*
 * Parses URL from GET request into hostname and path
 */
int parse_url(char *url, char *host, char *path, char *cgiargs)
{
    char *path_ptr;

    char temp[MAXLINE];
    strcpy(temp, url);

    sscanf(url, "http://%s", url);

    if ((path_ptr = strchr(url, '/')) == NULL)
    {
        strcpy(host, url);
	strcpy(cgiargs, "");
        strcpy(path, "/");

    }

    else
    {
        strcpy(path, path_ptr);
	strncpy(host, url, (int)(path_ptr - url));
	strcpy(cgiargs, "");
    }

    dbg_printf("LEAVING PARSE_URL()!\n");
    dbg_printf("   URL = %s\n", temp);
    dbg_printf("  HOST = %s\n", host);
    dbg_printf("  PATH = %s\n\n", path);

    return 1;

}


void make_request(int fd, char *host, char *path, char *reply)
{
    reply = checkCache(cache, char* path) != NULL
    if(reply != NULL)
        return;

    int net_fd;
    char buf[MAXBUF];
    rio_t rio;

    net_fd = Open_clientfd(host, 80);

    sprintf(buf, "GET %s HTTP/1.0\r\n", path);

    Rio_writen(net_fd, buf, strlen(buf));


    strcpy(reply, "");
    Rio_readinitb(&rio, net_fd);
    dbg_printf("Just sent request:\n");
    dbg_printf("    %s\n", buf);
    dbg_printf("To host:\n");
    dbg_printf("    %s\n", host);
    dbg_printf("Reading from host ... \n");
    int sizeOfWebObject = Rio_readlineb(&rio, reply, MAXLINE);
    if(sizeOfWebObject < MAX_OBJECT_SIZE)
        addToCache(cache, reply, path, sizeOfWebObject);
    dbg_printf("Writing to client ... \n");
    Rio_writen(fd, reply, strlen(reply));
}


/*
 * Serves static html to client
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, buf[MAXBUF];

    sprintf(buf, "Host: www.cmu.edu\r\n");
    sprintf(buf, "%sUser-Agent: %s\r\n", buf, user_agent);
    sprintf(buf, "%sAccept: %s\r\n", buf, accept_);
    sprintf(buf, "%sAccept-Encoding: %s\r\n", buf, accept_encoding);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n", buf);
    Rio_writen(fd, buf, strlen(buf));




    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);

} */









