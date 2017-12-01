#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include<linux/init.h>
#include<linux/moduleparam.h>

struct my_barrier{
	unsigned int barrier_id, tcount, max_tcount;
	int timeout;
	//int m_count;
	//unsigned int m_barrier_id;
	spinlock_t barrier_lock; 
	pid_t barrier_process_id;
	//unsigned int m_num_of_threads;
	wait_queue_head_t thread_queue;
	struct list_head barrier_head;
											
};

static LIST_HEAD(barrier_list);
static bool wait_flag = 0;
static int wait_count =0;
static int timer_val=2;
static struct hrtimer hr_timer;
ktime_t ktime;

enum hrtimer_restart timer_handler( struct hrtimer *timer )
{
	        printk(KERN_INFO "Timer hit \n");
		        return HRTIMER_NORESTART;
}

asmlinkage long sys_barrier_init(unsigned int count, unsigned int *barrier_id, unsigned int timeout)
{	
	struct my_barrier *new_barrier;
	struct my_barrier *barrier_walker;
	struct my_barrier *last_same_barrier;
	int found = 0;
	pid_t current_tgid;
	
	new_barrier = kzalloc(sizeof(struct my_barrier), GFP_KERNEL);
	last_same_barrier = kzalloc(sizeof(struct my_barrier), GFP_KERNEL);
	if (!new_barrier) {
		printk("Bad pbarrier Kmalloc\n");
	}
	
	//memset(pbarrier, 0, sizeof( barrier_struct));
	 current_tgid = task_tgid_vnr(current);
	
	//Using list_empty_careful to handle SMP scenarios
	if(list_empty_careful(&barrier_list)==0){
		
		//Check if the process that created  new barrier already has a barriers created.
		list_for_each_entry(barrier_walker, &barrier_list, barrier_head){
			if(barrier_walker->barrier_process_id == current_tgid){	
				last_same_barrier->barrier_id = barrier_walker->barrier_id;
				found = 1;
			}																	}
	}
	
	if(found == 1)
	        new_barrier->barrier_id = last_same_barrier->barrier_id + 1;
	else	
		new_barrier->barrier_id = 1;
	
	new_barrier->max_tcount = count;
	new_barrier->timeout = timeout;
	new_barrier->tcount = 0;
	new_barrier->barrier_process_id = current_tgid;
	init_waitqueue_head(&new_barrier->thread_queue);
	spin_lock_init(&new_barrier->barrier_lock);
	INIT_LIST_HEAD(&new_barrier->barrier_head) ;
	list_add(&new_barrier->barrier_head, &barrier_list );
	
	//printk("The barrier id created for tgid =  %d, pid = %d is %d\n",current_tgid, task_pid_nr(current), new_barrier->barrier_id);
	*barrier_id = new_barrier->barrier_id ;
	return 0;
}

asmlinkage long sys_barrier_wait(unsigned int barrier_id){
	
	struct my_barrier *barrier_walker;
	int found = 0;		    
	unsigned long nsecs_val;
	//int barrierID;
	//barrierID = barrier_id;
	pid_t current_tgid ;
	current_tgid = task_tgid_vnr(current);
	
	//Search for the barrier that made the system call.
	list_for_each_entry(barrier_walker, &barrier_list, barrier_head){
		if(barrier_walker->barrier_process_id == current_tgid && barrier_walker->barrier_id == barrier_id){
			found = 1;
			break;
		}
	}
	
	if(found == 0)
		return -EINVAL;
	
       	nsecs_val = barrier_walker->timeout;
      	spin_lock(&barrier_walker->barrier_lock);
	
	//0 or -ve value of timeout means no timeout.
	if(nsecs_val > 0 && barrier_walker->tcount == 0){
		//Initilize the timer here
		ktime = ktime_set( 0, nsecs_val);
		hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		hr_timer.function = &timer_handler;
		timer_val = hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );
	}
	
	//if timeout then wakeup all threads.
	/*if(timer_val == 0){
		wake_up_all(barrier_walker->thread_queue);
		barrier_walker->count=0;
		timer_val=2;
		wait_flag=1;
		spin_unlock(barrier_walker->barrier_lock);
	}*/
	else if(barrier_walker->tcount+1 < barrier_walker->max_tcount || timer_val>0){
		barrier_walker->tcount++;
		//printk("tgid = %d, pid = %d enqueued\n", task_tgid_vnr(current), task_pid_nr(current));
		wait_count++;
		spin_unlock(&barrier_walker->barrier_lock);	
		
		//Incase the count is reached its limit before this is called
		wait_event_interruptible(barrier_walker->thread_queue,wait_flag);  
		//printk(KERN_INFO "tgid = %d, pid = %d WOKEN UP\n",task_tgid_vnr(current), task_pid_nr(current));
		
		spin_lock(&barrier_walker->barrier_lock);
		if(wait_count == 0){
			wait_flag = 0;
		}
		else{
			wait_count--;
			spin_unlock(&barrier_walker->barrier_lock);
		}
	}
	else{
		//Reset the barrier parameters, cancel the timer.
		wait_flag =1;
		wake_up_all(&barrier_walker->thread_queue);
		barrier_walker->tcount = 0;
		timer_val=2;
		hrtimer_cancel(&hr_timer);
		spin_unlock(&barrier_walker->barrier_lock);
	}
	return 0;
}

asmlinkage long sys_barrier_destroy(unsigned int barrier_id)
{
	int found = 0;
	struct list_head *pos, *q;
	struct my_barrier *barrier_walker; 
	pid_t current_tgid;
	if(wait_count != 0)
		return -EBUSY;
	
	current_tgid = task_tgid_vnr(current);
	
	//Safely delete the barrier
	list_for_each_safe(pos, q ,&barrier_list){
		barrier_walker= list_entry(pos, struct my_barrier, barrier_head);
		if(barrier_walker->barrier_id == barrier_id && barrier_walker->barrier_process_id == current_tgid){
			found = 1;
			//printk("The barrier id %d with tgid %d is destroyed \n",del_pbarrier->m_barrier_id,task_tgid_vnr(current));
			list_del(pos);
			kfree(barrier_walker);
			break;
		}
	}
	
	if(found == 0)
		return -EINVAL;
	
	return 0;	
}
