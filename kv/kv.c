#include "kv.h"


char kv_device_buffer[KV_BUFFER_SIZE];

char* this_machine_id = "default_server";



struct key_node* key_head = NULL;
struct key_node* key_tail = NULL;

struct lock_node* lock_head = NULL;
struct lock_node* lock_tail = NULL;


int status;
dlm_lockspace_t *ls = NULL;

module_param(this_machine_id, charp, 0);


int node_amount = 2;
char ** node_list = NULL;


static void print_astarg(void* value);
static struct dlm_block* get_this_node_pre_block(void);
static struct dlm_block* get_this_node_post_block(void);
static void pre_bast_initial(void* arg, int mode);
static void to_nl_by_name_astarg(void* value);


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



int lock_key(char* key_name)
{
	struct key_value* key;
	if (is_there_lock(key_name))
	{
	    printk(KERN_INFO "charDev : try to lock locked key: %s\n", key_name);
		return -1;
	}
	else
	{
		key = find_key(key_name);
		if (key == NULL)
		{
		    printk(KERN_INFO "charDev : there is no such key: %s\n", key_name);
			return -1;
		}
		else
		{
		    insert_lock(create_lock(key, this_machine_id));
		    return 0;
		}
	}
}




int unlock_key(char* key_name)
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
		    remove_lock(lock);
		    return 0;
		}
	}
}




static int init_ls(void)
{
	int status;
	status = dlm_new_lockspace("new_ls43", "mycluster", DLM_LSFL_NEWEXCL | DLM_LSFL_FS,
		      PR_DLM_LVB_LEN, NULL, NULL, NULL, &ls);
	printk("kv : ls_create_status = %i", status);
	return status;

}


static void post_data_getter_bast(void* arg, int mode)
{
	int error;
	struct dlm_block* pre_block = get_this_node_pre_block();
	struct dlm_block* post_block = get_this_node_post_block();

	error = dlm_lock(ls, DLM_LOCK_EX, pre_block->lksb, DLM_LKF_CONVERT,
				     pre_block->name, strlen(pre_block->name),  0,
					 print_astarg, pre_block, pre_bast_initial);

	if (error) {
		printk("kv : post_data_getter_bast: pre convert failed: %i",  error);
		goto out;
	}

	error = update_or_add_dlm_block(pre_block);

	if (error) {
		printk("kv : post_data_getter_bast: update_or_add_dlm_block failed: %i",  error);
		goto out;
	}

	error = dlm_lock(ls, DLM_LOCK_NL, post_block->lksb, DLM_LKF_CONVERT,
				     post_block->name, strlen(post_block->name),  0,
					 print_astarg, post_block, NULL);

out:
	return;
}


static void pre_bast_initial(void* arg, int mode)
{
	int error;
	struct dlm_block* pre_block = get_this_node_pre_block();
	struct dlm_block* post_block = get_this_node_post_block();

	error = dlm_lock(ls, DLM_LOCK_NL, pre_block->lksb, DLM_LKF_CONVERT,
				     pre_block->name, strlen(pre_block->name),  0,
					 print_astarg, pre_block, NULL);

	if (error) {
		printk("kv : pre_bast_initial: pre convert failed: %i",  error);
		goto out;
	}

	error = dlm_lock(ls, DLM_LOCK_EX, post_block->lksb, DLM_LKF_CONVERT,
				     post_block->name, strlen(post_block->name),  0,
					 print_astarg, post_block, post_data_getter_bast);

out:
	return;
}


static struct dlm_block* get_this_node_pre_block()
{
	char  pre_name_temp_buffer[100];
	strcpy(pre_name_temp_buffer, PRE_PREFIX);
	strcat(pre_name_temp_buffer , this_machine_id);

	return get_block_by_name(pre_name_temp_buffer);
}

static struct dlm_block* get_this_node_post_block()
{
	char  post_name_temp_buffer[100];
	strcpy(post_name_temp_buffer, POST_PREFIX);
	strcat(post_name_temp_buffer , this_machine_id);

	return get_block_by_name(post_name_temp_buffer);
}


int dlm_init(void)
{
	int i;
	char* node_id;
	int error;
	struct dlm_block* pre_block;
	struct dlm_block* post_block;
	char  pre_name_temp_buffer[100];
	char  post_name_temp_buffer[100];

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
		printk("Creating DLM lockspace failed: %i",  error);
		goto out;
	}

	i = 0;
	while (i < node_amount)
	{
		node_id = node_list[i];
		printk("kv : dlm_init : node id =  %s",  node_id);
		strcpy(pre_name_temp_buffer, PRE_PREFIX);
		strcat(pre_name_temp_buffer , node_id);

		strcpy(post_name_temp_buffer, POST_PREFIX);
		strcat(post_name_temp_buffer , node_id);

		pre_block  = create_dlm_block(pre_name_temp_buffer, pre_name_temp_buffer);
		post_block = create_dlm_block(post_name_temp_buffer, post_name_temp_buffer);

		printk("kv : dlm_init : buffer = %s", pre_name_temp_buffer);
		error  = insert_dlm_block(pre_block);
		if (error != 0)
		{
			goto out;
		}
		error =  insert_dlm_block(post_block);
		if (error != 0)
		{
			goto out;
		}

		if (strcmp(this_machine_id, node_id) == 0)
		{
			printk("kv : dlm_init : init my node locks");
			error = dlm_lock(ls, DLM_LOCK_EX, pre_block->lksb, 0,
							pre_name_temp_buffer, strlen(pre_name_temp_buffer),  0,
							print_astarg, pre_block, pre_bast_initial);
		}
		else
		{
			printk("kv : dlm_init : init %s`s locks", node_id);
			error = dlm_lock(ls, DLM_LOCK_NL, pre_block->lksb, 0,
							pre_name_temp_buffer, strlen(pre_name_temp_buffer),  0,
							print_astarg, pre_block, pre_bast_initial);
		}

		if (error != 0)
		{
			printk("dlm : dlm_init: pre lock error = %i",  error);
			goto out;
		}
		error = dlm_lock(ls, DLM_LOCK_NL, post_block->lksb, 0,
						post_name_temp_buffer, strlen(post_name_temp_buffer),  0,
						print_astarg, post_block, NULL);

		i++;
	}

	print_all_blocks();

out:
	printk("dlm : dlm_init: error = %i",  error);
	return error;
}


static void remove_dlm_lock_astarg(void* value)
{
	int res;
	struct dlm_block* block = (struct dlm_block*) value;

	if (block->lock_type == 0)
	{
		printk("kv : remove_dlm_lock_astarg: status = %i. Lock type = 0", block->lksb->sb_status);
	}
	else
	{
		printk("kv : remove_dlm_lock_astarg: status = %i. Lock type = %i", block->lksb->sb_status, block->lock_type);

		block->lock_type = 0;

		res = dlm_unlock(ls, block->lksb->sb_lkid, 0, block->lksb, block);

		if (res != 0)
		{
			printk("kv : remove_dlm_lock_astarg:  FAIL! res = %i", res);
		}
		else
		{
			printk("kv : remove_dlm_lock_astarg: succeed");
		}
	}
}

static void print_astarg(void* value)
{
	struct dlm_block* block = (struct dlm_block*) value;
	printk("kv : print_astarg: status = %i", block->lksb->sb_status);
}

static void to_nl_astarg(void* value)
{
	int res;
	struct dlm_block* block = (struct dlm_block*) value;
	char* key;

	printk("kv : to_nl_astarg: status = %i", block->lksb->sb_status);

	key = block->name;
	block->lksb->sb_lvbptr = block->lvb;

	block->lock_type = FLAG_NL;

	res = dlm_lock(ls, DLM_LOCK_NL, block->lksb, DLM_LKF_CONVERT,
			key, key ? strlen(key) : 0, 0,
					print_astarg, block, NULL);

	if (res != 0)
	{
		printk("kv : to_nl_astarg: NL: FAIL! dlm_lock_res = %i", res);
	}
	else
	{
		printk("kv : to_nl_astarg: set %s = %s", key, block->lvb);
	}
}


static void dlm_updater_ast(void* arg)
{
	int error;
	struct dlm_block_extended* block = (struct dlm_block_extended*) arg;

	char  pre_name_temp_buffer[100];
	char  post_name_temp_buffer[100];

	strcpy(pre_name_temp_buffer, PRE_PREFIX);
	strcat(pre_name_temp_buffer , block->node_id);
	strcpy(post_name_temp_buffer, POST_PREFIX);
	strcat(post_name_temp_buffer , block->node_id);

	error = dlm_lock(ls, DLM_LOCK_NL, block->dlm_block->lksb, DLM_LKF_CONVERT,
				     pre_name_temp_buffer, strlen(pre_name_temp_buffer), 0,
					 print_astarg, block->dlm_block, NULL);

	printk("kv : dlm_updater_ast pre error = %i", error);

	error = dlm_lock(ls, DLM_LOCK_PR, block->dlm_block->lksb, DLM_LKF_CONVERT,
				     post_name_temp_buffer, strlen(post_name_temp_buffer), 0,
					 to_nl_by_name_astarg, post_name_temp_buffer, NULL);

	printk("kv : dlm_updater_ast post error = %i", error);


}


static int dlm_update_block_on_the_remote_node(struct dlm_block* block, char* node_id)
{
	struct dlm_block* pre_block;
	struct dlm_block_extended* extended_block;
	char  pre_name_temp_buffer[100];

	int error;

	extended_block = kmalloc(sizeof(struct dlm_block_extended), GFP_KERNEL);
	extended_block->node_id = node_id;
	extended_block->dlm_block = block;

	strcpy(pre_name_temp_buffer, PRE_PREFIX);
	strcat(pre_name_temp_buffer , node_id);

	pre_block = get_block_by_name(pre_name_temp_buffer);

	if (pre_block == NULL)
	{
		printk("kv : to_ex_astarg: block not found");
		return -1;
	}

	error = dlm_lock(ls, DLM_LOCK_PR, pre_block->lksb, DLM_LKF_CONVERT,
					pre_block->name, strlen(pre_block->name), 0,
					dlm_updater_ast, extended_block, NULL);

	printk("kv : dlm_update_block_on_the_remote_node error = %i", error);

	return error;

}


static int dlm_update_all_nodes(struct dlm_block* block)
{
	char* node_id;
	int error;
	int i;

	i = 0;
	while (i < node_amount)
	{
		node_id = node_list[i];

		printk("dlm : dlm_update_all_nodes: 1");
		if (strcmp(this_machine_id, node_id) == 0)
		{
			printk("dlm : dlm_update_all_nodes: 2");
			error =	0; //update_or_add_dlm_block(block);
		}
		else
		{
			printk("dlm : dlm_update_all_nodes: 3");
			error = dlm_update_block_on_the_remote_node(block, node_id);
		}
		if (error != 0)
		{
			printk("dlm : dlm_init: pre lock error = %i",  error);
			goto out;
		}

		i++;
	}

out:
	return error;
}


static void to_nl_by_name_astarg(void* value)
{
	int res;
	char* block_name;
	struct dlm_block* block;
	char* key;

	block_name = (char*) value;
	block = get_block_by_name(block_name);
	if (block == NULL)
	{
		printk("kv : to_ex_astarg: block not found");
		return;
	}

	printk("kv : to_ex_astarg: status = %i", block->lksb->sb_status);
	block->lock_type = FLAG_EX;
	key = block->name;

	res = dlm_lock(ls, DLM_LOCK_NL, block->lksb, DLM_LKF_CONVERT,
			key, key ? strlen(key) : 0, 0,
					to_nl_astarg, block, NULL);

	if (res != 0)
	{
		printk("kv : to_nl_by_name_astarg: NL: FAIL! dlm_lock_res = %i", res);
	}
	else
	{
		block->lvb = block->lksb->sb_lvbptr;
		printk("kv : to_nl_by_name_astarg: get value = %s", block->lksb->sb_lvbptr);
	}
}

static void to_ex_astarg(void* value)
{
	int res;
	struct dlm_block* block = (struct dlm_block*) value;
	char* key;


	printk("kv : to_ex_astarg: status = %i", block->lksb->sb_status);
	block->lock_type = FLAG_EX;
	key = block->name;

	res = dlm_lock(ls, DLM_LOCK_EX, block->lksb, DLM_LKF_CONVERT,
			key, key ? strlen(key) : 0, 0,
					to_nl_astarg, block, NULL);

	if (res != 0)
	{
		printk("kv : to_ex_astarg: NL: FAIL! dlm_lock_res = %i", res);
	}
	else
	{
		block->lvb = block->lksb->sb_lvbptr;
		printk("kv : to_ex_astarg: get value = %s", block->lksb->sb_lvbptr);
	}
}



int dlm_get_lkvb(struct dlm_block* block)
{
	int res;
	char* key;

	block->lock_type = FLAG_NL;
	key = block->name;

	res = dlm_lock(ls, DLM_LOCK_NL, block->lksb, 0,
			key, key ? strlen(key) : 0, 0,
					to_ex_astarg, block, NULL);
	if (res != 0)
	{
		printk("kv : NL: FAIL! dlm_lock_res = %i", res);
	}
	else
	{
		printk("kv : Dlm EX successful");
	}
	return res;
}




int register_key(char *key, char* value)
{
	struct dlm_block* new_dlm_block = create_dlm_block(key, value);
	int status;

	if (ls == NULL)
	{
		printk("kv : ls was NULL");
		init_ls();
		if (ls == NULL)
		{
			printk("kv : ls still 0 loooooooool");
		}
		else
		{
			printk("kv : ls is not NULL now | init in register_key??");
		}
	}


	printk("kv : register_key : 1");
	status  = dlm_update_all_nodes(new_dlm_block);

	if ( status < 0)
	{
		printk( "dlmlock : lock error, status: %i", status);
	}
	return status;
}


