#include "list_interactions.h"


struct key_value* find_key(char* key_name)
{
	struct key_node* cur_node;

	cur_node = key_head;
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->data->key, key_name) == 0)
		{
			return cur_node->data;
		}
		cur_node = cur_node -> next;
	}
	return NULL;
}

int insert_key(struct key_value* key)
{
	struct key_node* new_node;

	if (find_key(key->key) != NULL)
	{
	    printk(KERN_INFO "charDev : Error! there is key with same name: %s\n", key->key);
	    return -1;
	}
    new_node = create_key_node(key);
    if (key_tail != NULL)
    {
       key_tail->next = new_node;
    }
    if (key_head == NULL)
    {
       key_head = new_node;
    }
    key_tail = new_node;
    return 0;
}



void insert_lock(struct lock* lock)
{
    struct lock_node* new_node = create_lock_node(lock);
    if (lock_tail != NULL)
    {
       lock_tail->next = new_node;
    }
    if (lock_head == NULL)
    {
       lock_head = new_node;
    }
    lock_tail = new_node;
    return;
}


bool is_there_lock(char* key_name)
{
	struct lock_node* cur_node = lock_head;
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->data->key->key, key_name) == 0)
		{
			return true;
		}
		cur_node = cur_node -> next;
	}
	return false;
}

struct lock* find_lock(char* key_name)
{
	struct lock_node* cur_node = lock_head;
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->data->key->key, key_name) == 0)
		{
			return cur_node->data;
		}
		cur_node = cur_node -> next;
	}
	return NULL;
}



