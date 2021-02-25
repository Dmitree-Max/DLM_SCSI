#ifndef BUFFER_INTERACTIONS
#define BUFFER_INTERACTIONS

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>


#include "global_vars.h"
#include "structures.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

int fill_buffer_with_keys(char* buffer, int length, int* offset);
int fill_buffer_with_locks(char* buffer, int length, int* offset);
int send_buffer(char *buff, int bytes_to_send, loff_t *ppos);
int write_header(char* buffer, int length, int *offset, char* header);

#endif
