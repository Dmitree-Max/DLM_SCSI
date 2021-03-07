#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <linux/slab.h> 
#include <linux/dlm.h>

#include "global_vars.h"

struct key_value
{
    char* key;
    char* value;
};

struct lock
{
    struct key_value* key;
    char* owner;
};

struct key_node
{
    struct key_node* next;
    struct key_value* data;
    struct dlm_block* dlm_block;
};

struct lock_node
{
    struct lock_node* next;
    struct lock* data;
};

struct dlm_block
{
	struct dlm_lksb* lksb;
	char* lvb;
	char* name;
	int lock_type; // 0 for no lock
//	__u32 lock_id;
};


// this structure is used in bast to pass new block and id of node to update
struct dlm_block_extended
{
	struct dlm_block* dlm_block;
	char* node_id;
};

struct key_value* create_key(char* key, char* value);

struct key_node* create_key_node(struct key_value* key);
struct key_node* create_key_node_with_block(struct dlm_block* block);
struct lock_node* create_lock_node(struct lock* lock);
void delete_lock_node(struct lock_node* node);

void delete_lock(struct lock* lock);
void remove_lock(struct lock* lock);
struct lock* create_lock(struct key_value* key, char* owner);
struct dlm_block* create_dlm_block(char* key, char* value);
struct dlm_block* copy_block(struct dlm_block* block);

#endif
