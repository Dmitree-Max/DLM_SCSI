#ifndef LIST_INTERACTIONS
#define LIST_INTERACTIONS

#include <linux/kernel.h>


#include "global_vars.h"
#include "structures.h"


struct key_value* find_key(char* key_name);
struct key_node* find_key_node(char* key_name);
int insert_key(struct key_value* key);
int insert_lock(struct lock* lock);
bool is_there_lock(char* key_name);
struct lock* find_lock(char* key_name);
int insert_key_node(struct key_node* new_node);
struct dlm_block* get_block_by_name(char* name);
int update_or_add_key(char* key, char* value);
int insert_dlm_block(struct dlm_block* block);
int remove_lock(char* key);
bool is_there_such_key(char* key_name);

void print_all_blocks(void);
#endif
