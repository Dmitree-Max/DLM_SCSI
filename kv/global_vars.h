#ifndef GLOBAL_VARS

#define GLOBAL_VARS

#define PR_DLM_LVB_LEN 256
#define FLAG_NL 1
#define FLAG_EX 6
#define KV_MAX_KEY_NAME_LENGTH 64

extern char *this_machine_id;
extern int this_node_id;

extern struct key_node *key_head;
extern struct key_node *key_tail;

extern struct lock_node *lock_head;
extern struct lock_node *lock_tail;

extern char device_buffer[];
extern char temp_buffer[];

extern int node_amount;
extern char **node_list;
extern struct dlm_block **sys_locks;

#endif
