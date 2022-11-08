#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/fcntl.h>
#include<pthread.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/mman.h>
/*请在这里添加程序注释
本demon中采用到是映射到方式实现的，另一个demon使用的文件读写的方式*/

#define THREAD_NO 10

char *ptr = NULL;
int blocksize;
int filesize;   //文件总大小
char filePath_source[50];
char filePath_target[50];


int create_map(void)
{
	int fd; 
	if((fd = open(filePath_source, O_RDONLY)) == -1)
	{
		perror("Opne file failed:");
		exit(0);
	}
	int fsize = lseek(fd, 0, SEEK_END);
	ptr = mmap(NULL,fsize,PROT_READ,MAP_PRIVATE,fd,0);
	return fsize;
}

void * thread_copy(void * arg)
{
	int i = *(int *)arg;
	int POS = i* blocksize;
	if(POS >= filesize )
	{
		pthread_exit(NULL);   //如果拷贝地址超过文件大小，退出本线程
	}
	printf("Copy Thread 0x%x , i = %d POS = %d, BLOCKSIZE = %d\n", (unsigned int)pthread_self(),i,POS,blocksize);
	char buffer[blocksize];
	bzero(buffer,blocksize);
	
	//	snprintf(buffer,blocksize+1,"%s",ptr+POS);    //会丢失数据
	memcpy(buffer,ptr + POS,blocksize);    //不会丢失数据
	//打开目标文件，如果不存在则创建
	int dest_fd = open(filePath_target,O_RDWR|O_CREAT,0664);
	lseek(dest_fd,POS,SEEK_SET);
	//判断是不是最后一个拷贝线程
	if(i == 9)
	{

	//如果是最后一个线程
	int bsize = filesize - POS;   //文件剩余大小,不会为负，如果为负数上面判断是否超出文件范围时就会退出线程
	//写入拷贝数据
	write(dest_fd, buffer, bsize);   //此处注意不要直接改变clocksize的直，因为最后一个线程并不一定就是最后执行或者最后执行完
	}
	else
	{
		write(dest_fd, buffer, blocksize);
	}
	close(dest_fd);
	pthread_exit(NULL);

}


int thread_cur()
{
	int fd = open(filePath_source,O_RDONLY);
	int Fsize = lseek(fd,0,SEEK_END);
	printf("源文件大小为：%d\n",Fsize);
	if(Fsize % THREAD_NO == 0)
		return Fsize / THREAD_NO;
	else
		return Fsize / THREAD_NO + 1;
}

int thread_create(void)
{
	pthread_t tids[THREAD_NO];
	int i=0;
	int num[THREAD_NO];
	for(i = 0; i<THREAD_NO;i++)
	{
		num[i] = i;   //用数组来给线程传值，这样各线程参数地址都不一样，不会互相影响。
		pthread_create(&tids[i], NULL, thread_copy, (void *)&num[i]);
		
	}

	while(i--){
		pthread_join(tids[i],NULL);  //阻塞回收
	}
	return 0;
}

int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		printf("参数错误，请重新输入:格式为：命令+源文件地址+目标文件地址\n");
		exit(0);
	}
	strcpy(filePath_source,argv[1]);
	strcpy(filePath_target,argv[2]);
	int Fsize;
	Fsize = create_map();  //将拷贝数据加载到映射内存，返回数据为文件大小
	filesize = Fsize;
	blocksize = thread_cur();   //获取每个线程应该拷贝的大小
	printf("%d\n",blocksize);
	thread_create();
	munmap(ptr,Fsize);     //释放资源
	printf("Copy Success!\n");
	return 0;


}
