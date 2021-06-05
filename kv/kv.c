#include "kv.h"

char kv_device_buffer[KV_BUFFER_SIZE];

char *this_machine_id = "default_server";

struct key_node *key_head = NULL;
struct key_node *key_tail = NULL;

struct lock_node *lock_head = NULL;
struct lock_node *lock_tail = NULL;

struct workqueue_struct *workqueue = NULL;

int status;
dlm_lockspace_t *ls = NULL;

module_param(this_machine_id, charp, 0);

int node_amount = 2;
char **node_list = NULL;
int this_node_id;
struct dlm_block **sys_locks;

struct work_struct first_bast;
struct work_struct second_bast;
struct dlm_block *data_block;

static struct dlm_block *get_pre_block(int node_id);
static struct dlm_block *get_post_block(int node_id);
static void pre_bast_initial(void *arg, int mode);


void dlm_hw(void)
{
	pr_info("dlm : dlm_hw : Hello, world\n");
}
EXPORT_SYMBOL(dlm_hw);

static void dlm_wait_ast(void *astarg)
{
	struct dlm_block *dlm_block = astarg;

	if (!dlm_block->lksb->sb_status)
	{
		pr_info("kv : dlm_wait_ast : Locking error: %i name: %s\n",
					dlm_block->lksb->sb_status, dlm_block->name);
	}
	complete(dlm_block->compl);
}

static int sync_dlm_lock(int mode,
			 struct dlm_block *block, int flags,
			 const char *name, void (*bast) (void *, int))
{
	int res = 0;

	init_completion(block->compl);

	res = dlm_lock(ls, mode, block->lksb, flags,
		       (void *)name, name ? strlen(name) : 0, 0,
		       dlm_wait_ast, block, bast);

	if (res < 0)
		pr_info("kv : sync_dlm_lock : Locking error: %i name: %s\n",
			res, name);

	res = wait_for_completion_timeout(block->compl, 60 * HZ);

	if (res > 0)
		res = block->lksb->sb_status;
	else if (res == 0)
		res = -ETIMEDOUT;
	if (res < 0) {
		pr_info("kv : sync_dlm_lock : Locking error: %i name: %s\n",
			res, name);
	}

	return res;
}


static void print_all_sys_blocks(void)
{
	int nodeid = 0;
	struct dlm_block *cur;
	while (nodeid < node_amount * 2) {
		cur = sys_locks[nodeid];
		pr_info
		    ("dlm : print_all_blocks: name: %s, value: %s, lockid: %i, status: %d\n",
		     cur->name, cur->lksb->sb_lvbptr, cur->lksb->sb_lkid,
		     cur->lksb->sb_status);

		nodeid++;
	}

}

int get_values(char *buff, size_t length, loff_t * ppos)
{
	int size_in_min_buffer_left, bytes_written, offset;

	size_in_min_buffer_left = MIN(length, KV_BUFFER_SIZE);
	offset = *ppos;

	bytes_written =
	    write_header(kv_device_buffer, size_in_min_buffer_left, &offset,
			 "keys:\n");
	size_in_min_buffer_left -= bytes_written;

	if (size_in_min_buffer_left > 0) {
		bytes_written +=
		    fill_buffer_with_keys(kv_device_buffer + bytes_written,
					  size_in_min_buffer_left, &offset);
		size_in_min_buffer_left -= bytes_written;
	}

	if (size_in_min_buffer_left > 0) {
		bytes_written +=
		    write_header(kv_device_buffer + bytes_written,
				 size_in_min_buffer_left, &offset,
				 "\nlocks:\n");
		size_in_min_buffer_left -= bytes_written;
	}

	if (size_in_min_buffer_left > 0) {
		bytes_written +=
		    fill_buffer_with_locks(kv_device_buffer + bytes_written,
					   size_in_min_buffer_left, &offset);
		size_in_min_buffer_left -= bytes_written;
	}

	memcpy(buff, kv_device_buffer, bytes_written);

	return bytes_written;
}

static int init_ls(void)
{
	int status;

	ls =
	    kmalloc(sizeof(dlm_lockspace_t), GFP_KERNEL);
	if (!ls) {
		status = -ENOMEM;
		goto out;
	}

	status = dlm_new_lockspace("new_ls190", "mycluster", DLM_LSFL_FS,
				   PR_DLM_LVB_LEN, NULL, NULL, NULL, &ls);

	pr_info("kv : init_ls: ls_create_status = %i\n", status);

out:
	return status;

}

static int process_update(const struct update_structure update)
{
	int error = 0;

	pr_info("kv : process_update : update type %c, key: %s, value: %s\n",
		update.type, update.key, update.value);

	switch (update.type) {
	case '1':
		error = update_or_add_key(update.key, update.value);
		pr_info("kv : process_update: ins or up %s = %s\n", update.key,
			update.value);
		break;
	case '2':
		error = remove_key(update.key);
		pr_info("kv : process_update: remove key %s\n", update.key);
		break;
	case '3':
		error = insert_lock(create_lock(update.key, update.value));
		pr_info("kv : process_update: locked %s by %s\n", update.key, update.value);
		break;
	case '4':
		error = remove_lock(update.key);
		pr_info("kv : process_update: unlocked %s\n", update.key);
		break;
    default:
		pr_info("kv : process_update: wrong operation type %c\n", update.type);
	}

	if (error) {
		pr_info
		    ("kv : process_update: update fail: type: %c error: %i\n",
		     update.type, error);
	}

	return error;
}

static void post_data_getter_bast(void *arg, int mode)
{
	pr_info("kv : post_data_getter_bast: start work\n");
	queue_work(workqueue, &second_bast);
}

static void post_data_getter_work(struct work_struct *work)
{
	struct update_structure update;

	int error = 0;

	struct dlm_block *post_block;
	struct dlm_block *pre_block = get_pre_block(this_node_id);

	if (pre_block == NULL) {
		pr_info("kv : post_data_getter_work: pre block not found\n");
		goto out;
	}

	error = sync_dlm_lock(DLM_LOCK_EX, pre_block,
			  DLM_LKF_CONVERT, pre_block->name,
			  pre_bast_initial);

	pr_info("kv : post_data_getter_work: pre convert suc\n");

	if (error) {
		pr_info("kv : post_data_getter_work: pre convert failed: %i\n",
			error);
		goto out;
	}

	error = sync_dlm_lock(DLM_LOCK_PW, data_block,
			  DLM_LKF_CONVERT | DLM_LKF_VALBLK, data_block->name,
			  NULL);

	if (error) {
		pr_info("kv : post_data_getter_work: first data block convert failed: %i\n",
			error);
		goto out;
	}

	pr_info("kv : post_data_getter_work : buffer: %s\n",
		data_block->lksb->sb_lvbptr);

    update_from_buffer(&update, data_block->lksb->sb_lvbptr);

    printk("kv : post_data_getter_work : update type %i, key: %s, value: %s\n",
                      update.type, update.key, update.value);

    error = process_update(update);

	if (error) {
		pr_info
		    ("kv : post_data_getter_work : process update error: %i\n",
		     error);
	}


	error = sync_dlm_lock(DLM_LOCK_CR, data_block,
			  DLM_LKF_CONVERT| DLM_LKF_VALBLK, data_block->name,
			  NULL);

	if (error) {
		pr_info("kv : post_data_getter_work: second data block convert failed: %i\n",
			error);
		goto out;
	}


	post_block = get_post_block(this_node_id);

	if (post_block == NULL) {
		pr_info("kv : post_data_getter_work: post block not found\n");
		goto out;
	}

	error = sync_dlm_lock(DLM_LOCK_NL, post_block, DLM_LKF_CONVERT,
			      post_block->name, post_data_getter_bast);

	if (error != 0){
		pr_info("kv : post_data_getter_work: post convert failed: %i\n",
			error);
		goto out;
	}

	pr_info("kv : post_data_getter_work: post convert suc\n");

	if (error != 0) {
		pr_info("kv : post_data_getter_work : error: %i\n", error);
	} else {
		pr_info("kv : post_data_getter_work : suc\n");
	}

out:
	return;
}

static void pre_bast_initial(void *arg, int mode)
{
	queue_work(workqueue, &first_bast);
}

static void pre_bast_initial_work(struct work_struct *work)
{
	int error;
	struct dlm_block *pre_block;
	struct dlm_block *post_block = get_post_block(this_node_id);


	if (post_block == NULL) {
		pr_info("kv : pre_bast_initial_work: post block not found\n");
		goto out;
	}

	pr_info("kv : pre_bast_initial_work: post block name: %s id: %i\n",
		post_block->name, post_block->lksb->sb_lkid);

	error = sync_dlm_lock(DLM_LOCK_EX, post_block, DLM_LKF_CONVERT,
			      post_block->name, post_data_getter_bast);

	if (error) {
		pr_info("kv : pre_bast_initial_work: post convert failed: %i\n",
			error);
		goto out;
	} else {
		pr_info("kv : pre_bast_initial_work: post convert succeed\n");
	}

	pre_block = get_pre_block(this_node_id);

	if (pre_block == NULL) {
		pr_info("kv : pre_bast_initial_work: pre block not found\n");
		goto out;
	}

	pr_info("kv : pre_bast_initial_work: pre block name: %s id: %i\n",
		pre_block->name, pre_block->lksb->sb_lkid);

	error = sync_dlm_lock(DLM_LOCK_NL, pre_block, DLM_LKF_CONVERT,
			      pre_block->name, NULL);

	if (error) {
		pr_info("kv : pre_bast_initial_work: pre convert failed: %i\n",
			error);
	} else {
		pr_info("kv : pre_bast_initial_work: pre convert succeed\n");
	}

out:
	return;
}

static struct dlm_block *get_pre_block(int node_id)
{
	return sys_locks[2 * node_id];
}

static struct dlm_block *get_post_block(int node_id)
{
	return sys_locks[2 * node_id + 1];
}

static int insert_sys_dlm_block(struct dlm_block *block, int is_post,
				int node_id)
{
	sys_locks[node_id * 2 + is_post] = block;
	return 0;
}

int dlm_init(void)
{
	int i;
	char *node_id;
	int error;
	struct dlm_block *pre_block;
	struct dlm_block *post_block;
	char pre_name[KV_MAX_KEY_NAME_LENGTH];
	char post_name[KV_MAX_KEY_NAME_LENGTH];
	char** node_ids_list;
//
//	// error = get_cluster_nodes_info(&i, node_ids_list);
//
	sys_locks =
	    kcalloc(node_amount * 2, sizeof(struct dlm_block *), GFP_KERNEL);
	if (!sys_locks) {
		error = -ENOMEM;
		goto out;
	}

	node_list = kmalloc(sizeof(char **), GFP_KERNEL);
	if (!node_list) {
		error = -ENOMEM;
		goto out;
	}

	for (i = 0; i < node_amount; i++) {
		node_list[i] =
		    kcalloc(MAX_NODE_NAME_LENGTH, sizeof(char), GFP_KERNEL);
		if (!node_list[i]) {
			error = -ENOMEM;
			goto out;
		}
	}

	strncpy(node_list[0], "1", MAX_NODE_NAME_LENGTH - 1);
	strncpy(node_list[1], "2", MAX_NODE_NAME_LENGTH - 1);

	error = init_ls();
	if (error) {
		pr_info("Creating DLM lockspace failed: %i\n", error);
		goto out;
	}

	INIT_WORK(&first_bast, pre_bast_initial_work);
	INIT_WORK(&second_bast, post_data_getter_work);

	workqueue = create_singlethread_workqueue("dlm");

	for (i = 0; i < node_amount; i++) {
		node_id = node_list[i];
		pr_info("node id =  %s\n", node_id);
		strncpy(pre_name, PRE_PREFIX, KV_MAX_KEY_NAME_LENGTH - 1);
		strncat(pre_name, node_id,
			KV_MAX_KEY_NAME_LENGTH - strlen(PRE_PREFIX) - 1);

		strncpy(post_name, POST_PREFIX, KV_MAX_KEY_NAME_LENGTH - 1);
		strncat(post_name, node_id,
			KV_MAX_KEY_NAME_LENGTH - strlen(POST_PREFIX) - 1);

		pre_block = create_dlm_block(pre_name, this_machine_id);
		post_block = create_dlm_block(post_name, this_machine_id);

		error = insert_sys_dlm_block(pre_block, 0, i);
		if (error != 0) {
			goto out;
		}
		error = insert_sys_dlm_block(post_block, 1, i);
		if (error != 0) {
			goto out;
		}

		if (strcmp(this_machine_id, node_id) == 0) {
			this_node_id = i;
			pr_info("init my node locks\n");
			error = sync_dlm_lock(DLM_LOCK_EX, pre_block, 0,
					      pre_block->name,
					      pre_bast_initial);
		} else {
			pr_info("init %s`s locks\n", node_id);
			error = sync_dlm_lock(DLM_LOCK_NL, pre_block, 0,
					      pre_block->name, NULL);
		}

		if (error != 0) {
			pr_info("pre lock error = %i\n", error);
			goto out;
		}

		pr_info("get post lock with name: %s\n", post_name);

		error = sync_dlm_lock(DLM_LOCK_NL, post_block, 0,
				      post_block->name, post_data_getter_bast);
	}

	data_block = create_dlm_block("data_block", this_machine_id);

	pr_info("data block with name: %s\n", data_block->name);

	error = sync_dlm_lock(DLM_LOCK_CR, data_block, DLM_LKF_VALBLK,
			data_block->name, NULL);

	print_all_sys_blocks();

out:
	pr_info("error = %i\n", error);
	return error;
}


static int dlm_update_block_on_the_remote_node(const struct update_structure *update,
					       int node_id)
{
	struct dlm_block *pre_block;
	struct dlm_block *post_block;
	int error;

	pre_block = get_pre_block(node_id);

	if (pre_block == NULL) {
		pr_info
		    ("kv : dlm_update_block_on_the_remote_node: block not found\n");
		return -1;
	}

	pr_info("kv : dlm_update_block_on_the_remote_node pre: block name: %s\n",
	     pre_block->name);

	error = sync_dlm_lock(DLM_LOCK_EX, pre_block, DLM_LKF_CONVERT,
			      pre_block->name, pre_bast_initial);

	pr_info
	    ("kv : dlm_update_block_on_the_remote_node1 pre_block convert succ\n",
	     error);

	if (error != 0) {
		pr_info
		    ("kv : dlm_update_block_on_the_remote_node1 error = %i\n",
		     error);
		goto out;
	}


	error = sync_dlm_lock(DLM_LOCK_NL, pre_block,
			  DLM_LKF_CONVERT, pre_block->name,
			  NULL);

	if (error != 0) {
		pr_info
		    ("kv : dlm_update_block_on_the_remote_node2 error = %i\n",
		     error);
		goto out;
	}

	error =
	    sync_dlm_lock(DLM_LOCK_PW, data_block,
			  DLM_LKF_CONVERT | DLM_LKF_VALBLK, data_block->name,
			  NULL);

	if (error != 0) {
		pr_info
		    ("kv : dlm_update_block_on_the_remote_node data block 1 error = %i\n",
		     error);
		goto out;
	}

	update_to_buffer(update, data_block->lksb->sb_lvbptr);
	pr_info("kv : dlm_update_block_on_the_remote_node: buffer: %s\n",
			data_block->lksb->sb_lvbptr);

	error =
	    sync_dlm_lock(DLM_LOCK_CR, data_block,
			  DLM_LKF_CONVERT | DLM_LKF_VALBLK, data_block->name,
			  NULL);

	if (error != 0) {
		pr_info
		    ("kv : dlm_update_block_on_the_remote_node data block 2 error = %i\n",
		     error);
		goto out;
	}

	post_block = get_post_block(node_id);
	pr_info
	    ("kv : dlm_update_block_on_the_remote_node post: block name: %s\n",
	     post_block->name);

	error = sync_dlm_lock(DLM_LOCK_PR, post_block, DLM_LKF_CONVERT,
			      post_block->name, post_data_getter_bast);

	if (error != 0) {
		pr_info
		    ("kv : dlm_update_block_on_the_remote_node3 error = %i\n",
		     error);
		goto out;
	}

	error = sync_dlm_lock(DLM_LOCK_NL, post_block, DLM_LKF_CONVERT,
			      post_block->name, post_data_getter_bast);

	if (error) {
		pr_info
		    ("kv : dlm_update_block_on_the_remote_node4 error = %i\n",
		     error);
	} else {
		pr_info("kv : dlm_update_block_on_the_remote : success!!\n");
	}

out:
	return error;
}

static int dlm_update_all_nodes(const struct update_structure *update)
{
	int node_id;
	int error;

	for (node_id = 0; node_id < node_amount; node_id++) {
		if (this_node_id == node_id) {
			error = process_update(*update);
		} else {
			error =
			    dlm_update_block_on_the_remote_node(update,
								node_id);
		}
		if (error != 0) {
			pr_info("dlm : dlm_update_all_nodes: error = %i\n", error);
			goto out;
		}

	}

out:
	return error;
}

int kv_add_key(const char *key, const char *value)
{
	struct update_structure *update =
	    create_update_structure(key, value, '1');
	int status = 0;
	struct lock* lock;

	if (is_there_such_key(key))
	{
		lock = find_lock(key);
		if (lock != NULL)
		{
			if (strcmp(lock->owner, this_machine_id) != 0)
			{
				pr_info
				    ("charDev : Can't update key %s locked by %s\n",
				     key, lock->owner);
				return -1;
			}
		}
	}

	pr_info("kv : register_key; %s = %s\n", key, value);
	status = dlm_update_all_nodes(update);

	if (status < 0) {
		pr_info("dlmlock : lock error, status: %i\n", status);
	}
	return status;
}
EXPORT_SYMBOL(kv_add_key);


int kv_remove_key(const char *key)
{
	char value = '\0';
	struct update_structure *update =
	    create_update_structure(key, &value, '2');
	int status = 0;
	struct lock *lock = find_lock(key);

	if (lock != NULL) {
		pr_info("kv_remove_key : can't remove key %s, because it is locked\n", key);
		return -1;
	}

	if (ls == NULL) {
		pr_info("kv : ls was NULL\n");
		init_ls();
		if (ls == NULL) {
			pr_info("kv : ls still 0\n");
		} else {
			pr_info
			    ("kv : ls is not NULL now | init in register_key??\n");
		}
	}

	pr_info("kv : remove_key %s \n", key);
	status = dlm_update_all_nodes(update);

	if (status < 0) {
		pr_info("dlmlock : lock error, status: %i\n", status);
	}
	return status;
}

int kv_lock_key(const char *key_name)
{
	struct update_structure *update =
	    create_update_structure(key_name, this_machine_id, '3');
	int status;

	if (is_there_lock(key_name)) {
		pr_info(KERN_INFO "charDev : try to lock locked key: %s\n",
			key_name);
		return -1;
	} else {
		if (!is_there_such_key(key_name)) {
			pr_info(KERN_INFO
				"charDev : there is no such key: %s\n",
				key_name);
			return -1;
		} else {
			pr_info("kv : lock_key: %s\n", key_name);
			status = dlm_update_all_nodes(update);
			if (status < 0) {
				pr_info("dlmlock : lock error, status: %i\n",
					status);
			}
			return status;
		}
	}
}
EXPORT_SYMBOL(kv_lock_key);

int kv_unlock_key(const char *key_name)
{
	struct lock *lock;
	lock = find_lock(key_name);
	if (lock == NULL) {
		pr_info("charDev : there is no such lock: %s\n", key_name);
		return -1;
	} else {
		if (strcmp(lock->owner, this_machine_id) != 0) {
			pr_info
			    ("charDev : %s tries to unlock %s, which owned by %s\n",
			     this_machine_id, key_name, lock->owner);
			return -1;
		} else {
			char value = '\0';
			struct update_structure *update =
			    create_update_structure(key_name, &value, '4');
			int status;

			pr_info("kv : unlock_key: %s\n", key_name);
			status = dlm_update_all_nodes(update);

			if (status < 0) {
				pr_info("dlmlock : lock error, status: %i\n",
					status);
			}
			return status;
		}
	}
}
EXPORT_SYMBOL(kv_unlock_key);
