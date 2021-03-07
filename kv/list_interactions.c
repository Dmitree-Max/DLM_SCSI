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

struct dlm_block* get_block_by_name(char* name)
{
	printk("dlm : get_block_by_name: looking for: %s", name);
	struct key_node* cur_node;

	cur_node = key_head;
	while (cur_node != NULL)
	{
		printk("dlm : get_block_by_name: have: %s", cur_node->dlm_block->name);

		if (strcmp(cur_node->dlm_block->name, name) == 0)
		{
			return cur_node->dlm_block;
		}
		cur_node = cur_node -> next;
	}
	return NULL;
}

int update_or_add_dlm_block(struct dlm_block* block)
{
	struct key_node* cur_node;
	int error;

	cur_node = key_head;
	printk("dlm : update_or_add_dlm_block: 1");
	while (cur_node != NULL)
	{
		if (strcmp(cur_node->dlm_block->name, block->name) == 0)
		{
			cur_node->dlm_block = copy_block(block);
			return 0;
		}
		cur_node = cur_node -> next;
	}
	printk("dlm : update_or_add_dlm_block: 2");
	error = insert_dlm_block(block);
	printk("dlm : update_or_add_dlm_block: 3");
	return error;
}

int insert_dlm_block(struct dlm_block* block)
{
	struct dlm_block* new_block = copy_block(block);
	struct key_node* new_node = create_key_node_with_block(new_block);
	return insert_key_node(new_node);
}


void print_all_blocks(void)
{
	struct key_node* cur_node;

	cur_node = key_head;
	while (cur_node != NULL)
	{
		printk("dlm : print_all_blocks: name: %s", cur_node->dlm_block->name);
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



