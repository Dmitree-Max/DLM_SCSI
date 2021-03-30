#include "kv.h"


char kv_device_buffer[KV_BUFFER_SIZE];

char* this_machine_id = "default_server";



struct key_node* key_head = NULL;
struct key_node* key_tail = NULL;

struct lock_node* lock_head = NULL;
struct lock_node* lock_tail = NULL;


struct workqueue_struct* workqueue = NULL;

int status;
dlm_lockspace_t *ls = NULL;

module_param(this_machine_id, charp, 0);


int node_amount = 2;
char ** node_list = NULL;
int this_node_id;
struct dlm_block** sys_locks;


struct work_struct first_bast;
struct work_struct second_bast;


static void print_ast(void* value);
static void print_astarg_key_value(void* value);
static struct dlm_block* get_pre_block(int node_id);
static struct dlm_block* get_post_block(int node_id);
static void pre_bast_initial(void* arg, int mode);



static void dlm_wait_ast(void *astarg)
{
	struct dlm_block *dlm_block = astarg;

	complete(dlm_block->compl);
}


static int sync_dlm_lock(int mode,
	      struct dlm_block *block, int flags,
	      const char *name, void (*bast)(void *, int))
{
	int res;

	init_completion(block->compl);

	res = dlm_lock(ls, mode, block->lksb, flags,
			    (void *)name, name ? strlen(name) : 0, 0,
			    dlm_wait_ast, block, bast);
	if (res < 0)
	    printk("kv : sync_dlm_lock : Locking error: %i name: %s\n", res, name);

	res = wait_for_completion_timeout(block->compl, 60 * HZ);
	if (res > 0)
		res = block->lksb->sb_status;
	else if (res == 0)
		res = -ETIMEDOUT;
	if (res < 0) {
	    printk("kv : sync_dlm_lock : Locking error: %i name: %s\n", res, name);
	}

	return res;
}


static void print_lksb(struct dlm_lksb* lksb)
{
	printk("kv lksb : status: %d, id: %i, flags: %d, lvb : %s\n",
			lksb->sb_status, lksb->sb_lkid, lksb->sb_flags, lksb->sb_lvbptr);
}


static void print_all_sys_blocks(void)
{
	int nodeid = 0;
	struct dlm_block* cur;
	while (nodeid < node_amount * 2)
	{
		cur = sys_locks[nodeid];
		printk("dlm : print_all_blocks: name: %s, value: %s, lockid: %i, status: %d\n",
				cur->name, cur->lksb->sb_lvbptr,
				cur->lksb->sb_lkid, cur->lksb->sb_status);

		nodeid ++;
	}

}

int get_values(char *buff, size_t length, loff_t *ppos)
{
	int size_in_min_buffer_left, bytes_written, offset;

	size_in_min_buffer_left = MIN(length, KV_BUFFER_SIZE);
	offset = *ppos;

	bytes_written = write_header(kv_device_buffer, size_in_min_buffer_left, &offset, "keys:\n");
	size_in_min_buffer_left -= bytes_written;

	if (size_in_min_buffer_left > 0)
	{
	    bytes_written += fill_buffer_with_keys(kv_device_buffer + bytes_written, size_in_min_buffer_left, &offset);
		size_in_min_buffer_left -= bytes_written;
	}

	if (size_in_min_buffer_left > 0)
	{
	    bytes_written += write_header(kv_device_buffer + bytes_written, size_in_min_buffer_left, &offset, "\nlocks:\n");
		size_in_min_buffer_left -= bytes_written;
	}

	if (size_in_min_buffer_left > 0)
	{
	    bytes_written += fill_buffer_with_locks(kv_device_buffer + bytes_written, size_in_min_buffer_left, &offset);
		size_in_min_buffer_left -= bytes_written;
	}


	memcpy(buff, kv_device_buffer, bytes_written);

	return bytes_written;
}




static int init_ls(void)
{
	int status;
	status = dlm_new_lockspace("new_ls170", "mycluster", DLM_LSFL_FS,
		      PR_DLM_LVB_LEN, NULL, NULL, NULL, &ls);

	printk("kv : init_ls: sleeping for 3 sec\n");

	msleep(2000);
	printk("kv : init_ls: awake\n");
	printk("kv : init_ls: ls_create_status = %i\n", status);
	return status;

}


static int process_update(struct update_structure update)
{
	int error ;

	printk("kv : process_update : update type %i, key: %s, value: %s\n",
			update.type, update.key, update.value);

	switch (update.type)
	{
		case 1:
			error =	update_or_add_key(update.key, update.value);
			printk("kv : process_update: ins or up %s = %s\n", update.key, update.value);
			break;
		case 3:
			error =	insert_lock(create_lock(update.key, this_machine_id));
			printk("kv : process_update: locked %s\n", update.key);
			break;
		case 4:
			error =	remove_lock(update.key);
			printk("kv : process_update: locked %s\n", update.key);
			break;
	}

	if (error) {
		printk("kv : process_update: update fail: type: %i error: %i\n", update.type, error);
	}

	return error;
}

static void post_data_getter_bast(void* arg, int mode)
{
	printk("kv : post_data_getter_bast: start work %i\n");
	queue_work(workqueue, &second_bast);
}


static void post_data_getter_work(struct work_struct *work)
{
	struct update_structure update;

	int error = 0;

	struct dlm_block* post_block = get_post_block(this_node_id);
	struct dlm_block* pre_block  = get_pre_block(this_node_id);


	error = sync_dlm_lock(DLM_LOCK_EX, pre_block, DLM_LKF_CONVERT | DLM_LKF_VALBLK,
			pre_block->name, pre_bast_initial);


	if (error) {
		printk("kv : post_data_getter_work: pre convert failed: %i\n",  error);
		goto out;
	}

	printk("kv : post_data_getter_work: pre convert suc\n");

	printk("kv : post_data_getter_work : buffer: %s\n", pre_block->lksb->sb_lvbptr);

	update_from_buffer(&update, pre_block->lksb->sb_lvbptr);

	printk("kv : post_data_getter_work : update type %i, key: %s, value: %s\n",
			update.type, update.key, update.value);

	error = process_update(update);

	if (error != 0)
	{
		printk("kv : post_data_getter_work : process update error: %i\n", error);
	}

	error = sync_dlm_lock(DLM_LOCK_NL, post_block, DLM_LKF_CONVERT,
				post_block->name, NULL);


	if (error) {
		printk("kv : post_data_getter_work: post convert failed: %i\n",  error);
		goto out;
	}

	printk("kv : post_data_getter_work: post convert suc\n");

	if (error != 0)
	{
		printk("kv : post_data_getter_work : error: %i\n", error);
	}
	else
	{
		printk("kv : post_data_getter_work : suc\n");
	}

	out:
		return;
}


static void pre_bast_initial(void* arg, int mode)
{
	queue_work(workqueue, &first_bast);
}

static void pre_bast_initial_work(struct work_struct *work)
{
	int error;
	struct dlm_block* pre_block;
	struct dlm_block* post_block = get_post_block(this_node_id);


	printk("kv : pre_bast_initial_work: post block name: %s id: %i\n",
			post_block->name, post_block->lksb->sb_lkid);


	error = sync_dlm_lock(DLM_LOCK_EX, post_block, DLM_LKF_CONVERT,
				post_block->name, post_data_getter_bast);

	if (error) {
		printk("kv : pre_bast_initial_work: post convert failed: %i\n",  error);
		goto out;
	}
	else {
		printk("kv : pre_bast_initial_work: post convert succeed\n");
	}


	pre_block = get_pre_block(this_node_id);

	printk("kv : pre_bast_initial_work: pre block name: %s id: %i\n",
			pre_block->name, pre_block->lksb->sb_lkid);


	error = sync_dlm_lock(DLM_LOCK_NL, pre_block, DLM_LKF_CONVERT,
			pre_block->name, NULL);


	if (error) {
		printk("kv : pre_bast_initial_work: pre convert failed: %i\n",  error);
	}
	else
	{
		printk("kv : pre_bast_initial_work: pre convert succeed\n");
	}

out:
	return;
}



static struct dlm_block* get_pre_block(int node_id)
{
	return sys_locks[2 * node_id];
}

static struct dlm_block* get_post_block(int node_id)
{
	return sys_locks[2 * node_id + 1];
}


static int insert_sys_dlm_block(struct dlm_block* block, int is_post, int node_id)
{
	sys_locks[node_id * 2 + is_post] = block;
	return 0;
}


int dlm_init(void)
{
	int i;
	char* node_id;
	int error;
	struct dlm_block* pre_block;
	struct dlm_block* post_block;
	char  pre_name_temp_buffer[KV_MAX_KEY_NAME_LENGTH];
	char  post_name_temp_buffer[KV_MAX_KEY_NAME_LENGTH];

	sys_locks = (struct dlm_block**)kmalloc(sizeof(struct dlm_block*) * node_amount * 2,
			GFP_KERNEL);

	node_list = (char**)kmalloc(sizeof(char**), GFP_KERNEL);
	i = 0;
	while (i < node_amount)
	{
		node_list[i] = (char*)kmalloc(sizeof(char) * 10, GFP_KERNEL);
		i++;
	}

	strcpy(node_list[0], "1");
	strcpy(node_list[1], "2");

	error = init_ls();
	if (error) {
		printk("Creating DLM lockspace failed: %i\n",  error);
		goto out;
	}


	INIT_WORK(&first_bast, pre_bast_initial_work);
	INIT_WORK(&second_bast, post_data_getter_work);

	workqueue = create_singlethread_workqueue("dlm");

	i = 0;
	while (i < node_amount)
	{
		node_id = node_list[i];
		printk("kv : dlm_init : node id =  %s\n",  node_id);
		strcpy(pre_name_temp_buffer, PRE_PREFIX);
		strcat(pre_name_temp_buffer , node_id);

		strcpy(post_name_temp_buffer, POST_PREFIX);
		strcat(post_name_temp_buffer , node_id);

		pre_block  = create_dlm_block(pre_name_temp_buffer, this_machine_id);
		post_block = create_dlm_block(post_name_temp_buffer, this_machine_id);

		error  = insert_sys_dlm_block(pre_block, 0, i);
		if (error != 0)
		{
			goto out;
		}
		error  = insert_sys_dlm_block(post_block, 1, i);
		if (error != 0)
		{
			goto out;
		}

		if (strcmp(this_machine_id, node_id) == 0)
		{
			this_node_id = i;
			printk("kv : dlm_init : init my node locks\n");
			error = sync_dlm_lock(DLM_LOCK_EX, pre_block, 0,
					pre_block->name, pre_bast_initial);
		}
		else
		{
			printk("kv : dlm_init : init %s`s locks\n", node_id);
			error = sync_dlm_lock(DLM_LOCK_NL, pre_block, 0,
					pre_block->name, NULL);
		}

		if (error != 0)
		{
			printk("dlm : dlm_init: pre lock error = %i\n",  error);
			goto out;
		}

		printk("kv : dlm_init : get post lock with name: %s\n", post_name_temp_buffer);

		error = sync_dlm_lock(DLM_LOCK_NL, post_block, 0,
				post_block->name, NULL);
		i++;
	}

	print_all_sys_blocks();

out:
	printk("dlm : dlm_init: error = %i\n",  error);
	return error;
}



static void print_ast(void* value)
{
	struct dlm_block* block = (struct dlm_block*) value;
	printk(KERN_INFO "kv : print_ast: status = %i, lockid: %d\n",
			block->lksb->sb_status, block->lksb->sb_lkid);
}


static int dlm_update_block_on_the_remote_node(struct update_structure* update, int node_id)
{
	struct dlm_block* pre_block;
	struct dlm_block* post_block;
	int error;


	pre_block = get_pre_block(node_id);

	if (pre_block == NULL)
	{
		printk("kv : dlm_update_block_on_the_remote_node: block not found\n");
		return -1;
	}


	printk("kv : dlm_update_block_on_the_remote_node pre: block name: %s\n",
			pre_block->name);

	error = sync_dlm_lock(DLM_LOCK_EX, pre_block, DLM_LKF_CONVERT,
			pre_block->name, NULL);

	if (error != 0)
	{
		printk("kv : dlm_update_block_on_the_remote_node1 error = %i\n", error);
		goto out;
	}


	update_to_buffer(update, pre_block->lksb->sb_lvbptr);

	printk("kv : dlm_update_block_on_the_remote_node: buffer: %s\n", pre_block->lksb->sb_lvbptr);

	error = sync_dlm_lock(DLM_LOCK_NL, pre_block, DLM_LKF_CONVERT | DLM_LKF_VALBLK,
			pre_block->name, NULL);

	if (error != 0)
	{
		printk("kv : dlm_update_block_on_the_remote_node2 error = %i\n", error);
		goto out;
	}


	post_block = get_post_block(node_id);
	printk("kv : dlm_update_block_on_the_remote_node post: block name: %s\n",
			post_block->name);

	error = sync_dlm_lock(DLM_LOCK_EX, post_block, DLM_LKF_CONVERT,
			post_block->name, NULL);

	if (error != 0)
	{
		printk("kv : dlm_update_block_on_the_remote_node3 error = %i\n", error);
		goto out;
	}

	error = sync_dlm_lock(DLM_LOCK_NL, post_block, DLM_LKF_CONVERT,
			post_block->name, NULL);


	if (error != 0)
	{
		printk("kv : dlm_update_block_on_the_remote_node4 error = %i\n", error);
	}

out:
	return error;
}


static int dlm_update_all_nodes(struct update_structure* update)
{
	int node_id;
	int error;

	node_id = 0;
	while (node_id < node_amount)
	{
		if (this_node_id == node_id)
		{
			error = process_update(*update);
		}
		else
		{
			error = dlm_update_block_on_the_remote_node(update, node_id);
		}
		if (error != 0)
		{
			printk("dlm : dlm_init: pre lock error = %i\n",  error);
			goto out;
		}

		node_id++;
	}

out:
	return error;
}





int add_key(const char *key, const char* value)
{
	struct update_structure* update = create_update_structure(key, value, 1);
	int status;

	if (ls == NULL)
	{
		printk("kv : ls was NULL\n");
		init_ls();
		if (ls == NULL)
		{
			printk("kv : ls still 0\n");
		}
		else
		{
			printk("kv : ls is not NULL now | init in register_key??\n");
		}
	}


	printk("kv : register_key; %s = %s\n", key, value);
	status  = dlm_update_all_nodes(update);

	if ( status < 0)
	{
		printk( "dlmlock : lock error, status: %i\n", status);
	}
	return status;
}


int lock_key(const char* key_name)
{
	char* value = "\0";
	struct update_structure* update = create_update_structure(key_name, value, 3);
	int status;

	if (is_there_lock(key_name))
	{
	    printk(KERN_INFO "charDev : try to lock locked key: %s\n", key_name);
		return -1;
	}
	else
	{
		if (is_there_such_key(key_name))
		{
		    printk(KERN_INFO "charDev : there is no such key: %s\n", key_name);
			return -1;
		}
		else
		{
			printk("kv : lock_key: %s\n", key_name);
			status  = dlm_update_all_nodes(update);
			if ( status < 0)
			{
				printk( "dlmlock : lock error, status: %i\n", status);
			}
		    return status;
		}
	}
}




int unlock_key(const char* key_name)
{
	struct lock* lock;
	lock = find_lock(key_name);
	if (lock == NULL)
	{
	    printk(KERN_INFO "charDev : there is no such lock: %s\n", key_name);
		return -1;
	}
	else
	{
		if (strcmp(lock->owner, this_machine_id) != 0)
		{
		    printk(KERN_INFO "charDev : %s tries to unlock %s, which owned by %s\n", this_machine_id, key_name, lock->owner);
			return -1;
		}
		else
		{
			char* value = "\0";
			struct update_structure* update = create_update_structure(key_name, value, 4);
			int status;

			printk("kv : unlock_key: %s\n", key_name);
			status  = dlm_update_all_nodes(update);

			if ( status < 0)
			{
				printk( "dlmlock : lock error, status: %i\n", status);
			}
			return status;
		}
	}
}
