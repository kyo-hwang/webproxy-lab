/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */

#include<csapp.h>

#include<stdlib.h>

#include "../csapp.h"
//파일의 속성을 가지기 위해 필요한 라이브러리
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>

#include <sys/mman.h>
//파일의 형식
#include <fcntl.h>
//str처리
#include <string.h>


int main(void) {
  struct stat sbuf;
  int srcfd;
  char *srcp;
  char *buf, *p;
  char arg1[MAXLINE],arg2[MAXLINE], content[MAXLINE];
  int n1=0,n2=0;

  srcfd = Open("/home/shane5969/webproxy-lab/tiny/cgi-bin/add.html",O_RDONLY,0);
  //파일에 대한 정보를 얻음
  if (stat("/home/shane5969/webproxy-lab/tiny/cgi-bin/add.html", &sbuf) == -1) {
        perror("stat");
        exit(0);
    }
  srcp = Mmap(0,sbuf.st_size,PROT_READ,MAP_PRIVATE,srcfd,0);
  Close(srcfd);

  //환경변수로 부터 인수를 가져와 정수형으로 변환
  if((buf=getenv("QUERY_STRING"))!=NULL){
    if(strcmp(buf,"")==0){
      printf("Connection: close\r\n");
      printf("Content-length: %d\r\n",(int)sbuf.st_size);
      printf("Content-type: text/html\r\n\r\n");
      // printf("fafadqw");
      printf("%s",srcp);
      Munmap(srcp,sbuf.st_size);
      fflush(stdout);
      exit(0);
    }
    p = strchr(buf,'&');
    *p = '\0';
    strcpy(arg1,buf);
    strcpy(arg2,p+1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }
  sprintf(content,"QUERY_STRING=%s", buf);
  sprintf(content,"Welcom to add.com: ");
  sprintf(content,"%sTHE Internet addition portal. \r\n<p>", content);
  sprintf(content,"%sThe answer is : %d + %d = %d\r\n<p>",content,n1,n2,n1+n2);
  sprintf(content,"%sThanks for visiting!\r\n", content);

  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s",content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
