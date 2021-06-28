/*
 * echo - read and echo text lines until client closes connection
 */
/* $begin echo */
#include "csapp.h"

void Echo(int connfd)
{
    int n;
    rio_t rio;
    char buf[MAXLINE];
    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", n);
        n = store(buf,connfd); //store함수에서 show, buy, sell에 따른 Tree 최신화
        //strcpy(buf,"test");
        //printf("%s",buf);
        Rio_writen(connfd, buf, n);// buf에서 fd로 n 바이트 전송
    }
}
/* $end echo */

