#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char* argv[])
{
   if(argc>1)
   {
      fprintf(2,"Only 1 argument is needed!\n");
      exit(1);
   }	   
   int p1[2];
   int p2[2];
   // char* s="a";
   pipe(p1);
   pipe(p2);
   // write(p[1],s,1);
   int pid;
   if((pid=fork())<0)
   {
      fprintf(2,"fork error\n");
      exit(1);
   }
   else if(pid==0)
   {
      close(p1[1]);
      close(p2[0]);      
      char buf[10];	   
      read(p1[0],buf,4);  
      close(p1[0]);
     // printf("aa:%s\n",buf);
      printf("%d: received %s\n",getpid(),buf);
      write(p2[1],"pong",strlen("pong"));
      close(p2[1]);
   }
   else
   {   
      close(p1[0]);
      close(p2[1]);      
      write(p1[1],"ping",strlen("ping")); 
      close(p1[1]);     	
      // wait(0);
      char buf[10];   
      read(p2[0],buf,4);
      printf("%d: received %s\n",getpid(),buf);
      close(p2[0]);
   }  
   exit(0);
}
