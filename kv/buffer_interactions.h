#ifndef BUFFER_INTERACTIONS
#define BUFFER_INTERACTIONS

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>


#include "global_vars.h"
#include "structures.h"
#include "kv.h"


#define MIN(a,b) ((a) < (b) ? (a) : (b))

// fills buffer with key=value strings
// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
// returns bytes written
int fill_buffer_with_keys(char* buffer, int length, int* offset);

// fills buffer with key=value strings
// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
// returns bytes written
int fill_buffer_with_locks(char* buffer, int length, int* offset);

// fills buffer with header
// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
// header string to write
// returns bytes written
int write_header(char* buffer, int length, int *offset, const char* header);

// sends buffer to user space
// buff buffer
// bytes_to_send amount of bytes to send from buffer
// ppos number of byte to start from
// returns bytes send
int send_buffer(char *buff, int bytes_to_send, loff_t *ppos);


// writes update structure to buffer with special rules
void update_to_buffer(const struct update_structure* update, char* buffer);

// reads update structure from buffer with special rules
void update_from_buffer(struct update_structure* update, const char* buffer);

#endif
