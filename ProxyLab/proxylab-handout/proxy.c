#include "csapp.h"
#include "sbuf.h"
#include "threadPool.h"
#include "cache.h"
#include "readerWriter.h"
#include <stdio.h>

struct Uri
{
    char host[MAXLINE]; //hostname
    char port[MAXLINE]; //端口
    char path[MAXLINE]; //路径
};

struct ReqLine{
    char method[MAXLINE];
    char uri[MAXLINE];
    char version[MAXLINE];
};

struct Uri uriData;
struct ReqLine reqLine;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

// 要防止函数报错导致的进程中断

void doit(int fd);
void rw_resp(int clientfd, int connfd, char *obj_buf);
void send_reqHdrs(rio_t rio, int clientfd);
void parse_uri(char *uri, struct Uri *uri_data);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    // 一个健壮的服务器必须捕获SIGPIPE信号，并检查write函数是否有EPIPE错误（写已关闭的连接超过一次，会引发该错误）
    if(Signal(SIGPIPE, SIG_IGN)==SIG_ERR)
        unix_error("mask signal pipe error!");
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // 初始化线程池
    initThreadPool();
    // 初始化读者写者
    init_reader_writer();
    pthread_t tid;
    // 不断调整线程池的规模
    Pthread_create(&tid, NULL, adjustThreadPool, NULL);

    // 接收客户端请求
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // 将请求插入FIFO线程安全队列
        sbuf_insert(&sbuf, connfd);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
    }
    // 虽然不会执行这个，但是free sbuf
    deInitThreadPool();
}

void doit(int connfd){
    char buf[MAXLINE];
    rio_t rio;
    int clientfd;

    // 读客户端
    Rio_readinitb(&rio, connfd);
    // 读请求行 method URI version
    if (!Rio_readlineb(&rio, buf, MAXLINE))
    {
        clienterror(connfd, NULL, "400", "Bad request",
                    "The server proxy does not understand the request");
        return;
    }
    printf("%s", buf);
    sscanf(buf, "%s %s", reqLine.method, reqLine.uri);
    strcpy(reqLine.version, "HTTP/1.0");
    if (strcasecmp(reqLine.method, "GET"))
    {
        clienterror(connfd, reqLine.method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }
    // 如果没写端口号则默认是80 解析请求行uri
    parse_uri(reqLine.uri, &uriData);
    
    char *res = NULL;
    // 如果能读到cache 则不必连接服务器
    readCache(uriData.path, &res);
    if(res){
        Rio_writen(connfd, res, strlen(res));
        Free(res);
    }
    else{
        // 否则，请求服务器并转发响应
        clientfd = Open_clientfd(uriData.host, uriData.port);
        // 向服务器发送请求行
        send_reqHdrs(rio, clientfd);

        char obj_buf[MAX_CACHE_SIZE];
        // 转发响应并存储
        rw_resp(clientfd, connfd, obj_buf);

        // 在满足条件的情况下，缓存
        int obj_buf_len = strlen(obj_buf)+1;
        if(obj_buf_len <= MAX_OBJECT_SIZE)
            writeCache(uriData.path, obj_buf, obj_buf_len);

        Close(clientfd);
    }
}

void parse_uri(char *uri, struct Uri *uri_data){
    char *host = strstr(uri, "//");
    if(!host){
        char *path = strstr(uri, "/");
        if (!path)
            strcpy(uri_data->path, path);
        strcpy(uri_data->port, "80");
        return;
    }
    else{
        char *port = strstr(host+2, ":");
        if(port){
            int tmp;
            sscanf(port + 1, "%d%s", &tmp, uri_data->path);
            sprintf(uri_data->port, "%d", tmp);
            *port = '\0';
        }
        else{
            char *path = strstr(host+2, "/");
            if (!path)
            {
                strcpy(uri_data->path, path);
                strcpy(uri_data->port, "80");
                *path = '\0';
            }
        }
        strcpy(uri_data->host, host+2);
    }
}

void send_reqHdrs(rio_t rio, int clientfd){
    char buf[MAXLINE];
    // 写请求行
    sprintf(buf, "%s %s %s\r\n", reqLine.method, uriData.path, reqLine.version);
    Rio_writen(clientfd, buf, strlen(buf));
    // 写请求头
    sprintf(buf, "Host: localhost\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "%s", user_agent_hdr);
    Rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    // 读写请求头
    while (Rio_readlineb(&rio, buf, MAXLINE))
    {
        if(!strcmp(buf, "\r\n")){
            break;
        }
        if(strstr(buf, "Connection") || strstr(buf, "Host") || strstr(buf, "User-Agent")){
            continue;
        }
        printf("%s", buf);
        Rio_writen(clientfd, buf, strlen(buf));
    }
    sprintf(buf, "\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
}

void rw_resp(int clientfd, int connfd, char *obj_buf){
    rio_t rio;
    char buf[MAXLINE];

    Rio_readinitb(&rio, clientfd);
    // 读服务器端输出 写向客户端
    int n;
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) > 0)
    {
        strncat(obj_buf, buf, n);
        Rio_writen(connfd, buf, n);
    }
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor="
                 "ffffff"
                 ">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Proxy Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}