/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"
int main(int argc, char **argv)
{
    int clientfd,i;
    char *host, *port, buf[MAXLINE],temp;
    rio_t rio;

    if (argc != 3) {
    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port); // getaddrinfo & socket & connect
    Rio_readinitb(&rio, clientfd); //rio_t 구조체 초기화, 버퍼 사용-> rio_readlineb and rio_readnb

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf)); //Unbuffered
        Rio_readlineb(&rio, buf, MAXLINE); //buffered, 한 줄씨 읽어들인다 : '\n' 전까지 읽어들임.
        //fputs(buf,stdout);
        if(!(strcmp(buf,"\n")) || !(strcmp(buf,"\0"))) continue;
        if(!(strcmp(buf,"exit\n"))) break;
        for(i=0;i<MAXLINE;i++){
            temp = buf[i];
            if(temp == '\n'){fputc(temp,stdout);break;}
            if(temp == '!') temp = '\n';
            fputc(temp, stdout);
        }
        fflush(stdin); fflush(stdout);
    }
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */
