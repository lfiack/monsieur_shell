/*
 * shell.c
 *
 *  Created on: 25 mai 2023
 *      Author: Laurent Fiack
 */

#include "shell.h"

#include <stdio.h>

static int is_character_valid(char c)
{
	// We don't want weird characters
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static int sh_help(h_shell_t * h_shell, int argc, char ** argv) {
	int i;
	for(i = 0 ; i < h_shell->func_list_size ; i++) {
		int size;
		size = snprintf (h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "%c: %s\r\n", h_shell->func_list[i].c, h_shell->func_list[i].description);
		h_shell->drv.transmit(h_shell->print_buffer, size);
	}

	return 0;
}

void shell_init(h_shell_t * h_shell) {
	int size = 0;

	h_shell->func_list_size = 0;

	size = snprintf (h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "\r\n\r\n===== Monsieur Shell v0.2 =====\r\n");
	h_shell->drv.transmit(h_shell->print_buffer, size);

	size = snprintf (h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Correction TD (v0.2.1 du coup?)\r\n");
	h_shell->drv.transmit(h_shell->print_buffer, size);

	shell_add(h_shell, 'h', sh_help, "Help");
}

int shell_add(h_shell_t * h_shell, char c, shell_func_pointer_t pfunc, char * description) {
	if (is_character_valid(c))
	{
		if (h_shell->func_list_size < SHELL_FUNC_LIST_MAX_SIZE) {
			h_shell->func_list[h_shell->func_list_size].c = c;
			h_shell->func_list[h_shell->func_list_size].func = pfunc;
			h_shell->func_list[h_shell->func_list_size].description = description;
			h_shell->func_list_size++;
			return 0;
		}
	}

	return -1;
}

static int shell_exec(h_shell_t * h_shell, char * buf) {
	int i;

	char c = buf[0];

	int argc;
	char * argv[SHELL_ARGC_MAX];
	char *p;

	for(i = 0 ; i < h_shell->func_list_size ; i++) {
		if (h_shell->func_list[i].c == c) {
			argc = 1;
			argv[0] = buf;

			for(p = buf ; *p != '\0' && argc < SHELL_ARGC_MAX ; p++){
				if(*p == ' ') {
					*p = '\0';
					argv[argc++] = p+1;
				}
			}

			return h_shell->func_list[i].func(h_shell, argc, argv);
		}
	}

	int size;
	size = snprintf (h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "%c: no such command\r\n", c);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return -1;
}

int shell_run(h_shell_t * h_shell) {
	int reading = 0;
	int cmd_buffer_index = 0;

	while (1) {
		h_shell->drv.transmit("> ", 2);
		reading = 1;

		while(reading) {
			char c;
			h_shell->drv.receive(&c, 1);
			int size;

			switch (c) {
			//process RETURN key
			case '\r':
				//case '\n':
				size = snprintf (h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "\r\n");
				h_shell->drv.transmit(h_shell->print_buffer, size);
				h_shell->cmd_buffer[cmd_buffer_index++] = 0;     //add \0 char at end of string
				size = snprintf (h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, ":%s\r\n", h_shell->cmd_buffer);
				h_shell->drv.transmit(h_shell->print_buffer, size);
				reading = 0;        //exit read loop
				cmd_buffer_index = 0;            //reset buffer
				break;
				//backspace
			case '\b':
				if (cmd_buffer_index > 0) {      //is there a char to delete?
					h_shell->cmd_buffer[cmd_buffer_index] = '\0';	// Removes character from the buffer
																	// '\0' character is required for shell_exec to work
					cmd_buffer_index--;

					h_shell->drv.transmit("\b \b", 3);	// "Deletes" the char on the terminal
				}
				break;
				//other characters
			default:
				//only store characters if buffer has space
				if (cmd_buffer_index < SHELL_CMD_BUFFER_SIZE) {
					if (is_character_valid(c))
					{
						h_shell->drv.transmit(&c, 1);
						h_shell->cmd_buffer[cmd_buffer_index++] = c; //store
					}
				}
			}
		}
		shell_exec(h_shell, h_shell->cmd_buffer);
	}
	return 0;
}
