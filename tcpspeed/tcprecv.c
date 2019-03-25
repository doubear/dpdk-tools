#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>


#define SIZE 1024

const int port = 8080;
const char *ip = "192.168.1.210";
 
long tv1, tv2,diff;

long sys_gettimeofday_us(void)
{
	struct  timeval tv;
    gettimeofday(&tv,NULL);
	return (tv.tv_sec*1000000 + tv.tv_usec);//us
}

void *sock_run(void *arg)
{
	char buf[SIZE];
	int f = (int)arg;
	long number = 0;
	while(1)
	{
		number++;
		if(number==1)
		{
		  tv1 = sys_gettimeofday_us();
		}
	        memset(buf,'\0',sizeof(buf));
		ssize_t _s=read(f,buf,sizeof(buf)-1);
		if(_s>0)
		{
			buf[_s]='\0';
			//write(f,buf,strlen(buf)-1);
			//printf("client:%s\n",buf);
		}
		else if(_s==0)
		{
			printf("read done\n");
			break;
		}else
			break;
	}
	tv2 = sys_gettimeofday_us();
	diff = tv2 - tv1;
	double speed = 1000*1000.0*number/diff;
	printf("diff:%ld us  speed:%lf/s v:%lfMb/s\n",diff,speed,speed*SIZE/1024/1024);
	exit(0);
}

int main()
{
	int listen_sock = socket(AF_INET,SOCK_STREAM,0);
	if(listen_sock < 0)
	{
		perror("listen error");
	}
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(ip);
	
	if(bind(listen_sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		perror("bind error\n");
	}
	if(listen(listen_sock,5)<0)
	{
		perror("listen error");
	}
	while(1)
	{
		struct sockaddr_in peer;
		socklen_t len = sizeof(peer);
		int fd = accept(listen_sock,(struct sockaddr*)&peer,&len);
		
		if(fd!=-1)
		{
			pthread_t id;
			int ret = pthread_create(&id,NULL,sock_run,(void*)fd);
			if(ret!=0)
			{
				perror("pthread create error");
			}
			pthread_detach(id);
		}
	}
	
	
	
	return 0;
}


