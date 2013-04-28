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
static const char *accept_ = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";

void serve(int file_d);
void read_headers(rio_t *rp);
int parse_url(char *url, char *host, char *path, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);


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
    struct stat sbuf;
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
    is_static = parse_url(url, host, path, cgiargs);

    /* Parse out GET request */
    if (stat(path, &sbuf) < 0)
    {
        clienterror(file_d, path, "404", "Not found", "Couldn't access site");
        return;
    }

    /* Serve static content */
    if (is_static)
    {
        // check if file requested is legal
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            clienterror(file_d, path, "403", "Forbidden", "Can't read the file");
            return;
        }

        serve_static(file_d, path, sbuf.st_size);
    }

    /* Serve dynamic content */
    else
    {
        // check if file requested is legal
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        {
            clienterror(file_d, path, "403", "Forbidden", "Can't read the file");
            return;
        }

        serve_dynamic(file_d, path, cgiargs);
    }

 }


 /*
  * Sends error to proxy's client as html file
  */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];

    /* Build body */
    sprintf(body, "<html><title>Web Proxy Error</title><");
    sprintf(body, "%s<body bgcolor =""FF8680"">\r\n", body);
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
    Rio_writen(fd, body, strlen(buf));
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
    char *frst_ptr;
    char *last_ptr;

    /* i don't think this stuff is necessary ...
    if (!strstr(url, "cgi-bin"))
    {
        strcpy
    }
    */

    if ((frst_ptr = strstr(url, "www.")) == NULL)
    {
        app_error("URL doesn't have a \'.\' in it?");
        exit(1);
    }

    if ((last_ptr = strrchr(url, '.')) == frst_ptr)
    {
        app_error("Only one \'.\' in URL?");
        exit(1);
    }

    long diff = (unsigned long)last_ptr - (unsigned long)frst_ptr;
    diff += 4; // ending length
    if (diff <= 0 || diff >= MAXLINE)
    {
        app_error("I've dont something wrong in parsing...\n");
        exit(1);
    }

    strcpy(cgiargs, "");
    strncpy(host, frst_ptr, diff);
    host[diff] = '\0';

    dbg_printf("Just checking. Got HOST: %s\n", host);
    dbg_printf("               From URL: %s\n\n", url);

    int diff2 = strlen(url) - strlen("http://") - diff;
    strcpy(path, url + diff2);

    printf("\nCHECK 1\n");

    printf("path: %s\n", path);
    if (strstr(path, "/") != path)
    {
        char *temp = " ";
        strcpy(temp, path);
	printf("temp: %s\n", temp);
        path = "/";
        strcat(path, temp);
    }

    printf("\nCHECK 2\n");

    dbg_printf("Also checking. Got PATH: %s\n", path);
    dbg_printf("               From URL: %s\n\n", url);


    return 1;

}


/*
 * Serves static html to client
 */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, buf[MAXBUF];

    sprintf(buf, "Host: www.cmu.edu\r\n");
    sprintf(buf, "User-Agent: %s\r\n", user_agent);
    sprintf(buf, "Accept: %s\r\n", accept_);
    sprintf(buf, "Accept-Encoding: %s\r\n", accept_encoding);
    sprintf(buf, "Connection: close\r\n");
    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen(fd, buf, strlen(buf));

    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}


/*
 * Garbage?
 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    return;
}








