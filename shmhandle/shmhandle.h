#ifndef __SHMHANDLE__
#define __SHMHANDLE__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <assert.h>
#include <sys/ipc.h>
#include <unistd.h>


#define USST_DATA_SIZE 3000
#define USST_DATAPAC_SIZE 721
 //头信息结构体
 typedef struct shm_head
{
   int blocks;//块的总数
   int blksz;//每块的大小
   int rd_idx;//读位置
   int wr_idx;//写位置
}head_t;

//共享内存头部结构体
typedef struct shmfifo
{
   head_t *p_head;//指向头信息结构体的指针
   char* p_payload;//装有效内容的起始地址
   int shmid;
   int sem_full;//可容纳信号量
   int sem_empty;//剩余信号量
   int sem_mutex;//互斥信号量
}shmfifo_t;



//初始化以上结构体的函数,
//返回类型为那块共享内存(共享内存结构体)的地址
//参数为：共享内存的key,要申请的块数，每块的大小
shmfifo_t* shmfifo_init(int key,int blocks,int blksz);
//往这块共享内存放数据
//参数为：申请的共享内存的地址，要放的数据的源地方
void shmfifo_put(shmfifo_t* fifo, const void* buf);
//从这块共享内存取数据
//参数为：要取的共享内存地址，取到的数据暂放的地方
void shmfifo_get(shmfifo_t* fifo, void* buf);
//销毁申请的这块共享内存
//参数为：要销毁的共享内存的地址
void shmfifo_destroy(shmfifo_t* fifo);



#endif