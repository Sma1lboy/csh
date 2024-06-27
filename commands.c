#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "commands.h"
#include "alias.h"
#include "redirection.h"
#include "fdwrite.h"

#define MAX_ARGS_BYTES 512 // Maximum number of bytes for command line arguments

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
      const char *error_msg = "Error: Invalid format. Use export KEY=VALUE\n";
      fdwrite(STDERR_FILENO, error_msg);
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
  if (strcmp(args[0], "unset") == 0)
  {
    int unpresent_f = 0;
    for (int i = 1; i < size; i++)
    {
      if (getenv(args[i]) == NULL)
      {
        unpresent_f = 1;
        continue;
      }
      if (unsetenv(args[i]) < 0)
      {
        const char *error_msg = "Error: fail to unset env\n";
        fdwrite(STDERR_FILENO, error_msg);
      }
    }
    if (unpresent_f)
    {
      const char *error_msg = "unset: environment variable not present\n";
      fdwrite(STDERR_FILENO, error_msg);
    }
    return 1; // exec success
  }
  return 0;
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

  if (!env_cmd_check(args, i))
  {
    return;
  }

  int fd = redirection_check(args, i);

  if (fd == -2)
  {
    return;
  }

  if (!builtin_command_check(args, i))
  {
    int pid = fork();
    if (pid == 0)
    {
      if (fd != -1)
      {
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
          const char *redir_fail_msg = "Error: redirection failed\n";
          fdwrite(STDERR_FILENO, redir_fail_msg);
          exit(1);
        }
        close(fd);
      }
      execvp(args[0], args);
      const char *cmd_not_found_msg = "Error: command not found\n";
      fdwrite(STDERR_FILENO, cmd_not_found_msg);
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
