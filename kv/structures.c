#include "structures.h"

struct key_node* create_key_node(struct key_value* key)
{
	struct key_node* new_node =  kmalloc(sizeof(struct key_node), GFP_KERNEL);
	struct dlm_block* new_block = create_dlm_block(key->key, key->value);

	new_node -> data = key;
	new_node -> next = NULL;
	new_node -> dlm_block = new_block;
	return new_node;
}

struct key_node* create_key_node_with_block(struct dlm_block* block)
{
	struct key_node* new_node =  kmalloc(sizeof(struct key_node), GFP_KERNEL);

	char* key = block->name;
	char* value = block->lksb->sb_lvbptr;

	new_node -> data = create_key(key, value);
	new_node -> next = NULL;
	new_node -> dlm_block = block;
	return new_node;
}

void delete_lock(struct lock* lock)
{
	kfree(lock->owner);
	kfree(lock);
}

void delete_lock_node(struct lock_node* node)
{
	delete_lock(node->data);
	kfree(node);
}


void remove_lock(struct lock* lock)
{
	struct lock_node* temp;
	struct lock_node* cur_node;
	cur_node = lock_head;
	if (cur_node -> data == lock)
	{
		lock_head = cur_node->next;
		delete_lock_node(cur_node);
		return;
	}

	while (cur_node != NULL)
	{
		if (cur_node->next != NULL)
		{
			if (cur_node->next->data == lock)
			{
				temp = cur_node->next;
				if (cur_node->next == lock_tail)
				{
					lock_tail = cur_node;
				}
				cur_node->next = cur_node->next->next;
				delete_lock_node(temp);
			}
		}
		cur_node = cur_node->next;
	}
}


struct lock* create_lock(struct key_value* key, char* owner)
{
	 int owner_length;
	 char* owner_memery;
     struct lock* new_lock;

     new_lock = kmalloc(sizeof(struct lock), GFP_KERNEL);

     owner_length = strlen(owner) + 1;
     owner_memery = kmalloc(sizeof(char) * owner_length, GFP_KERNEL);
     memcpy(owner_memery, owner, sizeof(char) * owner_length);
     new_lock -> owner = owner_memery;
     new_lock -> key   = key;

     return new_lock;
}


struct dlm_block* create_dlm_block(char* key, char* value)
{
	struct dlm_lksb* new_lksb = kmalloc(sizeof(struct dlm_lksb), GFP_KERNEL);
	new_lksb->sb_status = 0;
	new_lksb->sb_lkid = 0;
	new_lksb->sb_lvbptr = value;
	struct dlm_block* new_block = kmalloc(sizeof(struct dlm_block), GFP_KERNEL);
	new_block->lvb  = value;
	printk("kv structures : create_dlm_block: with name: %s", key);
	new_block->name = key;
	new_block->lksb = new_lksb;
	new_block->lock_type = 0;

	return new_block;
}


struct dlm_block* copy_block(struct dlm_block* block)
{

	char* newname = (char*) kmalloc(strlen(block->name) * sizeof(char), GFP_KERNEL);
	char* newlvb  = (char*) kmalloc(strlen(block->lksb->sb_lvbptr) * sizeof(char), GFP_KERNEL);

	strcpy(newname, block->name);
	strcpy(newlvb, block->lksb->sb_lvbptr);
	return create_dlm_block(newname, newlvb);
}

struct lock_node* create_lock_node(struct lock* lock)
{
	struct lock_node* new_node =  kmalloc(sizeof(struct lock_node), GFP_KERNEL);
	new_node -> data = lock;
	new_node -> next = NULL;
	return new_node;
}


char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}


struct key_value* create_key(char* key, char* value)
{
	 int key_length, value_length;
	 char* key_memory;
	 char* value_memeroy;
     struct key_value* new_key;

     new_key = kmalloc(sizeof(struct key_value), GFP_KERNEL);
     replace_char(key, '\n', ' ');
     replace_char(value, '\n', ' ');

     key_length = strlen(key) + 1;
     key_memory = kmalloc(sizeof(char) * key_length, GFP_KERNEL);
     memcpy(key_memory, key, sizeof(char) * key_length);
     new_key -> key = key_memory;

     value_length = strlen(value) + 1;
     value_memeroy = kmalloc(sizeof(char) * value_length, GFP_KERNEL);
     memcpy(value_memeroy, value, sizeof(char) * value_length);
     new_key -> value = value_memeroy;


     return new_key;
}

