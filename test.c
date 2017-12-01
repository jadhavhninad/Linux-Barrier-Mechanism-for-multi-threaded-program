#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <fcntl.h> // open function
#include <errno.h>

#define TCOUNT_1 5
#define TCOUNT_2 20
#define AVG_SLEEP 1000 //Avg sleep in microseconds	
#define TIMEOUT 1000

int synch = 100;
pid_t pid_set[2];
extern int errno;
/* structure to pass information to the thread*/

struct thread_info
{
	unsigned int thread_barrier_id;
};

void *test1(void *arg)
{
	int ret, i=0;
	struct thread_info *thread_param ;
	thread_param = (struct thread_info *)arg;
        long int pid_v = syscall(SYS_getpid);
	long int tid_v = syscall(SYS_gettid);

	for( i=0; i<synch; i++){
		//pid_t process_id = getpid();
		//if (array_pid[0] == process_id)
			printf("%d sync : Thread -  pid is %ld , tid is %ld\n",i,pid_v,tid_v);
		//else if (array_pid[1] == process_id)
		//	printf("%d sync : Thread - pid is %ld  tid is %ld\n",i,pid_v, tid_v);
		
		//Call barrier wait
		ret = syscall(360, thread_param->thread_barrier_id);
		
		if(ret<0){
			printf("barrier_wait operation failed\n");
		}
		//if (array_pid[0] == process_id)
		//	printf("%d sync : Thread exiting : pid is %ld my thread id is %ld\n",i,pid_v,tid_v);
		//if (array_pid[1] == process_id)
			printf("%d sync : Thread exiting : pid is %ld my tid is %ld\n",i,pid_v,tid_v);
		
		
		//DO a random sleep
		int sleept = AVG_SLEEP + (-1+2*((float)rand())/RAND_MAX) * 100;
		usleep(sleept);
		}	
	pthread_exit(0);
}


void *test2(void *arg)
{
	int ret;
	int i=0;
	struct thread_info *thread_param;
	thread_param = (struct thread_info *)arg;
        long int pid_v = syscall(SYS_getpid);
        long int tid_v = syscall(SYS_gettid);
	//pid_t process_id = getpid(); 
	
	for( i= 0; i<synch; i++){
		//if (array_pid[0] == process_id)
			printf("%d sync : Thread : pid is %ld, tid is %ld\n",i+1,pid_v,tid_v);
		//else if (array_pid[1] == getpid())
		//	printf("%d sync : Thread : pid is %ld, tid is %ld\n",i+1,pid_v,tid_v);
		
		//Call barrier wait
		ret = syscall(360, thread_param->thread_barrier_id);
		if(ret<0){
			printf("barrier_wait operation failed\n");
		}
		
		//if (array_pid[0] == getpid())
			printf("%d sync : Thread exiting : pid is %ld tid is %ld\n",i+1,pid_v,tid_v);
		//if (array_pid[1] == getpid())
			//printf("%d sync : Thread exiting : pid is %ld tid is %ld\n",i+1,pid_v,tid_v);
		
		int sleept = AVG_SLEEP + (-1+2*((float)rand())/RAND_MAX) * 100;
		usleep(sleept);
		}	
		pthread_exit(0);
}


void Childprocess()
{
	unsigned int tc, bar1, bar2;
	int set;
	int i,j,tout;
	pid_t process_id;
	pthread_t tid[TCOUNT_1];
	pthread_t tid2[TCOUNT_2];
	process_id = getpid();    
	tout = TIMEOUT;

	printf("\nChild: pid = : %d\n", process_id);
	struct thread_info *thread_param= (struct thread_info *) malloc (sizeof(struct thread_info));
	unsigned int *id1= (unsigned int *) malloc (sizeof(unsigned int));
	unsigned int *id2= (unsigned int *) malloc (sizeof(unsigned int));
	tc = TCOUNT_1;
	
	//barrier_init for thread set 1
	if ((set = syscall(359,tc,id1,tout))==-1)
		printf("barrier_init failed\n");

	bar1 = *id1;
	for(i=0; i<TCOUNT_1; i++){
		thread_param->thread_barrier_id = bar1;
		pthread_create(&tid[i],NULL,test1,(void*)thread_param);
	}
	
	//barrier init for thread set 2
	tc = TCOUNT_2;
	if ((set =syscall(359,tc, id2,tout))==-1)    
		printf("barrier_init failed\n");
	
	bar2 = *id2;
	for(i=0; i<TCOUNT_2; i++){
		thread_param->thread_barrier_id = bar2;
		pthread_create(&tid2[i],NULL,test2,(void*)thread_param);
	}
	
	//wait for all threads to complete
	for(j=0;j<TCOUNT_1;j++){
		pthread_join(tid[j],NULL);
	}

	for(j=0;j<TCOUNT_2;j++){
		pthread_join(tid2[j],NULL);
	}
	
	sleep(1);
	//Destroy all barriers
	if ((set =syscall(361,bar1))==-1)
		printf("Failed delete of barrier id= %d \n",bar1);
																	
	if ((set =syscall(361,bar2))==-1)    		
		printf("Failed delete of barrier id= %d \n",bar2 );
	
	free(thread_param);
	free(id1);
	free(id2);
  	_exit(1);
}

int main( int argc, char *argv[])
{
	pid_t ppid = getpid();
	printf("Parent: my pid is: %d\n", ppid);
	pid_t child_pid1 , child_pid2;
	child_pid1 = fork();		
	
	if(child_pid1  < 0 ){
		printf("fork 1 failed");
		exit(1);
	}
	else if (child_pid1 == 0){
		//get pid of child1
		pid_set[0] = getpid();
		Childprocess();
	}
	
	else if (child_pid1 > 0) {
		//we got the parent process id. fork another process
		child_pid2 = fork();
		if(child_pid2  < 0 ){	
			printf("fork 2 failed");
			exit(1);
		}	
	
		else if(child_pid2 == 0){ 
			//get pid of child2
			pid_set[1] = getpid();
			Childprocess();
		}
	
		else if( child_pid2 > 0) {
			wait(0);
		}
		wait(0);
	}
	printf("Parent: Exited child process\n");
  	return 0;
}
