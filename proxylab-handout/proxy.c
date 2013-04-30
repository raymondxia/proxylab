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
static const char *accept_type = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";

void serve(int file_d);
void read_headers(rio_t *rp, char* host_header);
int parse_url(char *url, char *host, char *path, char *cgiargs);
void make_request(int fd, char *host, char *path, char *host_header);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void terminate(int param);


/*
* Main function
*/
int main(int argc, char **argv)
{
    printf("-------- START PROXY INFO --------\r\n");
    printf("%s%s%s", user_agent, accept_type, accept_encoding);
    printf("--------- END PROXY INFO ---------\r\n");

    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    signal(SIGPIPE, terminate);


    if (argc != 2)
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
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char buf[MAXLINE], host[MAXLINE], path[MAXLINE], cgiargs[MAXLINE];
    char host_header[MAXLINE];
    rio_t rio;

    /* Read in a request from client */
    Rio_readinitb(&rio, file_d); // initialize rio buffer with file descriptor
    Rio_readlineb(&rio, buf, MAXLINE); // read in line
    // move from buffer into string format
    sscanf(buf, "%s %s %s", method, url, version);

    // figure this out
    if (strcasecmp(method, "GET"))
    {
        dbg_printf("Asked for something other than GET\n");
        // use CS:APP error functions?
        clienterror(file_d, method, "501", "Request not implemented", "Nope");
        return;
    }


    printf("\n<><><><><><><><><><><><><><><><><><>\n");
    printf("<><><><><><><><><><><><><><><><><><>\n");


    read_headers(&rio, host_header);

    // Parse URL out of request
    dbg_printf("PRE-PARSE\n");
    parse_url(url, host, path, cgiargs);
    dbg_printf("POST-PARSE\n");

    make_request(file_d, host, path, host_header);


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
void read_headers(rio_t *rp, char *host_header)
{
   char buf[MAXLINE];

   dbg_printf("\nReading headers\n-----------\n");

   Rio_readlineb(rp, buf, MAXLINE);
   while(strcmp(buf, "\r\n"))
   {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);

        int prefix = strlen("Host: ");

	if (!strncmp(buf, "Host: ", prefix))
            strcpy(host_header, buf + prefix);
   }

   dbg_printf("--------\nDone with headers\n");
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
    dbg_printf(" URL = %s\n", temp);
    dbg_printf(" HOST = %s\n", host);
    dbg_printf(" PATH = %s\n\n", path);

    return 1;

}


void make_request(int fd, char *host, char *path, char *host_header)
{

    int net_fd;
    char buf[MAXBUF], reply[MAXBUF];
    rio_t rio;

    net_fd = Open_clientfd(host, 80);

    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    printf("Send request buf: \n%s\n", buf);

    if (host_header == NULL)
        sprintf(buf, "%sHost: %s\r\n", buf, host);
    else
        sprintf(buf, "%sHost: %s\r\n", buf, host_header);
    sprintf(buf, "%sUser-Agent: %s\r\n", buf, user_agent);
    sprintf(buf, "%sAccept: %s\r\n", buf, accept_type);
    sprintf(buf, "%sAccept-Encoding: %s\r\n", buf, accept_encoding);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n", buf);

    Rio_writen(net_fd, buf, strlen(buf));


    strcpy(reply, "");
    strcpy(buf, "");
    Rio_readinitb(&rio, net_fd);

    int read_return;

    // read_return = Rio_readlineb(&rio, reply, MAXLINE);
    // sprintf(buf, "%s%s", buf, reply);

    //int file_check = 1;


    do
    {
        read_return = Rio_readnb(&rio, reply, MAXBUF);
        //dbg_printf("READ_RETURN = %d\n", read_return);

	//printf("\n==============================\n");
	//printf("==============================\n");
	//read_headers(&rio);
	//Rio_writen(1, reply, read_return);
        Rio_writen(fd, reply, read_return);
    }while( read_return > 0);

    return;
}
