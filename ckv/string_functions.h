#ifndef STRING_FUNCTIONS
#define STRING_FUNCTIONS

#include <linux/kernel.h>

void replace_chars(char* str, char replace_from, char replace_to);
char* split(char* str, char* delimiter, char** next, int max_length);

#endif
