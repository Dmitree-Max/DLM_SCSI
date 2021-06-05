#ifndef LIST_INTERACTIONS
#define LIST_INTERACTIONS

#include <linux/kernel.h>

#include "global_vars.h"
#include "structures.h"

// returns first element with provided key, or NULL
struct key_value *find_key(const char *key_name);

// returns first node element with provided key, or NULL
struct key_node *find_key_node(const char *key_name);

// inserts new element in the end of list
int insert_key(struct key_value *key);

// inserts new element in the end of list
int insert_lock(struct lock *lock);

// returns true if there is lock with such name
bool is_there_lock(const char *key_name);

// removes key, if it exists. Otherwise returns -1.
int remove_key(const char *key);

// returns first lock with provided key, or NULL
struct lock *find_lock(const char *key_name);

// if key exists, change it`s value by allocating new memory (also frees last value memory)
// otherwise creates new key with allocating memory
// returns 0, if there is no error
int update_or_add_key(const char *key, const char *value);

// if there is key with name key, deletes it from list, frees memory, returns 0
// otherwise returns -1
int remove_lock(const char *key);

// returns true, if there is key with same key name
bool is_there_such_key(const char *key_name);

// prints all blocks info
void print_all_blocks(void);
#endif
