#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FuShengyuan");
MODULE_DESCRIPTION("deferred work");

/// @brief 存放work_struct与id信息
struct work_ctx
{
    struct work_struct work;
    int current_id;
};
struct delayed_work my_work;

struct work_ctx works[10];

struct task_struct *threads[10];
spinlock_t lock; 
/// @brief kthread执行体
/// @param data 传入参数
/// @return
int kthread_handler(void *data)
{
	//TODO Add Code here
    spin_lock(&lock);
    printk("kthread : %d",*(int*)data);
    kfree(data);
    spin_unlock(&lock);
    return 0;
}
/// @brief work queue执行体
/// @param work
void work_queue_handler(struct work_struct *work)
{

	//TODO Add Code here
    struct work_ctx* ctx=(struct work_ctx*)container_of(work,struct work_ctx, work); 
    spin_lock(&lock);
    printk("work queue : %d\n", ctx->current_id);
    spin_unlock(&lock);
}
/// @brief delayed work执行体
/// @param work 
void delayed_work_handler(struct work_struct *work)
{
    spin_lock(&lock);
    printk("delayed work!\n");
    spin_unlock(&lock);
}

/// @brief 内核模块初始化
/// @param
/// @return
int deferred_work_init(void)
{
    printk(KERN_INFO "deferred work module init\n");
    int i;
  	//TODO Add Code here,init workqueue and kernel thread
    INIT_DELAYED_WORK(&my_work,delayed_work_handler);
    schedule_delayed_work(&my_work,500);
    spin_lock_init(&lock);
  	for(i=0;i<10;i++)
    {
      works[i].current_id=i+599;
      INIT_WORK(&(works[i].work),work_queue_handler);
      if(schedule_work(&(works[i].work))==0)
       printk("work submit error\n");
    }

    for(i=0;i<10;i++)
    {
        int*num=(int*)kmalloc(sizeof(int));//不能使用局部变量，生命周期局限在本次迭代
        *num=i+599;        
        threads[i]=kthread_create(kthread_handler,num,"kthread");
        if(wake_up_process(threads[i])==0)
         printk("fail to wake up thread\n");
    }
    
      
     
    return 0;
}
/// @brief 内核模块退出
/// @param
void deferred_work_exit(void)
{

    int i=0;
	//TODO Add Code here
    for(i=0;i<10;i++)
    kthread_stop(threads[i]);
    printk(KERN_INFO "deferred work module exit\n");
}

module_init(deferred_work_init);
module_exit(deferred_work_exit);
