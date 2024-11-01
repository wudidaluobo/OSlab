#include <asm/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/pid.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/types.h>

// 保存edu设备信息
struct edu_dev_info
{
	resource_size_t io;
	long range, flags;
	void __iomem *ioaddr;
	int irq;
};

static struct pci_device_id id_table[] = {
	{PCI_DEVICE(0x1234, 0x11e8)}, // edu设备id
	{
		0,
	} // 最后一组是0，表示结束
};

struct edu_dev_info *edu_info;
spinlock_t lock;

/// @brief edu设备发现函数
/// @param dev 
/// @param id 
/// @return 
static int edu_driver_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	//TODO:请完成edu设备发现函数
	int ret = 0;
	printk("executing edu driver probe function!\n");

	ret = pci_enable_device(dev);
	if (ret)
	{
		printk(KERN_ERR "IO Error.\n");
		return -EIO;
	}

	//TODO: allocate memory for an instance of edu_dev_info& set irq data member in this instance

    edu_info=(struct edu_dev_info*)kmalloc(sizeof(struct edu_dev_info),GFP_KERNEL);
	ret = pci_request_regions(dev, "edu_dirver"); // 申请一块驱动掌管的内存空间
	if (ret)
	{
		printk("PCI request regions err!\n");
		goto out_mypci;
	}

	//TODO: 将写入BAR的总线地址映射到系统内存的虚拟地址

    //映射bar0
    if(pci_ioremap_bar(dev, 0)==NULL)
	 printk("map error");
	pci_set_drvdata(dev, edu_info); // 设置驱动私有数据
	printk("Probe succeeds.PCIE ioport addr start at %llX, edu_info->ioaddr is 0x%p.\n", edu_info->io, edu_info->ioaddr);

	return 0;

out_iounmap:
	iounmap(edu_info->ioaddr);
out_regions:
	pci_release_regions(dev);
out_mypci:
	kfree(edu_info);
	return ret;
}

/// @brief edu设备移除函数
/// @param dev 
static void edu_driver_remove(struct pci_dev *dev)
{
	//TODO:请完成edu设备删除函数
    	
	//TODO:acquired the address of edu_dev_info instance
	iounmap(edu_info->ioaddr);
	pci_release_regions(dev);
	//TODO:free the instance of edu_dev_info
	pci_disable_device(dev);
	kfree(edu_info);
	printk("Device is removed successfully.\n");
}

MODULE_DEVICE_TABLE(pci, id_table); // 暴露驱动能发现的设备ID表单

static struct pci_driver pci_driver = {
	.name = "edu_dirver",
	.id_table = id_table,
	.probe = edu_driver_probe,
	.remove = edu_driver_remove,
};

// =============================================================================== //

#define EDU_DEV_MAJOR 200  /* 主设备号 */
#define EDU_DEV_NAME "edu" /* 设备名 */


int current_id = 0;

struct user_data
{
	int id;
	atomic_t data;
};

struct thread_data
{
	struct user_data* user_data_ptr;
	int input_data;
};

//先设为全局变量
struct task_struct*threads[10];
int count=0;//线程的使用数

int kthread_handler(void *data)
{
	// TODO: implment the kernel thread
	struct thread_data* thread_data_ptr = (struct thread_data*)data;
	//输入值
	uint64_t value = thread_data_ptr->input_data;
	printk("ioctl cmd 0 : factorial\n");

	//TODO:将用户传入的变量交给edv设备进行阶乘计算，并读出结果，注意加锁。此外，结果放入user_data中的data数据成员中时，需要确保读写原子性。
	spin_lock(&lock);
	writel(value,edu_info->ioaddr);
	int ret=readl(edu_info->ioaddr);
	printk("kthread ret:%d\n",ret);
    struct user_data*user_data_ptr=thread_data_ptr->user_data_ptr;
	atomic_set(&user_data_ptr->data,ret);
	spin_unlock(&lock);
	return 0;
}



/// @brief open处理函数
/// @param inode 
/// @param filp 
/// @return 
static int edu_dev_open(struct inode *inode, struct file *filp)
{
	//TODO:请完成edu设备驱动open处理函数
	
	struct user_data* user_data_ptr = (struct user_data*)kmalloc(sizeof(struct user_data), GFP_KERNEL);
	user_data_ptr->id = current_id++;//这里应该也加一个锁
    //调试点
	printk("current_id:%d\n",current_id);
	//TODO: 设置filp->private_data
	filp->private_data=user_data_ptr;//相当于将用户的数据加载到设备中
	return 0;
}


/// @brief close处理函数
/// @param inode 
/// @param filp 
/// @return 
static int edu_dev_release(struct inode *inode, struct file *filp)
{
	//TODO:请完成edu设备驱动release处理函数，注意释放open中分配的内存
	struct user_data* user_data_ptr=filp->private_data;
	kfree(user_data_ptr);
	return 0;
}


/// @brief ioctl处理函数
/// @param pfilp_t 
/// @param cmd 
/// @param arg 
/// @return 
long edu_dev_unlocked_ioctl(struct file *pfilp_t, unsigned int cmd, unsigned long arg)
{
	//TODO:请完成edu设备ioctl处理函数，用户通过ioctl传入要计算阶乘的数值，并读取最后阶乘的结果。计算阶乘使用内核线程，线程代码放在kthread_handler中
    //cmd==0时，为写操作
	//调试点
	printk("cmd:%d arg:%ld\n",cmd,arg);

	if(cmd==0)
	{
	  //将用户数据和输入数写入thread_data
      struct thread_data* thread_data_ptr=(struct thread_data*)kmalloc(sizeof(struct thread_data),GFP_KERNEL);
	  thread_data_ptr->user_data_ptr=pfilp_t->private_data;
	  thread_data_ptr->input_data=(int)arg;
	  //建立线程

	  printk("cmd==0\n");
      struct task_struct*thread=kthread_create(kthread_handler, thread_data_ptr, "kthread");
      wake_up_process(thread);
	  //结束线程，销毁线程数据
	  //sleep(1);
	  //kthread_stop(thread);
	  kfree(thread_data_ptr);
      return 0;
	}
	//cmd为1，读操作
	else
	{
		struct user_data* user_data_ptr=pfilp_t->private_data;
		
		long ret=(long)atomic_read(&user_data_ptr->data);
		//调试点
		printk("ret:%ld\n",ret);
		return ret;
	}


	
}


static struct file_operations edu_dev_fops = {
	.open = edu_dev_open,
	.release = edu_dev_release,
	.unlocked_ioctl = edu_dev_unlocked_ioctl,
};
/// @brief 驱动程序初始化
/// @param  
/// @return 
static int __init edu_dirver_init(void)
{
	printk("HELLO PCI\n");
	int ret = 0;
	// 注册字符设备,这一步之后可以进行相应文件操作
	ret = register_chrdev(EDU_DEV_MAJOR, EDU_DEV_NAME, &edu_dev_fops);
	if (0 > ret)
	{
		printk("kernel edu dev register_chrdev failure\n");
		return -1;
	}
	printk("chrdev edu dev is insmod, major_dev is 200\n");
	// 注册edu pci设备
	ret = pci_register_driver(&pci_driver);
	if (ret)
	{
		printk("kernel edu dev pci_register_driver failure\n");
		return ret;
	}
	// 初始化自旋锁
    spin_lock_init(&lock);
	return 0;
}
/// @brief 驱动程序注销
/// @param  
/// @return 
static void __exit edu_dirver_exit(void)
{
	int i;
	// 注销字符设备
	unregister_chrdev(EDU_DEV_MAJOR, EDU_DEV_NAME);
	// 注销edu pci设备
	pci_unregister_driver(&pci_driver);
	//把未停止的线程停止
	for(i=0;i<10;i++)
	{
		if(threads[i]!=NULL)
		{
           kthread_stop(threads[i]);
		}
	}
	printk("GOODBYE PCI\n");
}

MODULE_LICENSE("GPL");

module_init(edu_dirver_init);
module_exit(edu_dirver_exit);