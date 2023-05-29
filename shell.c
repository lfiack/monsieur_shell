/**
 * @file shell.c
 * @brief Implementation file for the shell module.
 *
 * This file contains the implementation of the shell module, which provides a command-line interface.
 */

#include "shell.h"
#include <stdio.h>

/**
 * @brief Checks if a character is valid for a shell command.
 *
 * This function checks if a character is valid for a shell command. Valid characters include alphanumeric characters and spaces.
 *
 * @param c The character to check.
 * @return 1 if the character is valid, 0 otherwise.
 */
static int is_character_valid(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == ' ');
}

/**
 * @brief Help command implementation.
 *
 * This function is the implementation of the help command. It displays the list of available shell commands and their descriptions.
 *
 * @param h_shell The pointer to the shell instance.
 * @param argc The number of command arguments.
 * @param argv The array of command arguments.
 * @return 0 on success.
 */
static int sh_help(h_shell_t* h_shell, int argc, char** argv)
{
    int i;
    for (i = 0; i < h_shell->func_list_size; i++)
    {
        int size;
        size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "%c: %s\r\n", h_shell->func_list[i].c, h_shell->func_list[i].description);
        h_shell->drv.transmit(h_shell->print_buffer, size);
    }
    return 0;
}

/**
 * @brief Initializes the shell instance.
 *
 * This function initializes the shell instance by setting up the internal data structures and registering the help command.
 *
 * @param h_shell The pointer to the shell instance.
 */
void shell_init(h_shell_t* h_shell)
{
    int size = 0;

    h_shell->func_list_size = 0;

    size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "\r\n\r\n===== Monsieur Shell v0.2.1 =====\r\n");
    h_shell->drv.transmit(h_shell->print_buffer, size);

    shell_add(h_shell, 'h', sh_help, "Help");
}

/**
 * @brief Adds a shell command to the instance.
 *
 * This function adds a shell command to the shell instance.
 *
 * @param h_shell The pointer to the shell instance.
 * @param c The character trigger for the command.
 * @param pfunc Pointer to the function implementing the command.
 * @param description The description of the command.
 * @return 0 on success, or a negative error code on failure.
 */
int shell_add(h_shell_t* h_shell, char c, shell_func_pointer_t pfunc, char* description)
{
    if (is_character_valid(c))
    {
        if (h_shell->func_list_size < SHELL_FUNC_LIST_MAX_SIZE)
        {
            h_shell->func_list[h_shell->func_list_size].c = c;
            h_shell->func_list[h_shell->func_list_size].func = pfunc;
            h_shell->func_list[h_shell->func_list_size].description = description;
            h_shell->func_list_size++;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Executes a shell command.
 *
 * This function executes a shell command based on the input buffer.
 *
 * @param h_shell The pointer to the shell instance.
 * @param buf The input buffer containing the command.
 * @return 0 on success, or a negative error code on failure.
 */
static int shell_exec(h_shell_t* h_shell, char* buf)
{
    int i;

    char c = buf[0];

    int argc;
    char* argv[SHELL_ARGC_MAX];
    char* p;

    for (i = 0; i < h_shell->func_list_size; i++)
    {
        if (h_shell->func_list[i].c == c)
        {
            argc = 1;
            argv[0] = buf;

            for (p = buf; *p != '\0' && argc < SHELL_ARGC_MAX; p++)
            {
                if (*p == ' ')
                {
                    *p = '\0';
                    argv[argc++] = p + 1;
                }
            }

            return h_shell->func_list[i].func(h_shell, argc, argv);
        }
    }

    int size;
    size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "%c: no such command\r\n", c);
    h_shell->drv.transmit(h_shell->print_buffer, size);
    return -1;
}

/**
 * @brief Runs the shell.
 *
 * This function runs the shell, processing user commands.
 *
 * @param h_shell The pointer to the shell instance.
 * @return Never returns, it's an infinite loop.
 */
int shell_run(h_shell_t* h_shell)
{
    int reading = 0;
    int cmd_buffer_index = 0;

    while (1)
    {
        h_shell->drv.transmit("> ", 2);
        reading = 1;

        while (reading)
        {
            char c;
            h_shell->drv.receive(&c, 1);
            int size;

            switch (c)
            {
                // Process RETURN key
                case '\r':
                    //case '\n':
                    size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "\r\n");
                    h_shell->drv.transmit(h_shell->print_buffer, size);
                    h_shell->cmd_buffer[cmd_buffer_index++] = 0; // Add '\0' char at the end of the string
                    size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, ":%s\r\n", h_shell->cmd_buffer);
                    h_shell->drv.transmit(h_shell->print_buffer, size);
                    reading = 0;         // Exit read loop
                    cmd_buffer_index = 0; // Reset buffer
                    break;
                // Backspace
                case '\b':
                    if (cmd_buffer_index > 0) // Is there a character to delete?
                    {
                        h_shell->cmd_buffer[cmd_buffer_index] = '\0'; // Removes character from the buffer
                                                                      // '\0' character is required for shell_exec to work
                        cmd_buffer_index--;

                        h_shell->drv.transmit("\b \b", 3); // "Deletes" the character on the terminal
                    }
                    break;
                // Other characters
                default:
                    // Only store characters if the buffer has space
                    if (cmd_buffer_index < SHELL_CMD_BUFFER_SIZE)
                    {
                        if (is_character_valid(c))
                        {
                            h_shell->drv.transmit(&c, 1);
                            h_shell->cmd_buffer[cmd_buffer_index++] = c; // Store
                        }
                    }
            }
        }
        shell_exec(h_shell, h_shell->cmd_buffer);
    }
    return 0;
}
