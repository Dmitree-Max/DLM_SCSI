#include "kv.h"


char kv_device_buffer[KV_BUFFER_SIZE];

char* this_machine_id = "default_server";



struct key_node* key_head = NULL;
struct key_node* key_tail = NULL;

struct lock_node* lock_head = NULL;
struct lock_node* lock_tail = NULL;

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



int register_key(char *key, char* value)
{
	return insert_key(create_key(key, value));
}



static void make_test_keys(void)
{
    char first_key[]             = "test_key";
    char first_key_value[]       = "secret";
    char second_key[]            = "foo";
    char second_key_value[]      = "bar";
    insert_key(create_key(first_key, first_key_value));
    insert_key(create_key(second_key, second_key_value));
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




