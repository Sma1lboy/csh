#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define MAX_ARGS_BYTES 512

/*
built int cmds
*/

int env_cmd_check(char *args[], int size)
{
  for (int i = 1; i < size; i++)
  {
    if (args[i][0] == '$')
    {
      char *value = getenv(args[i] + 1);
      if (value != NULL)
      {
        args[i] = value;
      }
    }
  }
  return 0;
}
int exit_cmd(char *args[], int size)
{

  if (strcmp(args[0], "exit") == 0)
  {
    exit(0);
  }
  return 0;
}
int export_cmd(char *args[], int size)
{
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

int unset_cmd(char *args[], int size)
{
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
 *
 * Multiple redirection operators,
 * starting with a redirection sign,
 * multiple files to the right of the redirection sign,
 * not specifying an output file
 * are all error
 * Print Redirection error\n to stderr.
 */
int redirection_check(char *args[], int size)
{
  if (strlen(args[0]) == 1 && args[0][0] == '>')
  {
    write(STDERR, "Redirection error\n", 18);
    return -1;
  }

  int redir_f = 0;
  for (int i = 0; i < size; i++)
  {
    if (strcmp(args[i], ">") == 0)
    {
      if (redir_f)
      {
        write(STDERR, "Redirection error\n", 18);
        return -1;
      }
      redir_f = 1;

      if (i + 1 >= size || args[i + 1][0] == '>')
      {
        write(STDERR, "Redirection error\n", 18);
        return -1;
      }

      for (int j = i + 1; j < size; j++)
      {
        if (args[j][0] == '>')
        {
          write(STDERR, "Redirection error\n", 18);
          return -1;
        }
      }

      int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0)
      {
        write(STDERR, "Error: opening file\n", 20);
        return -1;
      }

      args[i] = NULL;
      return fd;
    }
  }

  return -1;
}

typedef struct alias_q
{
  char *key;
  char *value;
  struct alias_q *next;
} alias_q;

alias_q *header_alias_q;

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
    perror("Failed to allocate memory for key");
    free(alias);
    exit(1);
  }
  strcpy(alias->key, key);

  alias->value = (char *)malloc(strlen(value) + 1);
  if (!alias->value)
  {
    perror("Failed to allocate memory for value");
    free(alias->key);
    free(alias);
    exit(1);
  }
  strcpy(alias->value, value);
  return alias;
}
void exec_command(char *cmd);

int alias_cmd(char *args[], int size)
{
  if (strcmp(args[0], "alias") != 0)
  {
    alias_q *curr = header_alias_q;
    while (curr != NULL)
    {
      if (strcmp(args[0], curr->key) == 0)
      {
        char *cmd = curr->value;
        exec_command(cmd);
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
      int len = snprintf(buffer, sizeof(buffer), "%s=%s\n", curr->key, curr->value);
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
      if (strcmp(args[1], curr->key) == 0)
      {
        int len = snprintf(buffer, sizeof(buffer), "%s=%s\n", curr->key, curr->value);
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

    for (int i = 2; i < size && args[i] != NULL; i++)
    {
      strcat(value, args[i]);
      strcat(value, " ");
    }
    printf("log: key: %s, value: %s\n", key, value);

    alias_q *alias = create_alias(key, value);
    alias->next = header_alias_q;
    header_alias_q = alias;
    printf("log: alias added\n");

    return 1;
  }
}

int builtin_command_check(char *args[], int size)
{
  return alias_cmd(args, size) ||
         exit_cmd(args, size) ||
         export_cmd(args, size) ||
         unset_cmd(args, size);
}

void exec_command(char *cmd)
{

  /*
   * parsing first cmd token
   */
  char *args[MAX_ARGS_BYTES];
  int i = 0;
  char *token = strtok(cmd, " ");
  while (token != NULL)
  {
    args[i] = token;
    token = strtok(NULL, " \n");
    i++;
  }
  // using NULL to indicate end of arguments
  // TODO: if args out of bounds, print error
  args[i] = NULL;

  // check if there is env $ call
  env_cmd_check(args, i);

  int fd = redirection_check(args, i);

  // built in cmds check
  if (!builtin_command_check(args, i))
  {

    int pid = fork();
    if (pid == 0)
    {
      if (fd != -1)
      {
        if (dup2(fd, STDOUT) < 0)
        {
          write(STDERR, "Error: redirection failed\n", 26);
          exit(1);
        }
        close(fd);
      }

      execvp(args[0], args);
      write(STDERR, "Error: command not found\n", 26);
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

int main(int argc, char *argv[])
{
  // interactive mode
  if (argc == 1)
  {
    char cmd_buffer[MAX_ARGS_BYTES + 1];
    while (1)
    {
      write(STDOUT, "wish> ", 6);
      if (fgets(cmd_buffer, MAX_ARGS_BYTES, stdin) == NULL)
      {
        write(STDERR, "Error: reading command\n", 22);
        exit(1);
      }
      // remove newline character
      cmd_buffer[strcspn(cmd_buffer, "\n")] = 0;
      // remove extra characters
      cmd_buffer[MAX_ARGS_BYTES] = '\0';

      exec_command(cmd_buffer);
    }
  }
  else
  {
  }
  return 0;
}