#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "alias.h"
#include "commands.h"
#include "fdwrite.h" // 添加对 fdwrite 函数的包含

#define MAX_ARGS_BYTES 512 // Maximum number of bytes for command line arguments

alias_q *header_alias_q = NULL;

/**
 * create_alias - creates a new alias
 * @key: alias name
 * @value: alias value
 *
 * Return: pointer to the new alias
 */
alias_q *create_alias(char *key, char *value)
{
  const char *alloc_error_msg = "Error: allocating memory\n";
  const char *key_error_msg = "Error: Failed to allocate memory for key\n";
  const char *value_error_msg = "Error: Failed to allocate memory for value\n";

  alias_q *alias = calloc(1, sizeof(alias_q));
  if (!alias)
  {
    fdwrite(STDERR_FILENO, alloc_error_msg);
    exit(1);
  }

  alias->key = strdup(key);
  if (!alias->key)
  {
    fdwrite(STDERR_FILENO, key_error_msg);
    free(alias);
    exit(1);
  }

  alias->value = strdup(value);
  if (!alias->value)
  {
    fdwrite(STDERR_FILENO, value_error_msg);
    free(alias->key);
    free(alias);
    exit(1);
  }
  return alias;
}

/**
 * get_alias - retrieves an alias by its key
 * @key: alias name
 *
 * Return: pointer to the alias if found, NULL otherwise
 */
alias_q *get_alias(char *key)
{
  alias_q *curr = header_alias_q;
  while (curr != NULL)
  {
    if (strcmp(key, curr->key) == 0)
    {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

/**
 * add_alias - adds a new alias to the alias list
 * @a: pointer to the alias to be added
 *
 * Return: 1 if successful
 */
int add_alias(alias_q *a)
{
  if (header_alias_q == NULL)
  {
    header_alias_q = a;
    return 1;
  }

  alias_q *curr = header_alias_q;
  while (curr->next != NULL)
  {
    curr = curr->next;
  }
  curr->next = a;
  return 1;
}

/**
 * alias_cmd - handles the alias command
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: 1 if the command is alias and successful, 0 otherwise
 */
int alias_cmd(char *args[], int size)
{
  const char *alias_not_found_msg = "Error: alias not found\n";
  const char *alloc_cmd_error_msg = "Failed to allocate memory for command\n";

  if (strcmp(args[0], "alias") != 0)
  {
    alias_q *curr = header_alias_q;
    while (curr != NULL)
    {
      if (strcmp(args[0], curr->key) == 0)
      {
        char *cmd = (char *)malloc(strlen(curr->value) + 1);
        if (cmd == NULL)
        {
          fdwrite(STDERR_FILENO, alloc_cmd_error_msg);
          exit(1);
        }
        strcpy(cmd, curr->value);
        exec_command(cmd);
        free(cmd);
        return 1;
      }
      curr = curr->next;
    }
    return 0;
  }

  char buffer[MAX_ARGS_BYTES];

  if (size == 1)
  {
    alias_q *curr = header_alias_q;
    while (curr != NULL)
    {
      fdwrite(STDOUT_FILENO, buffer);
      curr = curr->next;
    }
    return 1;
  }
  else if (size == 2)
  {
    int f_flag = 0;
    alias_q *curr = header_alias_q;

    while (curr != NULL)
    {
      if (strcmp(args[1], curr->key) == 0)
      {
        fdwrite(STDOUT_FILENO, buffer);
        f_flag = 1;
        break;
      }
      curr = curr->next;
    }

    if (!f_flag)
    {
      fdwrite(STDERR_FILENO, alias_not_found_msg);
    }
    return f_flag;
  }
  else
  {
    char *key = args[1];
    char value[MAX_ARGS_BYTES] = {'\0'};

    for (int i = 2; i < size && args[i] != NULL; i++)
    {
      strcat(value, args[i]);
      strcat(value, " ");
    }
    int len = strlen(value);
    if (len > 0 && value[len - 1] == ' ')
    {
      value[len - 1] = '\0';
    }
    alias_q *alias = create_alias(key, value);

    alias_q *check = get_alias(key);
    if (check != NULL)
    {
      char *new_value = strdup(value);
      if (new_value == NULL)
      {
        free(alias->key);
        free(alias->value);
        free(alias);
        return 0;
      }
      free(check->value);
      check->value = new_value;
      free(alias->key);
      free(alias->value);
      free(alias);
    }
    else
    {
      add_alias(alias);
    }
    return 1;
  }
}
