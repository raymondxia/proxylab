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
#include "cache.h"

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


typedef struct {
    char *url;
    char *host;
    char *path;
    char *host_header;
    char *other_headers;
    int port;
    int fd;
} req_args;

void serve(int file_d);
void read_headers(rio_t *rp, char* host_header, char *other_headers);
int parse_url(char *url, char *host, char *path, char *cgiargs);
void make_request(req_args *argstruct);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void terminate(int param);

cache_LL* cache;

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

    //cache initialization
    cache = (cache_LL*) Calloc(1, sizeof(cache_LL));
    cache->head = NULL;
    cache->size = 0;


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
        serve(connfd);
        Close(connfd);
    }

    return 0;
}


/*
* Serves a client's request.
* file_d is the file descriptor
*/
void serve(int file_d)
{
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char buf[MAXLINE], host[MAXLINE], path[MAXLINE], cgiargs[MAXLINE];
    char host_header[MAXLINE], other_headers[MAXLINE];
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


    read_headers(&rio, host_header, other_headers);

    // Parse URL out of request
    dbg_printf("PRE-PARSE\n");

    // make a new string so as not to defile original url
    char url_arg[MAXLINE];
    strcpy(url_arg, url);
    int port = parse_url(url_arg, host, path, cgiargs);
    dbg_printf("POST-PARSE\n");


    if (!strncmp(buf, "https", strlen("https")))
        printf("\n\n\n EXPECT A DNS ERROR \n\n");


    dbg_printf("\nRequesting with URL : %s\n\n", url);
    make_request(file_d, url, host, path, host_header, other_headers, port);


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
    printf ("SIGPIPE . . . ignoring!\n");
    //    exit(1);
}



/*
* Reads in and ignores headers of requests.
* host_header and other_headers are merely buffers 
* to store the information obtained from reading 
* regarding the host headers and other necessary
* headers respectively.
*/
void read_headers(rio_t *rp, char *host_header, char *other_headers)
{
 char buf[MAXLINE];

 strcpy(other_headers, "");

 dbg_printf("\nReading headers\n-----------\n");

 Rio_readlineb(rp, buf, MAXLINE);
 while(strcmp(buf, "\r\n"))
 {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);

    int prefix = strlen("Host: ");

    if (!strncmp(buf, "Host: ", prefix))
        strcpy(host_header, buf + prefix);

    /* We add other headers when required */
    if (strncmp(buf, "User-Agent: ", strlen("User-Agent: ")) &&
        strncmp(buf, "Accept: ", strlen("Accept: ")) &&
        strncmp(buf, "Accept-Encoding: ", strlen("Accept-Encoding: ")) &&
        strncmp(buf, "Connection: ", strlen("Connection: ")) &&
        strncmp(buf, "Proxy-Connection: ", strlen("Proxy-Connection: ")))
    {
        sprintf(other_headers, "%s%s", other_headers, buf);
    }
    /* We added this in order to ignore garbage headers */
    if (buf[0] > 90 || buf[0] < 65)
    {
     return;
 }
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

    memset(host, 0, sizeof(host));
    memset(path, 0, sizeof(path));


    //    if (!strncmp(buf, "https", strlen("https")))
    // strcpy(


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

        dbg_printf("strlen\n");

        path_ptr[0] = '\0';
        strcpy(host, url);
    }


    char *port_ptr;
    int port;
    if ((port_ptr = strchr(host, ':')) != NULL)
    {
        port_ptr++;
        dbg_printf("Port Ptr: %s\n", port_ptr);
        port = atoi(port_ptr);
        dbg_printf("Got port number: %d\n", port);
    }

    else
        port = 80;

    dbg_printf("LEAVING PARSE_URL()!\n");
    dbg_printf(" URL = %s\n", temp);
    dbg_printf(" HOST = %s\n", host);
    dbg_printf(" PATH = %s\n\n", path);

    return port;

}

/* Make request creates a request using the information such as the port, 
 * file descriptor, url, host, path & necessary headers. These are stored 
 * in a structure called argstruct (in order to use Pcreate_thread for
 * each new request that needs to be made). If the object associated with the url 
 * exists in the cache, we return the stored data from the cache. 
 * If not, the complete request with the 
 * given information is created and stored in buf which is then read
 * MAXBUF bytes at a time. Information about the size & data from the web object
 * are kept track of and stored in cache_object_size & cache_object so that
 * the web object may be cached later.
 */
void make_request(req_args *argstruct)
{
    //Unpacking all the information from the argument struct
    int fd = argstruct->fd;
    int port = argstruct->port;
    char *url = argstruct->url;
    char *host = argstruct->host;
    char *path = argstruct->path;
    char *host_header = argstruct->host_header;
    char *other_headers = argstruct->other_headers;

    //We first search for an object in the cache
    web_object* found = checkCache(cache, url);

    //If the object is found, write the data back to the client
    if(found != NULL) {
        Rio_writen(fd, found->data, found->size);
        return;
    }

    //If it is not found in the cache, make a request
    int net_fd;
    char buf[MAXBUF], reply[MAXBUF];
    rio_t rio;

    net_fd = Open_clientfd(host, port);

    if (net_fd < -1)
    {
        clienterror(fd, host, "DNS!", "DNS error, this host isn't a host!", "Ah!");
        return;
    }

    /* The following code adds the necessary information to make buf a complete request */
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    printf("Send request buf: \n%s\n", buf);

    //Adds the host
    if (!strlen(host_header))
        sprintf(buf, "%sHost: %s\r\n", buf, host);
    else
        sprintf(buf, "%sHost: %s\r\n", buf, host_header);]

    //Adds the default headers
    sprintf(buf, "%s%s", buf, user_agent);
    sprintf(buf, "%s%s", buf, accept_type);
    sprintf(buf, "%s%s", buf, accept_encoding);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n", buf);
    //Adds the other headers
    sprintf(buf, "%s%s\r\n", buf, other_headers);

    dbg_printf("\n   SENDING REQUEST\n");
    dbg_printf("%s\n", buf);
    dbg_printf("\n   ENDING  REQUEST\n");


    int check = rio_writen(net_fd, buf, strlen(buf));
    if (check != strlen(buf))
      clienterror(fd, "Wrote wrong", "WRITE", "Writting Crash", "Error writting.");


  strcpy(reply, "");
  strcpy(buf, "");
  Rio_readinitb(&rio, net_fd);

  int read_return;

    // read_return = Rio_readlineb(&rio, reply, MAXLINE);
    // sprintf(buf, "%s%s", buf, reply);

    //int file_check = 1;


  char cache_object[MAX_OBJECT_SIZE];
  //cache_object size finds the total size of the data 
  //by summing the total number of bytes received from 
  //every read.
  int cache_object_size = 0;

  dbg_printf("Entering reading loop\n");
  do
  {
    strcpy(reply, "");

    dbg_printf("Read \n");
    read_return = rio_readnb(&rio, reply, MAXBUF);

    if (read_return < 0)
     clienterror(fd, "Reading reply", "READ", "Reading Crash", "Error reading.");

	//dbg_printf("Double check \n");
    dbg_printf("Read return: %d\n", read_return);
    dbg_printf("Object size: %d\n", cache_object_size);

    //Add the bytes returned from the read to the total object size
    cache_object_size += read_return;

    //As long as our object size is within the max, continue to add data
    //to the cache_object so that we can add it to the cache later.
    if ( cache_object_size < MAX_OBJECT_SIZE )
    {
        dbg_printf("Cache . . . \n");
        sprintf(cache_object, "%s%s", cache_object, reply);
    }

    dbg_printf("Write . . . \n");
    //Write the data back to the client
    rio_writen(fd, reply, read_return);

    dbg_printf("Loop\n\n");
} while ( read_return > 0);

//If the object size is less than the maximum, add it to the cache
if (cache_object_size < MAX_OBJECT_SIZE)
{
    dbg_printf("\nAdding to cache . . . \n");
    addToCache(cache, cache_object, url, cache_object_size);
    dbg_printf("Done!\n");
}

return;
}
