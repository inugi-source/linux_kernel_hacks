/* **************** LDD:2.0 s_14/lab4_proc_sig_solB.c **************** */
/*
 * The code herein is: Copyright Jerry Cooperstein, 2012
 *
 * This Copyright is retained for the purpose of protecting free
 * redistribution of source.
 *
 *     URL:    http://www.coopj.com
 *     email:  coop@coopj.com
 *
 * The primary maintainer for this code is Jerry Cooperstein
 * The CONTRIBUTORS file (distributed with this
 * file) lists those known to have contributed to the source.
 *
 * This code is distributed under Version 2 of the GNU General Public
 * License, which you should have received with the source.
 *
 */
/*
 * Using proc to send signals.
 *
 * It is sometimes desirable to send a signal to an application from
 * within the kernel.  The function for doing this is:
 *
 * int send_sig(int signal, struct task_struct *tsk, int priv);
 *
 * where signal is the signal to send, tsk points to the task
 * structure corresponding to the process to which the signal should be
 * sent, and priv indicates the privilege level (0 for user applications,
 * 1 for the kernel.)
 *
 * Write a module that opens up two entries in the proc file system.

 *    When the first entry is written to, it sets the process ID of the
 *           process which is registered to receive signals via this mechanism.
 *    When the second entry is written to, it gets the signal to be
 *           delivered and then sends it.
 *    Reading either entry simply shows the current values of these
 *           parameters.
 *
 * Remember you'll have to use pid_task(find_vpid PIDTYPE_PID) to
 * connect the pid to the task structure it corresponds with. 
 @*/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sched/signal.h>

#define NODE_DIR "my_sig_dir"
#define NODE_1 "pid"
#define NODE_2 "signal"

static int sig_pid = -1, sig_tosend = SIGUSR1;
static struct task_struct *sig_tsk = NULL;
static struct proc_dir_entry *proc_sigdir, *proc_pid, *proc_signal;
static struct file_operations my_file_ops1 = {
	.owner   = THIS_MODULE,
};
static struct file_operations my_file_ops2 = {
	.owner   = THIS_MODULE,
};

ssize_t
my_proc_read1(struct file *file, char __user *buffer,
		size_t count, loff_t *ppos)
{
	int nbytes;

	if (count < 4096)
		return 0;
	nbytes = sprintf(buffer, "%d\n", sig_pid);
	return nbytes;
}

ssize_t
my_proc_read2(struct file *file, char __user *buffer,
		size_t count, loff_t *ppos)
{
	int nbytes;

	if (count < 4096)
		return 0;
	nbytes = sprintf(buffer, "%d\n", sig_tosend);
	return nbytes;
}

ssize_t
my_proc_write1(struct file *file, const char __user * buffer,
		size_t count, loff_t *data)
{
	char *str = kmalloc((size_t) count, GFP_KERNEL);

	/* copy the string from user-space to kernel-space */

	if (copy_from_user(str, buffer, count)) {
		kfree(str);
		return -EFAULT;
	}

	/* convert the string into a long */

	/*        sig_pid = simple_strtol (str, NULL, 10); */
	sscanf(str, "%d", &sig_pid);
	pr_info("sig_pid has been set to %d\n", sig_pid);
	/* sig_tsk = find_task_by_vpid (sig_pid); */
	sig_tsk = pid_task(find_vpid(sig_pid), PIDTYPE_PID);
	kfree(str);
	return count;
}

ssize_t
my_proc_write2(struct file *file, const char __user * buffer,
		size_t count, loff_t *data)
{
	int retval;
	char *str = kmalloc((size_t) count, GFP_KERNEL);

	/* copy the string from user-space to kernel-space */

	if (copy_from_user(str, buffer, count))
		return -EFAULT;

	/* convert the string into a long */

	/*        sig_tosend = simple_strtol (str, NULL, 10); */
	sscanf(str, "%d", &sig_tosend);
	pr_info("sig_tosend has been set to %d\n", sig_tosend);
	if (!sig_tsk) {
		pr_info("You haven't set the pid; using current\n");
		sig_tsk = current;
		sig_pid = (int)current->pid;
	}
	pr_info(" Sending signal %d to process ID %d\n", sig_tosend, sig_pid);
	retval = send_sig(sig_tosend, sig_tsk, 0);
	pr_info("retval = %d\n", retval);
	kfree(str);
	return count;
}

static int __init my_init(void)
{
	proc_sigdir = proc_mkdir(NODE_DIR, NULL);
	if (!proc_sigdir) {
		pr_err("I failed to make %s\n", NODE_DIR);
		return -1;
	}
	pr_info("I created %s\n", NODE_DIR);

	my_file_ops1.read = my_proc_read1;
	my_file_ops1.write = my_proc_write1;
	proc_pid = proc_create(NODE_1, S_IRUGO | S_IWUSR, proc_sigdir, &my_file_ops1);
	if (!proc_pid) {
		pr_err("I failed to make %s\n", NODE_1);
		remove_proc_entry(NODE_DIR, NULL);
		return -1;
	}
	pr_info("I created %s\n", NODE_1);

	my_file_ops2.read = my_proc_read2;
	my_file_ops2.write = my_proc_write2;
	proc_signal = proc_create(NODE_2, S_IRUGO | S_IWUSR, proc_sigdir, &my_file_ops2);
	if (!proc_signal) {
		pr_err("I failed to make %s\n", NODE_2);
		remove_proc_entry(NODE_1, proc_sigdir);
		remove_proc_entry(NODE_DIR, NULL);
		return -1;
	}
	pr_info("I created %s\n", NODE_2);
	return 0;
}

static void __exit my_exit(void)
{
	if (proc_pid) {
		remove_proc_entry(NODE_1, proc_sigdir);
		pr_info("Removed %s\n", NODE_1);
	}
	if (proc_signal) {
		remove_proc_entry(NODE_2, proc_sigdir);
		pr_info("Removed %s\n", NODE_2);
	}
	if (proc_sigdir) {
		remove_proc_entry(NODE_DIR, NULL);
		pr_info("Removed %s\n", NODE_DIR);
	}
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Jerry Cooperstein");
MODULE_AUTHOR("Chunghan Yi");
MODULE_DESCRIPTION("LDD:2.0 s_14/lab4_proc_sig_solB.c");
MODULE_LICENSE("GPL v2");
