#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>
#include <assert.h>

#include "shmhandle.h"
//用来初始化信号量的初值的联合体
union semun
{
    int value;
};

//p操作
static void p(int id)
{
	struct sembuf sb[1];
	sb[0].sem_num = 0;
	sb[0].sem_op = -1;
	sb[0].sem_flg = 0;
	semop(id, sb, 1);
}
static void v(int id)
{
	struct sembuf sb[1];
	sb[0].sem_num = 0;
	sb[0].sem_op = 1;
	sb[0].sem_flg = 0;
	semop(id, sb, 1);
}
//初始化
shmfifo_t* shmfifo_init(int key,int blocks,int blksz)
{
    //向内存申请一块（共享内存结构体大小）的空间,用一个这种结构体类型指针保存其地址
    shmfifo_t *p=(shmfifo_t*)malloc(sizeof(shmfifo_t));
    //p->p_head = (head_t*)malloc(sizeof(head_t));
    //assert(p);
    memset(p,0x00,sizeof(shmfifo_t));
    //计算要创建的共享内存大小,记得加上头部信息结构体部分
    int len = blocks * blksz + sizeof(shmfifo_t);
    //打开len大小的共享内存key
    int shmid = shmget(key, 0, 0);
	printf("shmid1 = %d\n",shmid);
    if(shmid == -1)//共享内存不存在,则创建
    {
		shmid = shmget(key, len, IPC_CREAT|0644);
		printf("shmid2 = %d\n",shmid);
		if(shmid == -1)//创建失败
		{
		   perror("shmget failure\n"),exit(1);
		}else//创建成功，则初始化共享内存结构体，p指向这个结构体
			{
			    //读写位置初始化为0 
				p->p_head = (head_t*)shmat(shmid, NULL, 0);
				p->p_head->rd_idx=0;
				p->p_head->wr_idx=0;
			    //块数量、块大小由使用者传递过来
				p->p_head->blocks=blocks;
				p->p_head->blksz=blksz;
				printf("blocks:%d blksz:%d\n",blocks,blksz);
			    //指向真正内容的起始位置，即跳过头部信息
				p->p_payload=(char*)(p->p_head+1);
				p->shmid=shmid;
				p->sem_empty=semget(key,1,IPC_CREAT|0644);
				p->sem_full=semget(key+1,1,IPC_CREAT|0644);
				p->sem_mutex=semget(key+2,1,IPC_CREAT|0644);
			    //用来初始化信号量初值的联合体
				union semun su;
				semctl(p->sem_empty, 0, SETVAL, su);
				su.value=blocks;
				semctl(p->sem_full, 0, SETVAL, su);

			    //将sem_mutex信号量初始值设置为1（允许一个用户进来）
				su.value=1;
				semctl(p->sem_mutex, 0, SETVAL, su);
		    }
    }else//存在的话，将共享内存挂载并初始化部分数据
        {
	    /*       
			printf("blocks:%d blksz:%d",p->p_head->blocks,p->p_head->blksz);
			p->p_head=(head_t*)shmat(shmid,NULL,0);
	        //打开的话不用给信号量设定初值,由于是打开，后2个参数都是0
			p->sem_empty=semget(key+2,0,0);
	    */
			p->shmid = shmid;
			p->sem_empty = semget(key, 0, 0);
			p->sem_full = semget(key+1, 0, 0);
			p->sem_mutex = semget(key+2, 0, 0);
			p->p_head = (head_t*)shmat(p->shmid, NULL, 0);
			p->p_payload=(char*)p->p_head +sizeof(head_t);
			//printf("return ok\n");
        }
   return p;
}

//放数据
void shmfifo_put(shmfifo_t* fifo, const void* buf)
{
    //可装的资源减1
    p(fifo->sem_full);
    //保证只有一个操作
    p(fifo->sem_mutex);
    //放数据
    memcpy(fifo->p_payload+(fifo->p_head->wr_idx*fifo->p_head->blksz),buf,fifo->p_head->blksz);
    //写下标后移
    fifo->p_head->wr_idx=(fifo->p_head->wr_idx+1)%(fifo->p_head->blocks);
    //释放资源
    v(fifo->sem_mutex);
    v(fifo->sem_empty);
}

//取数据
void shmfifo_get(shmfifo_t* fifo, void* buf)
{
   p(fifo->sem_mutex);
   //可消费的资源减1
   p(fifo->sem_empty);
   //保证只有一个操作
   //取数据
   memcpy(buf,fifo->p_payload+(fifo->p_head->rd_idx*fifo->p_head->blksz),fifo->p_head->blksz);
   //写下标后移
   fifo->p_head->rd_idx=(fifo->p_head->rd_idx+1)%(fifo->p_head->blocks);
   //释放资源
   v(fifo->sem_full);
   v(fifo->sem_mutex);
}

//销毁共享内存
void shmfifo_destroy(shmfifo_t* fifo)
{
   //先卸载（即断开连接）
   shmdt(fifo->p_head);
   //销毁（即删除）
   shmctl(fifo->shmid, IPC_RMID, 0);
   semctl(fifo->sem_full, 0, IPC_RMID);
   semctl(fifo->sem_mutex, 0, IPC_RMID);
   semctl(fifo->sem_empty, 0, IPC_RMID);
   free(fifo);
}



/*
typedef struct
{
    int age;
    char name[12];
}person_t;
*/
//进程通信案例
/*
 int main()
{
     key_t key;
     key = ftok("/usr/etc/123.txt", 1);
   
    shmfifo_t *fifo = shmfifo_init(key,32,sizeof(person_t));
    person_t person;
    
    int i=0;
    for(i=0;i<10;i++)
{
    person.age=10+i;
    sprintf(person.name,"name:%d",i+1);
    shmfifo_put(fifo,&person);
    printf("put %d\n",i+1);
    }
}

*/
/*
int main()
{
    key_t key;
    key = ftok("/usr/etc/123.txt", 1);
   
    shmfifo_t *fifo = shmfifo_init(key,32,sizeof(person_t));
    person_t person;

   int i=0;
 for(i=0;i<10;i++)
{
    shmfifo_get(fifo,&person);
    printf("name=%s, age=%d\n", person.name, person.age);
   // sleep(1);
    }
    //shmfifo_destroy(fifo);
}
*/
