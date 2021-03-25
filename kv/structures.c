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



struct lock* create_lock(const char* key, const char* owner)
{
	 char* new_owner;
	 char* new_key;
     struct lock* new_lock;

     new_lock = kmalloc(sizeof(struct lock), GFP_KERNEL);

     new_key = kmalloc(sizeof(char) * strlen(key), GFP_KERNEL);
     new_owner = kmalloc(sizeof(char) * strlen(owner), GFP_KERNEL);

     strcpy(new_owner, owner);
     strcpy(new_key, key);

     new_lock -> owner = new_owner;
     new_lock -> key   = new_key;
     return new_lock;
}


struct dlm_block* create_dlm_block(const char* key, const char* value)
{
	struct dlm_lksb* new_lksb ;
	char* newkey;
	char* newvalue;
	struct dlm_block* new_block;

	new_lksb = kmalloc(sizeof(struct dlm_lksb), GFP_KERNEL);
	newkey = (char*) kmalloc(strlen(key) * sizeof(char), GFP_KERNEL);
	strcpy(newkey, key);

	newvalue = (char*) kmalloc(strlen(value) * sizeof(char), GFP_KERNEL);
	strcpy(newvalue, value);

	new_lksb->sb_status = 0;
	new_lksb->sb_lkid = 0;
	new_lksb->sb_lvbptr = newvalue;

	new_block = kmalloc(sizeof(struct dlm_block), GFP_KERNEL);
	printk("kv structures : create_dlm_block: with name: %s", key);
	new_block->name = newkey;
	new_block->lksb = new_lksb;
	new_block->lock_type = 0;

	return new_block;
}


struct dlm_block* copy_block(struct dlm_block* block)
{
	char* key;
	struct dlm_lksb* new_lksb;
	char* newkey;
	char* newvalue;
	struct dlm_block* new_block;

	key = block->name;
	new_lksb = kmalloc(sizeof(struct dlm_lksb), GFP_KERNEL);
	newkey = (char*) kmalloc(strlen(block->name) * sizeof(char), GFP_KERNEL);
	strcpy(newkey, block->name);

	newvalue = (char*) kmalloc(strlen(block->lksb->sb_lvbptr) * sizeof(char), GFP_KERNEL);
	strcpy(newvalue, block->lksb->sb_lvbptr);

	new_lksb->sb_status = block->lksb->sb_status;
	new_lksb->sb_lkid = block->lksb->sb_lkid;
	new_lksb->sb_lvbptr = newvalue;
	new_lksb->sb_flags = 0;
	new_block = kmalloc(sizeof(struct dlm_block), GFP_KERNEL);
	printk("kv structures : copy_block: with name: %s", block->name);
	new_block->name = newkey;
	new_block->lksb = new_lksb;
	new_block->lock_type = 0;

	return new_block;
}

struct lock_node* create_lock_node(struct lock* lock)
{
	struct lock_node* new_node =  kmalloc(sizeof(struct lock_node), GFP_KERNEL);
	new_node -> data = lock;
	new_node -> next = NULL;
	return new_node;
}

struct update_structure* create_update_structure(const char* key, const char* value, char type)
{
	char* newkey;
	char* newvalue;
	struct update_structure* res;

	newkey = (char*) kmalloc(strlen(key) * sizeof(char), GFP_KERNEL);
	strcpy(newkey, key);

	newvalue = (char*) kmalloc(strlen(value) * sizeof(char), GFP_KERNEL);
	strcpy(newvalue, value);

    res =  kmalloc(sizeof(struct update_structure), GFP_KERNEL);
	res->key = newkey;
	res->value = newvalue;
	res->type = type;

	return res;
}

struct update_with_target* create_update_target(struct update_structure* update, int target)
{
	struct update_with_target* res =  kmalloc(sizeof(struct update_with_target), GFP_KERNEL);
	res->update = update;
	res->target_id = target;

	return res;
}



char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}


struct key_value* create_key(const char* key, const char* value)
{
	 char* new_key;
	 char* new_value;
     struct key_value* res;

     res = kmalloc(sizeof(struct key_value), GFP_KERNEL);

     new_key = kmalloc(sizeof(char) * strlen(key), GFP_KERNEL);
     strcpy(new_key, key);

     new_value = kmalloc(sizeof(char) * strlen(value), GFP_KERNEL);
     strcpy(new_value, value);

     replace_char(new_key, '\n', ' ');
     replace_char(new_value, '\n', ' ');

     res -> key = new_key;
     res -> value = new_value;

     return res;
}

