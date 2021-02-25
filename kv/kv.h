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

#include "global_vars.h"
#include "structures.h"
#include "list_interactions.h"
#include "buffer_interactions.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define KV_BUFFER_SIZE 1024




int register_key(char *key, char* value);
int lock_key(char *key);
int unlock_key(char *key);
int get_values(char *device_buffer, size_t size_in_min_buffer_left, loff_t *ppos);

void add_test_data(void);


#endif
