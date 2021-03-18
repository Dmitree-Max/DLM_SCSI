#include "commands.h"

int init_flag = false;


int make_command(char* buff, int length)
{
	char* command;
	char* key_name;
	char* key_value;
	char* next = NULL;
	char* str = NULL;
    int length_left;

	if (init_flag)
	{
		dlm_init();
		init_flag = false;
	}


    str = buff;
    length_left = length;

	command = split(str, " ", &next, length_left);
	if (command == NULL)
	{
	    printk(KERN_INFO "charDev : no command %s\n", key_name);
	    return -1;
	}
	length_left -= next - str;
	str = next;
    printk(KERN_INFO "charDev : command %s\n", command);
	if (strcmp(command, "add-key") == 0)
	{
		key_name = split(str, " ", &next, length_left);
		if (key_name == NULL)
		{
		    printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		length_left -= next - str;
		str = next;
		key_value = split(str, " ", &next, length_left);
		if (key_value == NULL)
		{
		    printk(KERN_INFO "charDev : key value is NULL\n");
			return -1;
		}
		return add_key(key_name, key_value);
	}
	if (strcmp(command, "lock-key") == 0)
	{
		key_name = split(str, " ", &next, length_left);
		if (key_name == NULL)
		{
		    printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		return lock_key(key_name);
	}
	if (strcmp(command, "unlock-key") == 0)
	{
		key_name = split(str, " ", &next, length_left);
		if (key_name == NULL)
		{
		    printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		return unlock_key(key_name);
	}

    printk(KERN_INFO "charDev : no such command: %s\n", command);
	return -1;
}
