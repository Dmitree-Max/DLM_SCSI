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
int this_node_id;
struct dlm_block** sys_locks;


static void print_astarg(void* value);
static void print_astarg_test(void* value)
{
	struct dlm_block* block = (struct dlm_block*) value;

	printk(KERN_INFO "kv : print_astarg_test: status = %i, lockid: %d",
			block->lksb->sb_status, block->lksb->sb_lkid);
}

static void print_astarg_key_value(void* value);
static struct dlm_block* get_pre_block(int node_id);
static struct dlm_block* get_post_block(int node_id);
static void pre_bast_initial(void* arg, int mode);


static void print_lksb(struct dlm_lksb* lksb)
{
	printk("kv lksb : status: %d, id: %i, flags: %d, lvb : %s",
			lksb->sb_status, lksb->sb_lkid, lksb->sb_flags, lksb->sb_lvbptr);
}


static void print_all_sys_blocks(void)
{
	int nodeid = 0;
	struct dlm_block* cur;
	while (nodeid < node_amount * 2)
	{
		cur = sys_locks[nodeid];
		printk("dlm : print_all_blocks: name: %s, value: %s, lockid: %i, status: %d",
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
	status = dlm_new_lockspace("new_ls165", "mycluster", DLM_LSFL_FS,
		      PR_DLM_LVB_LEN, NULL, NULL, NULL, &ls);

	printk("kv : init_ls: sleeping for 3 sec");

	msleep(2000);
	printk("kv : init_ls: awake");
	printk("kv : init_ls: ls_create_status = %i", status);
	return status;

}


static int process_update(struct update_structure update)
{
	int error ;

	printk("kv : process_update : update type %i, key: %s, value: %s",
			update.type, update.key, update.value);

	switch (update.type)
	{
		case 1:
			error =	update_or_add_key(update.key, update.value);
			printk("kv : process_update: ins or up %s = %s", update.key, update.value);
			break;
		case 3:
			error =	insert_lock(create_lock(update.key, this_machine_id));
			printk("kv : process_update: locked %s ", update.key);
			break;
		case 4:
			error =	remove_lock(update.key);
			printk("kv : process_update: locked %s ", update.key);
			break;
	}

	if (error) {
		printk("kv : process_update: update fail: type: %i error: %i", update.type, error);
	}

	return error;
}


static void process_update_ast(void* value)
{


	struct dlm_block* block = (struct dlm_block*) value;
	struct dlm_block* post_block = get_post_block(this_node_id);
	struct update_structure update;

	int error = 0;

	print_all_sys_blocks();
	printk("kv : process_update_ast : buffer: %s", block->lksb->sb_lvbptr);

	update_from_buffer(&update, block->lksb->sb_lvbptr);

	printk("kv : process_update_ast : update type %i, key: %s, value: %s",
			update.type, update.key, update.value);

	switch (update.type)
	{
		case 1:
			error =	update_or_add_key(update.key, update.value);
			printk("kv : process_update_ast: ins or up %s = %s", update.key, update.value);
			break;
		case 3:
			error =	insert_lock(create_lock(update.key, this_machine_id));
			printk("kv : process_update_ast: locked %s ", update.key);
			break;
		case 4:
			error =	remove_lock(update.key);
			printk("kv : process_update_ast: locked %s ", update.key);
			break;
	}

	if (error) {
		printk("kv : process_update_ast: update fail: type: %i error: %i", update.type, error);
		goto out;
	}

	msleep(1000);

	error = dlm_lock(ls, DLM_LOCK_NL, post_block->lksb, DLM_LKF_CONVERT,
				     post_block->name, strlen(post_block->name),  0,
					 print_astarg, post_block, NULL);

	msleep(1000);

	if (error) {
		printk("kv : process_update_ast: post convert failed: %i",  error);
		goto out;
	}

	printk("kv : process_update_ast: post convert suc");

	if (error != 0)
	{
		printk("kv : process_update_ast : error: %i", error);
	}
	else
	{
		printk("kv : process_update_ast : suc");
	}

	out:
		return;
}


static void post_data_getter_bast(void* arg, int mode)
{
	int error;
	struct dlm_block* pre_block  = get_pre_block(this_node_id);

	error = dlm_lock(ls, DLM_LOCK_EX, pre_block->lksb, DLM_LKF_CONVERT | DLM_LKF_VALBLK,
				     pre_block->name, strlen(pre_block->name),  0,
					 process_update_ast, pre_block, pre_bast_initial);

	if (error) {
		printk("kv : post_data_getter_bast: pre convert failed: %i",  error);
		goto out;
	}

	printk("kv : post_data_getter_bast: pre convert suc");

out:
	return;
}



static void pre_step_two_ast(void* arg)
{
	int error;
	struct dlm_block* pre_block = get_pre_block(this_node_id);



	printk("kv : pre_step_two_ast: 1111");
	printk("kv : pre_bast_initial: pre block name: %s id: %i",
			pre_block->name, pre_block->lksb->sb_lkid);

	msleep(1000);
	error = dlm_lock(ls, DLM_LOCK_NL, pre_block->lksb, DLM_LKF_CONVERT,
				     pre_block->name, strlen(pre_block->name),  0,
					 print_astarg, pre_block, NULL);

	msleep(1000);

	printk("kv : pre_step_two_ast: 222");

	if (error) {
		printk("kv : pre_step_two_ast: pre convert failed: %i",  error);
	}
	else
	{
		printk("kv : pre_step_two_ast: pre convert succeed");
	}
}


static void pre_bast_initial(void* arg, int mode)
{
	int error;
	struct dlm_block* post_block = get_post_block(this_node_id);

	print_all_sys_blocks();

	printk("kv : pre_bast_initial: post block name: %s id: %i",
			post_block->name, post_block->lksb->sb_lkid);

	error = dlm_lock(ls, DLM_LOCK_EX, post_block->lksb, DLM_LKF_CONVERT,
				     post_block->name, strlen(post_block->name),  0,
					 pre_step_two_ast, post_block, post_data_getter_bast);


	if (error) {
		printk("kv : pre_bast_initial: post convert failed: %i",  error);
		goto out;
	}

	printk("kv : pre_bast_initial: post convert succeed");

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
							print_astarg, pre_block, NULL);
		}

		if (error != 0)
		{
			printk("dlm : dlm_init: pre lock error = %i",  error);
			goto out;
		}

		msleep(1000);
		printk("kv : dlm_init : get post lock with name: %s", post_name_temp_buffer);
		error = dlm_lock(ls, DLM_LOCK_NL, post_block->lksb, 0,
						post_name_temp_buffer, strlen(post_name_temp_buffer),  0,
						print_astarg_test, post_block, NULL);

		msleep(1000);

		i++;
	}

	print_all_sys_blocks();

out:
	printk("dlm : dlm_init: error = %i",  error);
	return error;
}



static void print_astarg(void* value)
{
	struct dlm_block* block = (struct dlm_block*) value;
	printk(KERN_INFO "kv : print_astarg: status = %i, lockid: %d",
			block->lksb->sb_status, block->lksb->sb_lkid);
}

static void print_astarg_key_value(void* value)
{
	struct dlm_block* block = (struct dlm_block*) value;
    print_all_blocks();

	printk("kv : print_astarg_key_value: status = %i, lockid: %d key: %s value: %s",
			block->lksb->sb_status, block->lksb->sb_lkid, block->name, block->lksb->sb_lvbptr);
}

static void to_nl_ast(void* value)
{
	int res;
	struct dlm_block* block = (struct dlm_block*) value;

	printk("kv : to_nl_ast: status = %i", block->lksb->sb_status);

	res = dlm_lock(ls, DLM_LOCK_NL, block->lksb, DLM_LKF_CONVERT,
			block->name, strlen(block->name), 0,
					print_astarg, block, NULL);

	if (res != 0)
	{
		printk("kv : to_nl_ast: NL: FAIL! dlm_lock_res = %i", res);
	}
	else
	{
		printk("kv : to_nl_ast:  %s ", block->name);
	}
}

static void dlm_updater_second_ast(void* arg)
{
	int error;
	struct update_with_target* target_update = (struct update_with_target*) arg;
	struct dlm_block* post_block;
	struct update_structure* update;

	print_all_sys_blocks();

	update = target_update->update;
	post_block = get_post_block(target_update->target_id);

	printk("kv : dlm_updater_second_ast post: publish key: %s, value: %s",
			update->key, update->value);

	printk("kv : dlm_updater_second_ast post: block name: %s",
			post_block->name);

	error = dlm_lock(ls, DLM_LOCK_EX, post_block->lksb, DLM_LKF_CONVERT,
					 post_block->name, strlen(post_block->name), 0,
					 to_nl_ast,  post_block, NULL);

	printk("kv : dlm_updater_second_ast post error = %i", error);

	printk("kv : dlm_updater_second_ast test333333333333");
}

static void dlm_updater_ast(void* arg)
{
	int error;
	struct update_with_target* target_update = (struct update_with_target*) arg;
	struct dlm_block* pre_block;
	struct update_structure* update;

	update = target_update->update;
	pre_block = get_pre_block(target_update->target_id);

	update_to_buffer(update, pre_block->lksb->sb_lvbptr);

	printk("kv : dlm_updater_ast: buffer: %s", pre_block->lksb->sb_lvbptr);

	error = dlm_lock(ls, DLM_LOCK_NL, pre_block->lksb, DLM_LKF_CONVERT | DLM_LKF_VALBLK,
					 pre_block->name, strlen(pre_block->name), 0,
					 dlm_updater_second_ast, arg, NULL);

	printk("kv : dlm_updater_ast pre error = %i", error);

}


static int dlm_update_block_on_the_remote_node(struct update_structure* update, int node_id)
{
	struct dlm_block* pre_block;
	int error;
	struct update_with_target* target_update;


	pre_block = get_pre_block(node_id);

	if (pre_block == NULL)
	{
		printk("kv : to_ex_astarg: block not found");
		return -1;
	}

	target_update = create_update_target(update, node_id);

	printk("kv : dlm_update_block_on_the_remote_node pre: block name: %s",
			pre_block->name);

	error = dlm_lock(ls, DLM_LOCK_EX, pre_block->lksb, DLM_LKF_CONVERT,
					pre_block->name, strlen(pre_block->name), 0,
					dlm_updater_ast, target_update, NULL);

	printk("kv : dlm_update_block_on_the_remote_node error = %i", error);

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
			printk("dlm : dlm_init: pre lock error = %i",  error);
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
		printk("kv : ls was NULL");
		init_ls();
		if (ls == NULL)
		{
			printk("kv : ls still 0");
		}
		else
		{
			printk("kv : ls is not NULL now | init in register_key??");
		}
	}


	printk("kv : register_key; %s = %s", key, value);
	status  = dlm_update_all_nodes(update);

	if ( status < 0)
	{
		printk( "dlmlock : lock error, status: %i", status);
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
			printk("kv : lock_key: %s", key_name);
			status  = dlm_update_all_nodes(update);
			if ( status < 0)
			{
				printk( "dlmlock : lock error, status: %i", status);
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

			printk("kv : unlock_key: %s", key_name);
			status  = dlm_update_all_nodes(update);

			if ( status < 0)
			{
				printk( "dlmlock : lock error, status: %i", status);
			}
			return status;
		}
	}
}
