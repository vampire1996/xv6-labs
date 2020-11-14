#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define FDSTDIN 0 // standard input file descriptor
#define FDSTDOUT 1

int main(int argc,char* argv[])
{
   char buf[10]; 
   char* cur[MAXARG];// pointer array cur[i] is a pointer	
   int i=0,j=0;  
   for(i=0;i<argc-1;i++)
   {
      cur[i]=argv[i+1];
   }   
   cur[i]=malloc(512);
   while(read(FDSTDIN,buf,1)==1)
   { 
      if(i==argc-1) printf("%s,%s\n",buf,cur[i]);	   
      switch(buf[0])
      {	      
        case '\n':
	{
           	
           if(fork()==0)
	   {
	      // printf("%s\n",cur[argc-1]);	   
	      exec(cur[0],cur);
	      exit(0);   
	   }
	   wait(0);
	   for(int k=argc-1;k<=i;k++)
	   {
	      // printf("%s\n",cur[k]);	 
	      // for(int l=0;l<strlen(cur[k),l++) cur[k][l]=0;  
	      memset(cur[k],0,512);
	       free(cur[k]);
	   }
           i=argc-1;
	   cur[i]=malloc(512);
           j=0;	   
	   break;
	}
	case ' ':
	{
	   i++;	
	   j=0;
	   if(i<MAXARG) cur[i]=malloc(512);
           break;	  
	}
	default :
	{
         // strcat(cur[i],buf);
	  cur[i][j++]=buf[0];
	  break;
	}

      }
      // printf("%s,%c\n",buf,cur[i][j]);	
   }	   
   exit(0);
}
