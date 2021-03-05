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





//
//
//static inline int kv_dlm_new_lockspace(const char *name, int namelen,
//					 dlm_lockspace_t **lockspace,
//					 uint32_t flags,
//					 int lvblen)
//{
//	return dlm_new_lockspace(name, NULL, flags, lvblen, NULL, NULL, NULL,
//				 lockspace);
//}
//

static int init_ls(void)
{
	int status;
	status = dlm_new_lockspace("new_ls8", "mycluster", DLM_LSFL_NEWEXCL | DLM_LSFL_FS,
		      PR_DLM_LVB_LEN, NULL, NULL, NULL, &ls);
	printk("kv : ls_create_status = %i", status);
	return status;

}


int dlm_init(void)
{
	int res;
	char* lsp_name = "testlsp8";

	res = init_ls();
	if (res) {
		printk("Creating DLM lockspace %s failed: %d", lsp_name, res);
	}
	return res;
}

static void make_test_keys(void)
{
    char first_key[]             = "test_key";
    char first_key_value[]       = "secret";
    char second_key[]            = "foo";
    char second_key_value[]      = "bar";
    register_key(first_key, first_key_value);
    register_key(second_key, second_key_value);
}


static void make_test_lock(struct key_value* key)
{
    char first_owner[]             = "Pentagon";

    insert_lock(create_lock(key, first_owner));
}

void add_test_data(void)
{
	  make_test_keys();
	  make_test_lock(key_head->data);
}


//static void scst_dlm_ast(void *astarg)
//{
//	struct scst_lksb *scst_lksb = astarg;
//
//	complete(&scst_lksb->compl);
//}
//
//
//static int scst_dlm_lock_wait(dlm_lockspace_t *ls, int mode,
//			      struct scst_lksb *lksb, int flags,
//			      const char *name, void (*bast)(void *, int))
//{
//	int res;
//
//	struct dlm_lksb lksb_lksb;
//	lksb->lksb = lksb_lksb;
//	init_completion(&lksb->compl);
//	res = dlm_lock(ls, mode, &lksb->lksb, flags,
//			    (void *)name, name ? strlen(name) : 0, 0,
//			    scst_dlm_ast, lksb, bast);
//
//	if (res < 0)
//	{
//		printk("res < 0");
//		goto out;
//	}
//	res = wait_for_completion_timeout(&lksb->compl, 60 * HZ);
//	if (res > 0)
//		res = lksb->lksb.sb_status;
//	else if (res == 0)
//		res = -ETIMEDOUT;
//	if (res < 0) {
//		//int res2 = scst_dlm_cancel(ls, lksb, flags, name);
//
//		printk("canceling lock %s / %08x failed: %d\n",
//		     name ? : "?", lksb->lksb.sb_lkid, res);
//	}
//
//out:
//	return res;
//}


static int my_dlm_lock(int mode, char* name, char* value)
{
	int res;
	struct dlm_lksb new_lksb;

	new_lksb.sb_lvbptr = value;
	printk("dlm : ls: %p" , ls);
	res = dlm_lock(ls, mode, &new_lksb, 0,
			    name, name ? strlen(name) : 0, 0,
			    NULL, NULL, NULL);
	printk("dlm : my_dlm_lock_res = %i, EINVAL = %i", res, EINVAL);
	return res;

}


int register_key(char *key, char* value)
{
	int status;
	struct scst_lksb lksb;

	if (ls == NULL)
	{
		printk("ls was NULL");
		init_ls();
	}
	if (ls == NULL)
	{
		printk("ls still 0 loooooooool");
	}
	else
	{
		printk("ls is not NULL");
	}


	printk("kv : registring key");
	status  = my_dlm_lock(DLM_LOCK_EX, key, value);
//	status = dlm_lock(ls,				/* dlm_lockspace_t */
//					  DLM_LOCK_EX,      /* mode  */
//					  &lksb,   			/* addr of lock status block */
//					  0,   		    	/* flags  */
//					  "RES-A",   		/* name  */
//					  5,  				/* namelen  */
//					  0,  		    	/* ast routine triggered */
//					  NULL,   				/* astargs */
//					  0,				/* ? */
//					  0); 				/* ?  */
	if ( status < 0)
	{
		printk( "dlmlock : lock error, status: %i", status);
	}
	return insert_key(create_key(key, value));
}


