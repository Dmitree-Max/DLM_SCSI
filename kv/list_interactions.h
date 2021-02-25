#ifndef LIST_INTERACTIONS
#define LIST_INTERACTIONS

#include <linux/kernel.h>


#include "global_vars.h"
#include "structures.h"


struct key_value* find_key(char* key_name);
int insert_key(struct key_value* key);
void insert_lock(struct lock* lock);
bool is_there_lock(char* key_name);
struct lock* find_lock(char* key_name);


#endif
