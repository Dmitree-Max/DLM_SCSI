#include "buffer_interactions.h"



// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
int fill_buffer_with_keys(char* buffer, int length, int* offset)
{
    int offset_controller = 0;
    struct key_node* cur_node = key_head;
    int bytes_in_buffer = 0;
	char separator[] = " = ";
	int max_bytes_to_copy;

    while (cur_node != NULL)
    {
		int key_string_length = strlen(cur_node->data->key) + strlen(cur_node->data->value) + strlen(separator) + 1; // 1 for \n
		if (offset_controller + key_string_length > *offset)
		{
			strcpy(temp_buffer, cur_node->data->key);
			strcat(temp_buffer, separator);
			strcat(temp_buffer, cur_node->data->value);
			strcat(temp_buffer, "\n");

			// not copy more then buffer
			max_bytes_to_copy = length - bytes_in_buffer;

			if (offset_controller < *offset)
			{
				strncpy(buffer, temp_buffer + (*offset - offset_controller), max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer + (*offset - offset_controller)), max_bytes_to_copy);
			}
			else
			{
				strncpy(buffer + bytes_in_buffer, temp_buffer, max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer), max_bytes_to_copy);
			}

		}
		offset_controller += key_string_length;
        cur_node = cur_node->next;

        // overload of buffer
        if (bytes_in_buffer > length)
        {
            printk(KERN_INFO "charDev : Error! Buffer overload\n");
        }

        // full buffer
        if (bytes_in_buffer == length)
        {
        	break;
        }
    }

    *offset -= offset_controller;
    *offset = max(*offset, 0);
    return bytes_in_buffer;
}



// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
int fill_buffer_with_locks(char* buffer, int length, int* offset)
{
    int offset_controller = 0;
    struct lock_node* cur_node = lock_head;
    int bytes_in_buffer = 0;
	char separator1[] = " (";
	char separator2[] = ")";
	int max_bytes_to_copy;
	int lock_string_length;

    while (cur_node != NULL)
    {
		lock_string_length = strlen(cur_node->data->key->key) + strlen(cur_node->data->owner) +\
				strlen(separator1) + strlen(separator2) + 1; // 1 for \n
		if (offset_controller + lock_string_length > *offset)
		{
			strcpy(temp_buffer, cur_node->data->key->key);
			strcat(temp_buffer, separator1);
			strcat(temp_buffer, cur_node->data->owner);
			strcat(temp_buffer, separator2);
			strcat(temp_buffer, "\n");

			// not copy more then buffer
			max_bytes_to_copy = length - bytes_in_buffer;

			if (offset_controller < *offset)
			{
				strncpy(buffer, temp_buffer + (*offset - offset_controller), max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer + (*offset - offset_controller)), max_bytes_to_copy);
			}
			else
			{
				strncpy(buffer + bytes_in_buffer, temp_buffer, max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer), max_bytes_to_copy);
			}

		}
		offset_controller += lock_string_length;
        cur_node = cur_node->next;

        // overload of buffer
        if (bytes_in_buffer > length)
        {
            printk(KERN_INFO "charDev : Error! Buffer overload\n");
        }

        // full buffer
        if (bytes_in_buffer == length)
        {
        	break;
        }
    }

    *offset -= offset_controller;
    *offset = max(*offset, 0);
    return bytes_in_buffer;
}


int send_buffer(char *buff, int bytes_to_send, loff_t *ppos)
{
	int bytes_really_send;

	bytes_really_send = bytes_to_send - copy_to_user(buff, device_buffer, bytes_to_send);
    *ppos += bytes_really_send;
	return bytes_really_send;
}


// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
// header -- string to write
int write_header(char* buffer, int length, int *offset, char* header)
{
	int bytes_written = 0;
	char* symbol = header;
	int offset_controller = 0;
	int buffer_place = 0;

	while (*symbol != '\0')
	{
		if (offset_controller >= *offset)
		{
			buffer[buffer_place] = *symbol;
			buffer_place++;
			bytes_written++;
		}
		offset_controller++;
		if (buffer_place > length)
		{
			break;
		}
		symbol++;
	}
	*offset -= offset_controller;
    *offset = max(*offset, 0);
	return bytes_written;
}
