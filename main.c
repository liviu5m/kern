#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "utils.h"

#define BUFFER_SIZE 4096

char *builtinCmds[] = {
  "exit",
  "echo",
  "type"
};
char *pathValues[256];
int pathCount = 0;
char *currentDirectory;
char *homePath;

int isBuiltinCommand(char *command) {

  for(int i = 0; i < sizeof(builtinCmds) / sizeof(char*); i++) {
    if(strcmp(command, builtinCmds[i]) == 0) {
      printf("%s is a built-in command\n", command);
      return 1;
    }
  }

  for(int i = 0;i<pathCount;i++) {
    DIR *d = opendir(pathValues[i]);
    if(d == NULL) continue;
    struct dirent *val;
    while((val = readdir(d)) != NULL) {
      char fullPath[1024];
      if(strcmp(val->d_name, command)) continue;
      snprintf(fullPath, sizeof(fullPath), "%s/%s", pathValues[i], val->d_name);
      struct stat sb;
      if(stat(fullPath, &sb) == 0) {
        if(S_ISREG(sb.st_mode) && access(fullPath, X_OK) == 0) {
          printf("%s is %s\n", command, fullPath);
          return 2;
        }
      }
    }
    closedir(d);
  }
  return 0;
}

void getPwd(char *buffer) {
  if(getcwd(buffer, BUFFER_SIZE) == NULL)  {
    printf("Error on pwd");
  }
}

int parseCommand(char command[], char *args[]) {
  int i = 0,k = 0, counter = 0;
  bool quote = false, dquote = false;
  char *buffer = malloc(strlen(command) + 1);
  while(command[i] != '\0') {
    if(command[i] == '>') {
      if (k > 0) {
          if (i > 0 && (command[i-1] == '1' || command[i-1] == '2')) {
              buffer[k-1] = '\0';
          } else {
              buffer[k] = '\0';
          }
          if (buffer[0] != '\0') { 
              args[counter++] = strdup(buffer);
          }
          k = 0;
      }

      if (i > 0 && command[i-1] == '1') strcpy(buffer, "1>");
      else if (i > 0 && command[i-1] == '2') strcpy(buffer, "2>");
      else strcpy(buffer, ">");

      if (command[i+1] == '>') {
          strcat(buffer, ">");
          i++;
      }
      args[counter++] = strdup(buffer);
      k = 0;
  }else if(command[i] == ' ') {
      if(!quote && !dquote) {
        if(k == 0) {
          i++;
          continue;
        }
        if(k != 0) {
          buffer[k] = '\0';
          args[counter] = strdup(buffer);
          k = 0;
          counter++;
        }
      }else buffer[k++] = command[i];
    }else if(command[i] == '\"') {
      if(!quote) dquote = !dquote;
      else buffer[k++] = '\"';
    }else if(command[i] == '\'') {
      if(dquote) buffer[k++] = '\'';
      else quote = !quote;
    }
    else if(command[i] == '\\' && dquote) {
      char next = command[i+1];
      if (next == '\"' || next == '\\' || next == '$' || next == '`' || next == '\n') {
        i++;
        buffer[k++] = command[i];
      } else {
        buffer[k++] = command[i];
      }
    }else if(command[i] == '\\' && quote) {
      buffer[k++] = command[i];
    }else if(command[i] == '\\') {
      i++;
      buffer[k++] = command[i];
    }else {
      buffer[k++] = command[i];
    }
    i++;
  }
  if (k > 0) {
    buffer[k] = '\0';
    args[counter++] = strdup(buffer);
  }else if((quote || dquote) && k == 0) {
    args[counter++] = strdup("");
  }
  args[counter] = NULL;
  return counter;
}

int commands(char command[]) {
  char *args[1024];
  int ind = parseCommand(command, args);
  if(ind == 0) return 0;
  char path[BUFFER_SIZE];
  getPwd(path);

  int redirect_fd = -1;
  int saved_stdout = dup(STDOUT_FILENO);

  for (int i = 0; i < ind; i++) {
    if (strncmp(args[i], ">", 1) == 0 || strncmp(args[i], "1>", 2) == 0 || strncmp(args[i], "2>", 2) == 0) {
      bool write = true;
      
      if(strncmp(args[i], "1>>",3) == 0 || strncmp(args[i], "2>>",3) == 0 || strncmp(args[i], ">>",2) == 0) write = false;
      int targetFd = -1;
      if(strncmp(args[i], "1>", 2) == 0 || strncmp(args[i], ">", 1) == 0) targetFd = STDOUT_FILENO;
      else if(strcmp(args[i], "2>") == 0) targetFd = STDERR_FILENO;
      char *filename = args[i + 1];
      if(write) redirect_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      else redirect_fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
            
      dup2(redirect_fd, targetFd);
      close(redirect_fd);

      args[i] = NULL; 
      ind = i; 
      break;
    }
  }

  if(strcmp(command, "exit") == 0) {
    return -1;
  }else if(strncmp(command, "echo ", 5) == 0) {
    for(int i = 1;i<ind;i++) {
      printf("%s", args[i]);
      if(i != ind-1) printf(" ");
      else printf("\n");
    }
  }else if(strncmp(command, "type ", 5) == 0) {
    int val = isBuiltinCommand(args[1]);
    if(val == 0) printf("%s: not found\n", args[1]);
    return 0;
  }else if(strcmp(command, "pwd") == 0) {
    printf("%s\n", path);
  }else if(strcmp(args[0], "cd") == 0) {
    if(args[1][0] == '~') {
      char completePath[1024];
      strcpy(completePath, homePath);
      char *token = strstr(args[1], "~");
      if(strlen(token) >= 1) token += 1;
      strcat(completePath, token);
      if(chdir(completePath) == 0) {
        getPwd(path);
        currentDirectory = path;
      }else {
        printf("No such file or directory\n");
      }
    }else {
      if(chdir(args[1]) == 0) {
        getPwd(path);
        currentDirectory = path;
      }else {
        printf("No such file or directory\n");
      }
    }    
  }else {
    pid_t pid = fork();
    if(pid < 0) {
      printf("Fork failed");
      return 0;
    }else if(pid == 0) {
      if(execvp(args[0], args) == -1) {
        printf("%s: command not found\n", command);
      }
      exit(EXIT_FAILURE);
    }else {
      int status;
      waitpid(pid, &status, 0);
    }
  }
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);
  return 0;
}

char* find_lcp(char **matches, int count) {
  if (count == 0) return NULL;
  if (count == 1) return matches[0];

  static char lcp[1024];
  strcpy(lcp, matches[0]);

  for (int i = 1; i < count; i++) {
    int j = 0;
    while (lcp[j] != '\0' && matches[i][j] != '\0' && lcp[j] == matches[i][j]) {
      j++;
    }
    lcp[j] = '\0';
    
    if (lcp[0] == '\0') break;
  }
  return lcp;
}

int handle_tab(int count, int key) {
  char command[BUFFER_SIZE];
  strncpy(command, rl_line_buffer, rl_point);
  command[rl_point] = '\0';
  char *commandsFound[BUFFER_SIZE];
  int commandCounter = 0;
  for(int i = 0;i<pathCount;i++) {
    DIR *d = opendir(pathValues[i]);
    if(d == NULL) continue;
    struct dirent *val;
    while((val = readdir(d)) != NULL) {
      char fullPath[1024];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", pathValues[i], val->d_name);
      if(strncmp(val->d_name, command, rl_point)) continue;

      struct stat sb;
      if(stat(fullPath, &sb) == 0) {
        if(S_ISREG(sb.st_mode) && access(fullPath, X_OK) == 0) {
          if(commandCounter < BUFFER_SIZE) {
            if(strcmp(command, val->d_name) == 0) {
              rl_insert_text(" ");
              return 0;
            }
            char *temp = malloc(strlen(val->d_name) + 1);
            if (temp) {
                strcpy(temp, val->d_name);
                commandsFound[commandCounter++] = temp;
            }
          }else break;
          
        }
      }
    }
    closedir(d);
  }

  char *lcp = find_lcp(commandsFound, commandCounter);
  if (lcp != NULL) {
    int prefix_len = strlen(command);
    int lcp_len = strlen(lcp);
    if (lcp_len > prefix_len) {
      rl_insert_text(lcp + prefix_len);
    }
    putchar('\a');
    fflush(stdout);
  }
  if(commandCounter == 1 && strcmp(lcp, commandsFound[0])) {
    rl_insert_text(commandsFound[0] + strlen(command));
    rl_insert_text(" ");
  }
  else if(commandCounter > 1) {
    int proceed = 1;
    if(commandCounter >= 100) {
      printf("\nDisplay all %i possibilities? (y or n)", commandCounter);
      fflush(stdout);

      int c = rl_read_key(); 
      
      if(c != 'y' && c != 'Y' && c != ' ') {
          proceed = 0;
          printf("\n");
      }
    }
    if(proceed) {
      int terminal_width = 80;
      int max_len = 0;
      for(int i = 0; i < commandCounter; i++) {
        int len = strlen(commandsFound[i]);
        if(len > max_len) max_len = len;
      }

      int col_width = max_len + 2;
      int num_cols = terminal_width / col_width;
      if (num_cols == 0) num_cols = 1;

      int num_rows = (commandCounter + num_cols - 1) / num_cols;

      printf("\n");
      for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
          int index = row + (col * num_rows);
            if (index < commandCounter) {
              printf("%-*s", col_width, commandsFound[index]);
            }
        }
        printf("\n");
      }
    }
    rl_on_new_line();

    rl_redisplay();
  }


  if(strcmp(command, "exi") == 0) rl_insert_text("t");
  else if(strcmp(command, "ech") == 0) rl_insert_text("o ");
  else {
    printf("\a");
    fflush(stdout);
  }
  rl_redisplay();
  return 0;
}

int main()
{
  rl_bind_key('\t', handle_tab);
  homePath = getenv("HOME");
  if(homePath == NULL) homePath = "/";
  currentDirectory = malloc(BUFFER_SIZE);
  getPwd(currentDirectory);
  findPathValues(pathValues, &pathCount);
  while(1) {
    char *formattedDirectory;
    formattedDirectory = malloc(1024);
    if(!formattedDirectory) return 0;
    formattedDirectory[0] = '\0';
    char *token = strstr(currentDirectory, homePath);
    if(token != NULL) {
      formattedDirectory[0] = '~';
      formattedDirectory[1] = '\0';
      if(strlen(token) >= strlen(homePath)) token += strlen(homePath);
      strcat(formattedDirectory, token);
    }else {
      strcpy(formattedDirectory, currentDirectory);
    }
    sprintf(formattedDirectory, "%s $ ", formattedDirectory);
    fflush(stdout);
    char* command;
    command = readline(formattedDirectory);
    if (command == NULL) break;
    if (command[0] == '\0') {
      free(command);
      continue;
    }
    if(command[0] == '\n') {
      continue;
    }
    int val = commands(command);
    free(command);
    if(val == -1) break;
  }
  return 0;
}