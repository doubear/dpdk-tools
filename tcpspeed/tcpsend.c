#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>

#include <time.h>


#define SIZE 1024
#define NUMBER 3200000

const int port = 8080;
const char*ip = "192.168.1.205";
 
long tv1 = 0,tv2 = 0,diff = 0;

long sys_gettimeofday_us(void)
{
	struct  timeval tv;
    gettimeofday(&tv,NULL);
	return (tv.tv_sec*1000000 + tv.tv_usec);//us
}

int main()
{
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0)
	{
		perror("socket error");
	}
	struct sockaddr_in remote;
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	remote.sin_addr.s_addr = inet_addr(ip);
	
	int ret = connect(sock,(struct sockaddr*)&remote,sizeof(remote));
	if(ret<0)
	{
		perror("connect error");
	}
	char buf[SIZE];
	memset(buf,'a',sizeof(buf));
	long i = NUMBER;
	long number = NUMBER;
	tv1 = sys_gettimeofday_us();
	
	while(i--)
	{
		memset(buf,'a',sizeof(buf));
		//ssize_t _s=read(0,buf,sizeof(buf)-1);
		//if(_s>0)
		//{
			//buf[_s]='\0';
			send(sock,buf,strlen(buf),0);
			//printf("%s\n",buf);
			//write(sock,buf,strlen(buf));
			//memset(buf,'T',sizeof(buf));
			//ssize_t f=read(sock,buf,sizeof(buf)-1);
			/*if(f>0)
			{
				buf[f]='\0';
				printf("%s\n",buf);
			}
			printf("server echo:%s\n",buf);
			*/
		//}
		//printf("%ld\n",i);
	}
	tv2 = sys_gettimeofday_us();
	diff = tv2 - tv1;
	double speed = 1000*1000.0*number/diff;
	printf("diff:%ld us  speed:%lf/s v:%lfMb/s\n",diff,speed,speed*SIZE/1024/1024);
	
	return 0;
}


