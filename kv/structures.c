#include "structures.h"

struct key_node *create_key_node(struct key_value *key)
{
	int error = 0;
	struct key_node *new_node =
	    kmalloc(sizeof(struct key_node), GFP_KERNEL);
	if (!new_node) {
		error = -ENOMEM;
		goto out;
	}

	new_node->data = key;
	new_node->next = NULL;

out:
	if (error != 0) {
		pr_info("error: %i\n", error);
	}
	return new_node;
}

static void delete_key(struct key_value *key)
{
	kfree(key->key);
	kfree(key->value);
	kfree(key);
}

static void delete_lock(struct lock *lock)
{
	kfree(lock->owner);
	kfree(lock->key);
	kfree(lock);
}

void delete_lock_node(struct lock_node *node)
{
	delete_lock(node->data);
	kfree(node);
}

void delete_key_node(struct key_node *node)
{
	delete_key(node->data);
	kfree(node);
}

struct lock *create_lock(const char *key, const char *owner)
{
	int error = 0;
	char *new_owner;
	char *new_key;
	struct lock *new_lock;

	new_lock = kmalloc(sizeof(struct lock), GFP_KERNEL);
	if (!new_lock) {
		error = -ENOMEM;
		goto out;
	}

	new_key = kcalloc(strlen(key) + 1, sizeof(char), GFP_KERNEL);
	if (!new_key) {
		error = -ENOMEM;
		goto out;
	}

	new_owner = kcalloc(strlen(owner) + 1, sizeof(char), GFP_KERNEL);
	if (!new_owner) {
		error = -ENOMEM;
		goto out;
	}

	strcpy(new_owner, owner);
	strcpy(new_key, key);

	new_lock->owner = new_owner;
	new_lock->key = new_key;

out:
	if (error != 0) {
		pr_info("error: %i\n", error);
	}
	return new_lock;
}

struct dlm_block *create_dlm_block(const char *key, const char *value)
{
	int error = 0;

	struct dlm_lksb *new_lksb;
	char *newkey;
	char *newvalue;
	struct dlm_block *new_block;
	struct completion *compl;

	new_lksb = kmalloc(sizeof(struct dlm_lksb), GFP_KERNEL);
	if (!new_lksb) {
		error = -ENOMEM;
		goto out;
	}

	newkey = kcalloc(KV_MAX_KEY_NAME_LENGTH, sizeof(char), GFP_KERNEL);
	if (!newkey) {
		error = -ENOMEM;
		goto out;
	}

	strncpy(newkey, key, KV_MAX_KEY_NAME_LENGTH - 1);

	newvalue = kcalloc(PR_DLM_LVB_LEN, sizeof(char), GFP_KERNEL);
	if (!newvalue) {
		error = -ENOMEM;
		goto out;
	}

	strncpy(newvalue, value, PR_DLM_LVB_LEN - 1);

	compl = kmalloc(sizeof(struct completion), GFP_KERNEL);
	if (!compl) {
		error = -ENOMEM;
		goto out;
	}

	new_lksb->sb_status = 0;
	new_lksb->sb_lkid = 0;
	new_lksb->sb_flags = 0;
	new_lksb->sb_lvbptr = newvalue;

	new_block = kmalloc(sizeof(struct dlm_block), GFP_KERNEL);
	if (!new_block) {
		error = -ENOMEM;
		goto out;
	}

	printk("kv structures : create_dlm_block: with name: %s", key);
	new_block->name = newkey;
	new_block->lksb = new_lksb;
	new_block->lock_type = 0;
	new_block->compl = compl;

out:
	if (error != 0) {
		pr_info("error: %i\n", error);
	}
	return new_block;
}



struct lock_node *create_lock_node(struct lock *lock)
{
	struct lock_node *new_node =
	    kmalloc(sizeof(struct lock_node), GFP_KERNEL);
	new_node->data = lock;
	new_node->next = NULL;
	return new_node;
}

struct update_structure *create_update_structure(const char *key,
						 const char *value, char type)
{
	int error = 0;
	char *newkey;
	char *newvalue;
	struct update_structure *res;

	newkey = kcalloc(KV_MAX_KEY_NAME_LENGTH, sizeof(char), GFP_KERNEL);
	if (!newkey) {
		error = -ENOMEM;
		goto out;
	}

	strncpy(newkey, key, KV_MAX_KEY_NAME_LENGTH - 1);
	newvalue = kcalloc(KV_MAX_KEY_NAME_LENGTH, sizeof(char), GFP_KERNEL);
	if (!newvalue) {
		error = -ENOMEM;
		goto out;
	}
	strncpy(newvalue, value, KV_MAX_KEY_NAME_LENGTH - 1);
	res = kmalloc(sizeof(struct update_structure), GFP_KERNEL);
	if (!res) {
		error = -ENOMEM;
		goto out;
	}
	res->key = newkey;
	res->value = newvalue;
	res->type = type;
out:
	if (error != 0) {
		pr_info("error: %i\n", error);
	}
	return res;
}

struct update_with_target *create_update_target(struct update_structure *update,
						int target)
{
	int error = 0;

	struct update_with_target *res =
	    kmalloc(sizeof(struct update_with_target), GFP_KERNEL);
	if (!res) {
		error = -ENOMEM;
		goto out;
	}

	res->update = update;
	res->target_id = target;

out:
	if (error != 0) {
		pr_info("error: %i\n", error);
	}
	return res;
}

char *replace_char(char *str, char find, char replace)
{
	char *current_pos = strchr(str, find);
	while (current_pos) {
		*current_pos = replace;
		current_pos = strchr(current_pos, find);
	}
	return str;
}

struct key_value *create_key(const char *key, const char *value)
{
	int error = 0;
	char *new_key;
	char *new_value;
	struct key_value *res;

	res = kmalloc(sizeof(struct key_value), GFP_KERNEL);
	if (!res) {
		error = -ENOMEM;
		goto out;
	}

	new_key = kcalloc(KV_MAX_KEY_NAME_LENGTH, sizeof(char), GFP_KERNEL);

	if (!new_key) {
		error = -ENOMEM;
		goto out;
	}

	strncpy(new_key, key, KV_MAX_KEY_NAME_LENGTH - 1);

	new_value = kcalloc(KV_MAX_KEY_NAME_LENGTH, sizeof(char), GFP_KERNEL);
	if (!new_value) {
		error = -ENOMEM;
		goto out;
	}

	strncpy(new_value, value, KV_MAX_KEY_NAME_LENGTH - 1);

//	replace_char(new_key, '\n', ' ');
//	replace_char(new_value, '\n', ' ');

	res->key = new_key;
	res->value = new_value;

out:
	if (error != 0) {
		pr_info("error: %i\n", error);
	}
	return res;
}
