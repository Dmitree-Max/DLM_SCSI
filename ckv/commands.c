#include "commands.h"

int make_command(char *buff, int length)
{
	char *command;
	char *key_name;
	char *key_value;
	char *next = NULL;
	int length_left;

	length_left = length;

	command = split(buff, " ", &next, length_left);
	if (command == NULL) {
		printk(KERN_INFO "charDev : no command %s\n", key_name);
		return -1;
	}
	length_left -= next - buff;
	buff = next;
	printk(KERN_INFO "charDev : command %s\n", command);
	if (strcmp(command, "add-key") == 0) {
		key_name = split(buff, " ", &next, length_left);
		if (key_name == NULL) {
			printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}

		length_left -= next - buff;
		buff = next;
		key_value = split(buff, " ", &next, length_left);
		if (key_value == NULL) {
			printk(KERN_INFO "charDev : key value is NULL\n");
			return -1;
		}

		return kv_add_key(key_name, key_value);
	}
	if (strcmp(command, "remove-key") == 0) {
		key_name = split(buff, " ", &next, length_left);
		if (key_name == NULL) {
			printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		return kv_remove_key(key_name);
	}
	if (strcmp(command, "lock-key") == 0) {
		key_name = split(buff, " ", &next, length_left);
		if (key_name == NULL) {
			printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		return kv_lock_key(key_name);
	}
	if (strcmp(command, "unlock-key") == 0) {
		key_name = split(buff, " ", &next, length_left);
		if (key_name == NULL) {
			printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		return kv_unlock_key(key_name);
	}

	printk(KERN_INFO "charDev : no such command: %s\n", command);
	return -1;
}
