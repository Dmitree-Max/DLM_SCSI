#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <linux/slab.h>
#include <linux/dlm.h>

#include "global_vars.h"

struct key_value {
	char *key;
	char *value;
};

struct lock {
	char *key;
	char *owner;
};

struct key_node {
	struct key_node *next;
	struct key_value *data;
};

struct lock_node {
	struct lock_node *next;
	struct lock *data;
};

// lksb is lock status block, contains lvb
// name is the name of lock resource
// lock type is field, used to specify lock type in some cases
// compl is structure to make locking synchronous
struct dlm_block {
	struct dlm_lksb *lksb;
	char *name;
	int lock_type;		// 0 for no lock
	struct completion *compl;

};

// this structure is used in bast to pass new block and id of node to update
// this struct can be transfered to char buufer and back to put into lvb
struct update_structure {
	char type;
	char *key;
	char *value;
};

// this structure is update_structure excluded with target id. We need this structure,
// because ast function applies only 1 argument
struct update_with_target {
	struct update_structure *update;
	int target_id;
};

// creates key_value structure. Allocates it and also allocates memory for new key and value,
// so we don`t depend on their future update.
struct key_value *create_key(const char *key, const char *value);

// allocates memory only for key_node and use key itself
struct key_node *create_key_node(struct key_value *key);

// allocates memory only for lock_node and use lock itself
struct lock_node *create_lock_node(struct lock *lock);

// creates lock structure. Allocates it and also allocates memory for new key and owner,
// so we don`t depend on their future update.
struct lock *create_lock(const char *key, const char *owner);

// frees memory
void delete_lock_node(struct lock_node *node);

// frees memory
void delete_lock(struct lock *lock);

// creates new dlm block, key and value values copied to new allocated memory
struct dlm_block *create_dlm_block(const char *key, const char *value);

// creates update structure, key and value values copied to new allocated memory
struct update_structure *create_update_structure(const char *key,
						 const char *value, char type);

// creates update with target structure, update not reallocated
struct update_with_target *create_update_target(struct update_structure *update,
						int target);
#endif
