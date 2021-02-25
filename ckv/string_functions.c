#include "string_functions.h"

void replace_chars(char* str, char replace_from, char replace_to)
{
	char* symbol = str;
	while (*symbol != '\0')
	{
		if (*symbol == replace_from)
		{
			*symbol = replace_to;
		}
		symbol++;
	}
}



char* split(char* str, char* delimiter, char** next, int max_length)
{
	int state = 0; // 0 - whitespaces before token, 1 - token didnt find token
	int length = 0;
	char* symbol;
	char* result;
	symbol = str;
	while(*symbol != '\0')
	{
		if (*symbol == *delimiter)
		{
			if (state == 1)
			{
				*symbol = '\0';
				*next = (symbol + 1);
				break;
			}
		}
		else
		{
			// echo adds '\n' symbol to the end
			if (*symbol == '\n')
			{
				*symbol = *delimiter;
				continue;
			}
			else
			{
				if (state == 0)
				{
					state = 1;
					result = symbol;
				}
			}
		}

		symbol++;
		length++;
		if (length > max_length)
		{
			break;
		}
	}

	if (state == 0)
	{
		return NULL;
	}
	return result;
}
