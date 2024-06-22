#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define STDIN 0  // Standard input file descriptor
#define STDOUT 1 // Standard output file descriptor
#define STDERR 2 // Standard error output file descriptor

#define MAX_ARGS_BYTES 512 // Maximum number of bytes for command line arguments

/*
 * Struct Definitions
 */
typedef struct alias_q
{
  char *key;
  char *value;
  struct alias_q *next;
} alias_q;

alias_q *header_alias_q;

/*
 * Function Declarations
 */
int env_cmd_check(char *args[], int size);
int exit_cmd(char *args[], int size);
int export_cmd(char *args[], int size);
int unset_cmd(char *args[], int size);
int redirection_check(char *args[], int size);
void exec_command(char *cmd);
int builtin_command_check(char *args[], int size);
int alias_cmd(char *args[], int size);
alias_q *create_alias(char *key, char *value);
alias_q *get_alias(char *key);
int add_alias(alias_q *a);

/*
 * Function Implementations
 */

/**
 * env_cmd_check - checks and handles environment variable substitution in command arguments
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: 1 if successful, 0 otherwise
 */
int env_cmd_check(char *args[], int size)
{
  for (int i = 0; i < size; i++)
  {
    // check if the argument starts with '$'
    if (args[i][0] == '$')
    {
      char *value = getenv(args[i] + 1);
      if (value == NULL)
      {
        return 0;
      }
      args[i] = value;
    }
  }
  return 1;
}

/**
 * exit_cmd - handles the exit command
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: 1 if the command is exit, 0 otherwise
 */
int exit_cmd(char *args[], int size)
{
  // check if the command is 'exit'
  if (strcmp(args[0], "exit") == 0)
  {
    exit(0);
  }
  return 0;
}

/**
 * export_cmd - handles the export command
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: 1 if the command is export and successful, 0 otherwise
 */
int export_cmd(char *args[], int size)
{
  // check if the command is 'export' with the correct number of arguments
  if (size == 2 && strcmp(args[0], "export") == 0)
  {
    char *key = strtok(args[1], "=");
    char *value = strtok(NULL, "=");
    if (key != NULL && value != NULL)
    {
      setenv(key, value, 1);
      return 1;
    }
    else
    {
      write(STDERR, "Error: Invalid format. Use export KEY=VALUE\n", 45);
      return 0;
    }
  }
  return 0;
}

/**
 * unset_cmd - handles the unset command
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: 1 if the command is unset and successful, 0 otherwise
 */
int unset_cmd(char *args[], int size)
{
  // check if the command is 'unset'
  if (strcmp(args[0], "unset") == 0)
  {
    int unpresent_f = 0;
    for (int i = 1; i < size; i++)
    {
      if (getenv(args[i]) == NULL)
      {
        unpresent_f = 1;
      }
      if (unsetenv(args[i]) < 0)
      {
        write(STDERR, "Error: fail to unset env\n", 26);
      }
    }
    if (unpresent_f)
    {
      write(STDERR, "unset: environment variable not present\n", 41);
    }
    return 1; // exec success
  }
  return 0;
}

/**
 * redirection_check - checks for output redirection
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: file descriptor if redirection is found, -1 if not, -2 on error
 */
int redirection_check(char *args[], int size)
{
  // check if the first argument is '>'
  if (strlen(args[0]) == 1 && args[0][0] == '>')
  {
    write(STDERR, "Redirection error\n", 18);
    return -2;
  }

  int redir_f = 0;
  for (int i = 0; i < size; i++)
  {
    // check for '>' in the arguments
    if (strcmp(args[i], ">") == 0)
    {
      if (redir_f)
      {
        write(STDERR, "Redirection error\n", 18);
        return -2;
      }
      redir_f = 1;

      if (i + 1 >= size || args[i + 1][0] == '>')
      {
        write(STDERR, "Redirection error\n", 18);
        return -2;
      }

      if (i + 2 != size)
      {
        write(STDERR, "Redirection error\n", 18);
        return -2;
      }

      // check for multiple '>' signs
      for (int j = i + 1; j < size; j++)
      {
        if (args[j][0] == '>')
        {
          write(STDERR, "Redirection error\n", 18);
          return -2;
        }
      }

      int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0)
      {
        write(STDERR, "Error: opening file\n", 20);
        return -2;
      }

      args[i] = NULL;
      return fd;
    }
  }

  return -1;
}

/**
 * create_alias - creates a new alias
 * @key: alias name
 * @value: alias value
 *
 * Return: pointer to the new alias
 */
alias_q *create_alias(char *key, char *value)
{
  alias_q *alias = calloc(1, sizeof(alias_q));
  if (!alias)
  {
    write(STDERR, "Error: allocating memory\n", 26);
    exit(1);
  }

  alias->key = (char *)malloc(strlen(key) + 1);
  if (!alias->key)
  {
    write(STDERR, "Error: Failed to allocate memory for key\n", 41);
    free(alias);
    exit(1);
  }
  strcpy(alias->key, key);

  alias->value = (char *)malloc(strlen(value) + 1);
  if (!alias->value)
  {
    write(STDERR, "Error: Failed to allocate memory for value\n", 43);
    free(alias->key);
    free(alias);
    exit(1);
  }
  strcpy(alias->value, value);
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
    // check if the current alias matches the key
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
  if (strcmp(args[0], "alias") != 0)
  {
    alias_q *curr = header_alias_q;
    while (curr != NULL)
    {
      // check if the current alias matches the first argument
      if (strcmp(args[0], curr->key) == 0)
      {
        char *cmd = (char *)malloc(strlen(curr->value) + 1);
        if (cmd == NULL)
        {
          write(STDERR, "Failed to allocate memory for command\n", 38);
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

  // if no additional arguments, print all aliases
  if (size == 1)
  {
    alias_q *curr = header_alias_q;
    while (curr != NULL)
    {
      int len = snprintf(buffer, sizeof(buffer), "%s='%s'\n", curr->key, curr->value);
      write(STDOUT, buffer, len);
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
      // check if the current alias matches the second argument
      if (strcmp(args[1], curr->key) == 0)
      {
        int len = snprintf(buffer, sizeof(buffer), "%s='%s'\n", curr->key, curr->value);
        write(STDOUT, buffer, len);
        f_flag = 1;
        break;
      }
      curr = curr->next;
    }

    if (!f_flag)
    {
      write(STDERR, "Error: alias not found\n", 23);
    }
    return f_flag;
  }
  else
  {
    char *key = args[1];
    char value[MAX_ARGS_BYTES] = {'\0'}; // the value could be max args - 1

    // concatenate the remaining arguments to form the alias value
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
    // TODO: free later
    alias_q *alias = create_alias(key, value);

    alias_q *check = get_alias(key);
    if (check != NULL)
    {
      char *new_value = strdup(value);
      if (new_value == NULL)
      {
        return 0;
      }
      free(check->value);
      check->value = new_value;
    }
    else
    {
      add_alias(alias);
    }
    return 1;
  }
}

/**
 * builtin_command_check - checks if the command is a built-in command
 * @args: array of command arguments
 * @size: number of arguments
 *
 * Return: 1 if the command is a built-in command, 0 otherwise
 */
int builtin_command_check(char *args[], int size)
{
  return alias_cmd(args, size) ||
         exit_cmd(args, size) ||
         export_cmd(args, size) ||
         unset_cmd(args, size);
}

/**
 * exec_command - executes a command
 * @cmd: the command to execute
 */
void exec_command(char *cmd)
{
  char *args[MAX_ARGS_BYTES];
  int i = 0;
  char *saveptr;
  char *token = strtok_r(cmd, " ", &saveptr);
  while (token != NULL)
  {
    args[i] = token;
    token = strtok_r(NULL, " ", &saveptr);
    i++;
  }
  args[i] = NULL;

  // check for environment variable substitution
  if (!env_cmd_check(args, i))
  {
    return;
  }

  // check for output redirection
  int fd = redirection_check(args, i);

  if (fd == -2)
  {
    return;
  }

  // check for built-in commands
  if (!builtin_command_check(args, i))
  {
    int pid = fork();
    if (pid == 0)
    {
      // handle output redirection
      if (fd != -1)
      {
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
          write(STDERR_FILENO, "Error: redirection failed\n", 26);
          exit(1);
        }
        close(fd);
      }
      execvp(args[0], args);
      write(STDERR_FILENO, "Error: command not found\n", 25);
      exit(1);
    }
    else
    {
      waitpid(pid, NULL, 0);
      if (fd != -1)
      {
        close(fd);
      }
    }
  }
}

/**
 * main - entry point of the shell program
 * @argc: argument count
 * @argv: argument vector
 *
 * Return: 0 on success, 1 on error
 */
int main(int argc, char *argv[])
{
  if (argc == 1)
  {
    char cmd_buffer[MAX_ARGS_BYTES + 1];
    while (1)
    {
      write(STDOUT_FILENO, "wish> ", 6);
      if (fgets(cmd_buffer, MAX_ARGS_BYTES, stdin) == NULL)
      {
        write(STDERR_FILENO, "Error: reading command\n", 21);
        exit(1);
      }
      cmd_buffer[strcspn(cmd_buffer, "\n")] = 0;
      cmd_buffer[MAX_ARGS_BYTES] = '\0';

      exec_command(cmd_buffer);
    }
  }
  else if (argc == 2)
  {
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
      write(STDERR_FILENO, "Error: opening file\n", 20);
      exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    FILE *file = fdopen(fd, "r");
    if (!file)
    {
      write(STDERR_FILENO, "Error: fdopen\n", 14);
      close(fd);
      exit(1);
    }

    while ((nread = getline(&line, &len, file)) != -1)
    {
      if (nread > 0 && line[nread - 1] == '\n')
      {
        line[nread - 1] = '\0';
      }

      char *line_copy = strdup(line);
      if (line_copy == NULL)
      {
        write(STDERR, "Error: strdup failed\n", 21);
        fclose(file);
        close(fd);
        exit(1);
      }

      if (strlen(line_copy) == 0)
      {
        write(STDOUT_FILENO, "\n", 1);
      }
      else
      {
        write(STDOUT_FILENO, line_copy, strlen(line_copy));
        write(STDOUT_FILENO, "\n", 1);
        exec_command(line_copy);
      }

      free(line_copy);
    }

    if (ferror(file))
    {
      write(STDERR_FILENO, "Error: reading file\n", 20);
    }

    free(line);
    fclose(file);
  }
  return 0;
}
