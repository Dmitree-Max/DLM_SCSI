#ifndef COMMANDS_H
#define COMMANDS_H

#include <linux/kernel.h>
    
#include "../kv/kv.h"
#include "string_functions.h"
     
// reads command from buffer, parses and executes it
// buff - buffer with command
// length - length of command and arguments in buffer
// returns -1 if error or no such command, or code of command execution
int make_command(char *buff, int length);
   
#endif	/*  */
