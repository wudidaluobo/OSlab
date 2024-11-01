#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched/signal.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

MODULE_AUTHOR("FuShengyuan");

MODULE_DESCRIPTION("A simple kernel module with a list and two threads.");

static struct list_head my_list;
static struct task_struct *thread1, *thread2;

spinlock_t lock;

// 定义链表节点结构
struct pid_node
{
    int pid;
    char comm[16];
    struct list_head list;
};



// thread1 函数体
static int thread1_func(void *data)
{

//TODO: add code here
    
    struct pid_node* node; 
      
    //使用 for_each_process 宏遍历所有进程 
    struct task_struct *p;
    
    for_each_process(p) { 
     //初始化对应的节点
     node=(struct pid_node*)kmalloc(sizeof(struct pid_node),GFP_KERNEL);
     node->pid=p->pid;
     strcpy(node->comm,p->comm);
     //printk("PID: %d, Command: %s\n", p->pid, p->comm); 
     spin_lock(&lock);
     list_add_tail(&(node->list), &my_list);
     spin_unlock(&lock);   
    }  
      
    return 0;
}
// thread2 函数体
static int thread2_func(void *data)
{
    
//TODO: add code here
    struct list_head *pos,*n;
    spin_lock(&lock);
    list_for_each_safe(pos,n,&my_list) { 
    //下一个节点等于头节点，休眠一段时间
    if(n==&mylist)
    {
        spin_unlock(&lock);
        msleep_interruptible(10);
        spin_lock(&lock);
    }

    //获取对应节点 
    struct pid_node* node = list_entry(pos, struct pid_node, list); 
        
    printk("PID: %d, Command: %s\n",node->pid,node->comm); 
   } 
    spin_unlock(&lock); 
    return 0;
}

// 模块初始化函数
int kernel_module_init(void)
{
    printk(KERN_INFO "List and thread module init\n");

//TODO: add code here
    //初始链表头
    INIT_LIST_HEAD(&my_list);
    //初始化锁
    spin_lock_init(&lock);
    //初始化线程
    thread1=kthread_create(thread1_func, NULL, "thread1");
    thread2=kthread_create(thread2_func, NULL, "thread2");
    if(wake_up_process(thread1)==0)
      printk("thread1 init error");
    if(wake_up_process(thread2)==0)
      printk("thread2 init error");



    return 0;
}

// 模块清理函数
void kernel_module_exit(void)
{

   //TODO: add code here
    // 停止线程1
    kthread_stop(thread1);
    // 停止线程2
    kthread_stop(thread2);
    // 清理链表
    struct list_head *pos,*n;
    list_for_each_safe(pos,n,&my_list) { 
    //获取对应节点 
    struct pid_node* node = list_entry(pos, struct pid_node, list); 
    list_del(pos);
    kfree(node); 
   } 
    printk(KERN_INFO "List and thread module exit\n");
}

module_init(kernel_module_init);

module_exit(kernel_module_exit);