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
/*请在这里添加程序注释
实现多进程拷贝
本demon采用的是文件读写的方式，另一个demon中采用的是映射的方式
*/

	int start[10];
	int copy_size = 0;    //copy大小   //大小和文件路径都可以用全局变量代替
	char filePath_target[50];   //目标文件路径    
	char filePath_source[50];   //源文件路径

	int Report_size=0;  //一次写入de大小：为了适应进度条
	int Writed_size[10]= {0};  //当前各线程已写入大小，定义成数组防止多线程读取同一地址数据冲突
	//进度条线程函数
	void * Report_jobs(void * data)
	{
		float size = *(float *)data;
		while(1)
		{
		int All_writed_size = 0;
		for(int i = 0; i< 10;i++)  //统计所有线程一共写入了多少数据
		{
			All_writed_size += Writed_size[i];
		}

		printf("当前进度: %3.2f %   \n",(All_writed_size / size) * 100);
		usleep(50000);  //50ms打印一次进度
		printf("\033[A"); //将当前光标上移
		printf("\033[K");  //删除光标后面到内容
		}
	
	}

void * thread_jobs(void * data)   //拷贝线程函数
{
	int start =*(int*)data;  //线程编号
	int start_addr =start * copy_size;  //当前线程复制起点
	int sfd, dfd;
	sfd = open(filePath_source,O_RDONLY);   //打开源文件
	dfd = open(filePath_target, O_WRONLY | O_CREAT, 0664);
	if(-1 == dfd | -1 == sfd)
	{
		perror("目标/源文件打开/创建失败");
		exit(0);   //全部退出
	}
	lseek(sfd,start_addr, SEEK_SET);   //将源文件光标置于起始地址处
	lseek(dfd, start_addr, SEEK_SET); 

	char buffer[copy_size];
	bzero(buffer,copy_size);
	int recvlen = read(sfd, buffer,copy_size);
	int i =0;
	while((recvlen -= Report_size) >= 0)
	{
	//`	printf("s\n");	
		int size = write(dfd,buffer + i,Report_size);
		Writed_size[start] += size;  //保存写入的数据数量
		//需要手动偏移光标!!!!	
		start_addr +=size;
	//	printf("%d\n",start_addr);
		lseek(sfd,start_addr, SEEK_SET);  
		lseek(dfd, start_addr, SEEK_SET);
		i+= size; 
	}
	if((Report_size +recvlen)>0)
	{
	int size = write(dfd,buffer+i,Report_size + recvlen);  //此时recvlen是负数  
	Writed_size[start] += size;  //保存写入到数据数量
	}
	close(sfd);
	close(dfd);
	return 0;


}


int main(int argc, char ** argv  )   //参数至少为3个：命令/源地址/目标地址
{
	if(argc != 3)
	{
		printf("参数错误，清重新输入！\n");
		exit(0);
	}
	int fd;
	if((fd = open(argv[1], O_RDONLY)) == -1 )
	{
		perror("打开源文件失败");
		exit(0);
	}
	strcpy(filePath_source,argv[1]);
	strcpy(filePath_target,argv[2]);
	int fsize;
	fsize = lseek(fd, 0 ,SEEK_END);   //将光标移动到文件末尾，返回值为文件大小
	if(fsize/10000 != 0)  //精度为 0.01%
		{
			Report_size =fsize/10000;
		}
		else
		{
			Report_size = 1;
		}
	if(fsize % 10 == 0)  //线程数为10
	{
		copy_size = fsize / 10;
	}
	else{
		copy_size = fsize / 10 + 1;
	}
	
	pthread_t tid[10];
	//创建进度条线程
	float size = fsize;
	int err1;
	pthread_t Report_tid;
		if( (err1 = pthread_create(&Report_tid, NULL, Report_jobs,(void*)&size)) > 0 )
		{
			printf("create Report thread failed: %s \n",strerror(err1));
			exit(0);
		}
	//循环创建10个线程
	for(int i = 0; i < 10; i++)
	{
		int err;
		 start[i] = i;
		if( (err = pthread_create(&tid[i], NULL, thread_jobs,(void*)&start[i])) > 0 )
		{
			printf("create thread failed: %s \n",strerror(err));
			exit(0);
		}


	}

	for(int i = 0; i<10;i++)   //阻塞回收10个线程
	{
		int err;
		if( (err = pthread_join(tid[i], NULL))> 0)
		{
			printf("线程 0x%x 回收失败: %s \n",(unsigned int)tid[i], strerror(err));

		}
	}
	pthread_cancel(Report_tid);  //结束目标线程（目标线程有系统调用，可以结束）
	pthread_join(Report_tid,NULL);
	printf("\033[A");
	printf("\033[K");
	printf("当前进度：100 %\n");
	//lseek(STDOUT_FILENO,-3,SEEK_END);
	//printf("hhh\n");
	return 0;
}
