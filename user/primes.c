#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define FDRD 0
#define FDWT 1
void primes(int p1[],int p2[])
{   
   if(fork()==0)
   {
     
   
      int num[1];	
      // close(p1[FDWT]);
      if(read(p1[FDRD],num,sizeof(num))!=sizeof(num))
      {
	  close(p1[FDRD]);
          close(p2[FDRD]);
          close(p2[FDWT]);	  
	  exit(0);
      }
      int p=num[0];
     
      printf("prime %d\n",num[0]);
     // printf("p1[0]:%d,p1[1]:%d,p2[0]:%d,p2[1]:%d\n",p1[0],p1[1],p2[0],p2[1]);
      while(read(p1[FDRD],num,sizeof(num))==sizeof(num))
      {
	  // printf("%d\n",num[0]);
           if(num[0]%p!=0) write(p2[FDWT],num,sizeof(num));
      }
      
      close(p1[FDRD]);	
      close(p2[FDWT]);
   
      pipe(p1);
      primes(p2,p1);
      exit(0);      
   }
   else
   {
      close(p1[FDRD]);
      close(p2[FDWT]);
      close(p2[FDRD]);      
      wait(0);// wait child process to end
      exit(0);
   }

}

int main(int argc,char* argv[])
{
   int p1[1];
   int p2[2];
   pipe(p1);
   pipe(p2);
   int i[1];
   for(i[0]=2;i[0]<=35;i[0]++)
   {
      write(p1[FDWT],i,sizeof(i));
   }
   close(p1[FDWT]);
   primes(p1,p2);
  
   // wait(0);
    exit(0);
}


