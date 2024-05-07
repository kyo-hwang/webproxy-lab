#include "csapp.h"

int main(int argc, char **argv)
{
    int client_fd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc !=3){
        fprintf(stderr,"usage : %s <host><port>\n",argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    client_fd = Open_clientfd(host,port);
    Rio_readinitb(&rio,client_fd);

    while(Fgets(buf,MAXLINE,stdin)!=NULL){
        Rio_writen(client_fd,buf,strlen(buf));
        //stdout 표준 입력에 buf의 내용을 쓴다.
        Fputs(buf,stdout); 
    }
    Close(client_fd);
    exit(0);
}