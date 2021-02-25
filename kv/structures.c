#include "structures.h"

struct key_node* create_key_node(struct key_value* key)
{
	struct key_node* new_node =  kmalloc(sizeof(struct key_node), GFP_KERNEL);
	new_node -> data = key;
	new_node -> next = NULL;
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

