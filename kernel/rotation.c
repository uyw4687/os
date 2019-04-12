#include "../arch/arm64/include/asm/unistd.h"
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/rwlock.h>
#include <linux/rwlock_types.h>

#define READ 0
#define WRITE 1

int rotation;

rwlock_t rot_lock;
rwlock_t held_lock;
rwlock_t wait_lock;

struct list_head lock_queue;
struct list_head wait_queue;

int front = 0, rear = 0;
int is_initialized = 0; // if initialize() is called, set to 1

/* range descriptor */
struct rd {
    pid_t pid;      /* process id of requested process */
    int range[2];   /* acquire lock only if range[0] <= rotation <= range[1] */
    int type;      /* READ or WRITE */
    struct list_head list;
};

int compare_rd(struct rd *rd1, struct rd *rd2) {

	if((rd1->pid == rd2->pid) && ((rd1->range)[0] == (rd2->range)[0]) && ((rd1->range)[1] == (rd2->range)[1]) && (rd1->type == rd2->type))
		return 1; //true : same
	else
		return 0; //false : different
}

void set_lower_upper(int degree, int range, int *lower, int *upper) {
	
	*lower= degree-range;
	*upper = degree+range;

	if(*lower < 0)
		*lower = *lower + 360;
	
	if(*upper >= 360)
		*upper = *upper - 360;
	
}

int check_range(int rotation, struct rd* rd1){
    int lower = rd1->range[0];
    int upper = rd1->range[1];
    if(lower <= upper && lower <= rotation && rotation <= upper)
        return 1;
    else if(lower >= upper && (lower <= rotation || rotation <= upper))
        return 1;
    else return 0;
}//return 1 is rotation include rd range, 0 is rotation not include range

int my_enqueue(struct list_head *queue, struct rd* val) {
    if (val->pid == -1) {
        printk(KERN_ERR "invalid rd");
        return -1;
    }
    list_add_tail(&(val->list), queue);
    return 0;
}

void delete_lock(struct list_head *queue, int degree, int range, int type) {
	struct list_head *head;
	struct list_head *next_head;

	struct rd *lock_entry;

	int lower, upper;

	struct rd compare;

	set_lower_upper(degree, range, &lower, &upper);

	compare.pid = task_pid_nr(current);
	compare.range[0] = lower;
	compare.range[1] = upper;
	compare.type = type;
	
	list_for_each_safe(head, next_head, queue) {

		lock_entry = list_entry(head, struct rd, list);

		if(compare_rd(lock_entry, &compare)){

			list_del(head);

			kfree(lock_entry);

			break;

		}
	}
}

void remove_all(struct list_head *queue, pid_t pid) {
	
	struct list_head *head;
	struct list_head *next_head;

	struct rd *lock_entry;

	list_for_each_safe(head, next_head, queue) {
		lock_entry = list_entry(head, struct rd, list);
		if(lock_entry->pid == pid) {
			list_del(head);
			kfree(lock_entry);
		}
	}
}

struct rd* my_dequeue(struct list_head *queue) {

    struct rd* out;
    if (queue->next == queue) {
        printk(KERN_ERR "queue is empty");

        out = (struct rd*)kmalloc(sizeof(struct rd), GFP_KERNEL);
        out->pid = -1;  // empty rd
        INIT_LIST_HEAD(&(out->list));
        return out;
    }
    out = list_entry(queue->next, struct rd, list);
    list_del_init(queue->next);
    return out;
    // TODO must call kfree(out);
}

int check_input(int degree, int range) {

	if(degree < 0 || degree >= 360 || range <= 0 || range >=180) {
        printk(KERN_ERR "Out of range");
		return -1;
	}
	return 0;
}

void initialize_list(void) {
    // TODO concurrency, call timing
    if (is_initialized == 0) {
        is_initialized = 1;
        rwlock_init(&rot_lock);
        INIT_LIST_HEAD(&lock_queue);
        INIT_LIST_HEAD(&wait_queue);
    }
}

long sys_set_rotation(int degree) {
    if (degree < 0 || degree > 360) {
        printk(KERN_ERR "Out of range");
        return -1;
    }

    initialize_list();

    write_lock(&rot_lock);

    rotation = degree;

    write_unlock(&rot_lock);

    return 0;

}//TODO set queue stack that include rotation range.

void set_lock(struct rd* newlock, int degree, int range, int type) {

	int lower, upper; 

	set_lower_upper(degree, range, &lower, &upper);
	
	newlock->pid = task_pid_nr(current);
	newlock->range[0]= lower;
	newlock->range[1]= upper;
	newlock->type = type;
}

long sys_rotlock_read(int degree, int range){

	struct rd* newlock = (struct rd*)kmalloc(sizeof(struct rd), GFP_KERNEL);

	if(check_input(degree, range) < 0)
		return -1;

	set_lock(newlock, degree, range, READ);

    write_lock(&wait_lock);

	my_enqueue(&wait_queue, newlock);

    write_unlock(&wait_lock);

	//wait till getting the lock
	//...

    return -1;
}

long sys_rotlock_write(int degree, int range){

	struct rd* newlock = (struct rd*)kmalloc(sizeof(struct rd), GFP_KERNEL);

	if(check_input(degree, range) < 0)
		return -1;
	
	set_lock(newlock, degree, range, WRITE);
	
    write_lock(&wait_lock);

	my_enqueue(&wait_queue, newlock);

    write_unlock(&wait_lock);

	//wait till getting the lock
	//...

    return -1;
}

long sys_rotunlock_read(int degree, int range){

	if(check_input(degree, range) < 0)
		return -1;

    write_lock(&held_lock);

	delete_lock(&lock_queue, degree, range, READ);	

    write_unlock(&held_lock);

    return 0;
}

long sys_rotunlock_write(int degree, int range){

	if(check_input(degree, range) < 0)
		return -1;

	write_lock(&held_lock);
	
	delete_lock(&lock_queue, degree, range, WRITE);	

    write_unlock(&held_lock);
	
    return 0;
}

void exit_rotlock(struct task_struct *tsk){

    write_lock(&held_lock);
	remove_all(&lock_queue, tsk->pid);
    write_unlock(&held_lock);

    write_lock(&wait_lock);
	remove_all(&wait_queue, tsk->pid);
    write_lock(&wait_lock);
	//called with every thread exiting? or every process exiting?
}
/*
**********pid or tgid what to use in lock struct
*/
