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

bool is_there_such_key(char* key_name)
{
	struct key_node* cur_node;

	cur_node = key_head;
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->data->key, key_name) == 0)
		{
			return true;
		}
		cur_node = cur_node -> next;
	}
	return false;
}



int update_or_add_key(char* key, char* value)
{
	struct key_node* cur_node;
	int error;
	char* new_value;

	cur_node = key_head;
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->data->key, key) == 0)
		{
			printk("dlm : update_or_add_key: key updated");
			kfree(cur_node->data->value);

			new_value = (char*) kmalloc(strlen(value) * sizeof(char), GFP_KERNEL);
			strcpy(new_value, value);

			cur_node->data->value = new_value;
			return 0;
		}
		cur_node = cur_node -> next;
	}

	error = insert_key(create_key(key, value));
	return error;
}


void print_all_blocks(void)
{
	struct key_node* cur_node;

	cur_node = key_head;
	while (cur_node != NULL)
	{
		printk("dlm : print_all_blocks: key	: %s, value: %s",
				cur_node->data->key, cur_node->data->value);
		cur_node = cur_node -> next;
	}

}


struct key_node* find_key_node(char* key_name)
{
	struct key_node* cur_node;

	cur_node = key_head;
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->data->key, key_name) == 0)
		{
			return cur_node;
		}
		cur_node = cur_node -> next;
	}
	return NULL;
}

int insert_key(struct key_value* key)
{
	struct key_node* new_node;

	printk("dlm : insert_key: inserting key");
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

int insert_key_node(struct key_node* new_node)
{
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



int insert_lock(struct lock* lock)
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
    return 0;
}


int remove_lock(char* key)
{
	struct lock_node* temp;
	struct lock_node* cur_node;

	cur_node = lock_head;
	if (strcmp(cur_node -> data -> key, key) == 0)
	{
		lock_head = cur_node->next;
		delete_lock_node(cur_node);
		return 0;
	}

	while (cur_node != NULL)
	{
		if (cur_node->next != NULL)
		{
			if (strcmp(cur_node -> data -> key, key) == 0)
			{
				temp = cur_node->next;
				if (cur_node->next == lock_tail)
				{
					lock_tail = cur_node;
				}
				cur_node->next = cur_node->next->next;
				delete_lock_node(temp);
				return 0;
			}
		}
		cur_node = cur_node->next;
	}
	return -1;
}


bool is_there_lock(char* key_name)
{
	struct lock_node* cur_node = lock_head;
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->data->key, key_name) == 0)
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
		if (strcmp(cur_node->data->key, key_name) == 0)
		{
			return cur_node->data;
		}
		cur_node = cur_node -> next;
	}
	return NULL;
}



