/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio,fd);
  Rio_readlineb(&rio,buf,MAXLINE);
  printf("Request headers:\n");
  printf("%s",buf);
  sscanf(buf,"%s %s %s",method,uri,version);
  printf("uri: %s\n",uri);
  if(strcasecmp(method,"GET")){
    clienterror(fd,method,"501","Not implementd","Tiny does not implement this method");
    return ;
  }
  read_requesthdrs(&rio);

  is_static = parse_uri(uri,filename,cgiargs);
  if (stat(filename, &sbuf)<0){
    clienterror(fd,filename,"404","Not found","Tiny couldn't find this file");
    return;
  }
  if(is_static){
    if(!(S_ISREG(sbuf.st_mode))|| !(S_IRUSR & sbuf.st_mode)){
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't read the file");
      return ;
    }
    //stat로 얻은 파일의 속성으로 파일의 사이즈를 얻을 수 있음.
    serve_static(fd,filename,sbuf.st_size);
  }
  else{
    if(!(S_ISREG(sbuf.st_mode))|| !(S_IRUSR & sbuf.st_mode)){
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't read the file");
      return ;
    }
    serve_dynamic(fd,filename,cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXLINE];

  sprintf(body,"<html><title>Tiny Error</title>");
  sprintf(body,"%s<body bgcolor=""ffffff"">\r\n",body);
  sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
  sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n",body);
  
  //응답 라인
  sprintf(buf, "HTTP/1.0 %s %s\r\n",errnum,shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  //응답 헤더
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n",(int)strlen(buf));
  Rio_writen(fd,buf,strlen(buf));
  //응답 바디
  Rio_writen(fd,body,strlen(body));
}

//헤더를 읽는 함수, 이 프로그램에서는 요청 헤더를 사용하지 않으므로 그냥 버퍼 비우기만
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  //처음에는 buf에 아무 값도 들어 있지 않으니까 buf를 채워 준다.
  Rio_readlineb(rp,buf,MAXLINE);
  while(strcmp(buf,"\r\n")){
    Rio_readlineb(rp,buf,MAXLINE);
    printf("%s",buf);
  }
  return;
}

//요구하는 컨텐츠가 동적인지 정적인지 판단하고 찾을 file 경로와 인수 수정
int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;
  if(!strstr(uri,"cgi-bin")){
    strcpy(cgiargs,"");
    strcpy(filename,".");
    strcat(filename,uri);
    if(uri[strlen(uri)-1]=='/') strcat(filename,"home.html");
    return 1;
  }
  else{
    ptr = index(uri,'?');
    if(ptr){
      strcpy(cgiargs,ptr+1);
      // \0은 문자열의 끝을 의미함 즉 인수 부분이랑 파일 위치랑 분리함
      *ptr = '\0';
    }
    else strcpy(cgiargs,"");
    strcpy(filename,".");
    strcat(filename,uri);
    return 0;
  }
}

//응답 라인, 응답 헤더, 응답 바디를 다 채워야 할 것.
void serve_static(int fd, char *filename, int filesize){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  //읽기 버퍼와 연결
  rio_t rp;

  //응답 라인, 헤더 전송
  get_filetype(filename,filetype);
  sprintf(buf,"HTTP/1.0 200 OK\r\n");
  sprintf(buf,"%sServer: Tiny Web Server\r\n",buf);
  sprintf(buf,"%sConnection: close\r\n",buf);
  //filesize는 doit함수에서 이미 찾았음.
  sprintf(buf,"%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf,"%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers\n");
  printf("%s",buf);

  //전송할 파일 읽기
  //응답 바디 전송
  srcfd = Open(filename, O_RDONLY, 0);
  //파일 디스크립터를 직접 얻어서 하는 방식이 아닌 직접 메모리에 접근하여 데이터를 가져오는 방식
  //void *mmap(void *addr, size_t length, int prot, int flags,int fd, off_t offset);
  //매핑될 주소, 매핑할 길이, 메모리 보호 특성, 매핑의 유형, 파일 디스크립트, 매핑할 때, 올길 데이터의 시작점
  srcp = (char*)malloc(filesize);
  //파일 식별자와 읽기 버퍼와 연결
  Rio_readinitb(&rp,srcfd);
  //읽기 버퍼에 있는 내용을 srcp에 담는다.
  Rio_readnb(&rp,srcp,filesize);
  Close(srcfd);
  Rio_writen(fd,srcp,filesize);
  //mime을 소켓에 전달하고 메모리를 해제시킨다.
  free(srcp);
}

void serve_dynamic(int fd, char *filename, char* cgiargs){
  char buf[MAXLINE], *emptylist[] = {NULL};

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork()==0){
    //실행파일을 돌리기 전에 인수를 환경 변수에 등록해준다.
    if(strcmp(cgiargs,"")==0){
      printf("!!!!!!!!!!!인수 아무것도 입력 안함..\n");
    }
    setenv("QUERY_STRING",cgiargs,1);
    Dup2(fd,STDOUT_FILENO);
    Execve(filename, emptylist,environ);
  }
  Wait(NULL);
}

void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename, ".html")) strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif")) strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png")) strcpy(filetype, "image/png");
  else if(strstr(filename, ".jpg")) strcpy(filetype, "image/jpeg");
  else if(strstr(filename,".mp4")) strcpy(filetype, "video/mp4");
  else strcpy(filetype,"text/plain");
}