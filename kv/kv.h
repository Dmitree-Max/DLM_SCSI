#ifndef KV_H
#define KV_H

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/dlm.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/workqueue.h>

#include "global_vars.h"
#include "structures.h"
#include "list_interactions.h"
#include "buffer_interactions.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define KV_BUFFER_SIZE 1024
#define KV_MAX_KEY_NAME_LENGTH 64

#define PRE_PREFIX "pre_"
#define POST_PREFIX "post_"


int add_key(const char *key, const char* value);
int lock_key(const char *key);
int unlock_key(const char *key);

int get_values(char *device_buffer, size_t size_in_min_buffer_left, loff_t *ppos);
int dlm_init(void);
int dlm_get_lkvb(struct dlm_block* block);

void add_test_data(void);



#endif
