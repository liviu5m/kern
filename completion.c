#define _DEFAULT_SOURCE
#include "completion.h"
#include "utils.h"
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
#define BUFFER_SIZE 4096

void displayFormattedValues(char *matches[], int counter) {
  int terminal_width = 80;
  int max_len = 0;
  for(int i = 0; i < counter; i++) {
    int len = strlen(matches[i]);
    if(len > max_len) max_len = len;
  }

  int col_width = max_len + 2;
  int num_cols = terminal_width / col_width;
  if (num_cols == 0) num_cols = 1;

  int num_rows = (counter + num_cols - 1) / num_cols;

  printf("\n");
  for (int row = 0; row < num_rows; row++) {
    for (int col = 0; col < num_cols; col++) {
      int index = row + (col * num_rows);
        if (index < counter) {
          printf("%-*s", col_width, matches[index]);
        }
    }
    printf("\n");
  }
}

int handleCompletion(char *matches[], int counter,char *command) {
  char *lcp = find_lcp(matches, counter);
  if (lcp != NULL) {
    int prefix_len = strlen(command);
    int lcp_len = strlen(lcp);
    if (lcp_len > prefix_len) {
      rl_insert_text(lcp + prefix_len);
    }
    putchar('\a');
    fflush(stdout);
  }
  if(counter == 1 && strcmp(lcp, matches[0])) {
    rl_insert_text(matches[0] + strlen(command));
    rl_insert_text(" ");
    rl_redisplay();
    return 1;
  }
  else if(counter > 1) {
    int proceed = 1;
    if(counter >= 100) {
      printf("\nDisplay all %i possibilities? (y or n)", counter);
      fflush(stdout);

      int c = rl_read_key(); 
      
      if(c != 'y' && c != 'Y' && c != ' ') {
          proceed = 0;
          printf("\n");
      }
    }
    if(proceed) displayFormattedValues(matches, counter);
    rl_on_new_line();

    rl_redisplay();
  }else {
    printf("\a");
    fflush(stdout);
  }
  rl_redisplay();
  return 0;
}

int handleTabCommandCompletion(char *pathValues[], int pathCount) {
  for (int i = 0; i < rl_point; i++) {
    if (rl_line_buffer[i] == ' ') {
      return 1;
    }
  }
  char command[BUFFER_SIZE];
  strncpy(command, rl_line_buffer, rl_point);
  command[rl_point] = '\0';
  if(strcmp(command, "exi") == 0) rl_insert_text("t");
  else if(strcmp(command, "ech") == 0) rl_insert_text("o ");
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
              return 1;
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

  return handleCompletion(commandsFound, commandCounter, command);
}


int handleTabFileCompletion() {
  char currentPath[BUFFER_SIZE];
  char command[BUFFER_SIZE];
  getPwd(currentPath);
  strncpy(command, rl_line_buffer, rl_point);
  command[rl_point] = '\0';
  char *token = strrchr(command, ' ');
  if(token != NULL) token += 1;
  else token = command;
  char *lastToken = strrchr(token, '/');
  if(lastToken != NULL) {
    char* fileName = strdup(lastToken + 1);
    *lastToken  = '\0';
    if(token[0] == '/') strcpy(currentPath, token);
    else {
      strcat(currentPath, "/");
      strcat(currentPath, token);
    }
    strcpy(token, fileName);
    free(fileName);
  }
  char *entries[BUFFER_SIZE];
  int entryCount = 0;
  DIR *d = opendir(currentPath);
  if(d == NULL) return 0;
  struct dirent *entry;
  while((entry = readdir(d))) {
    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
    if(strncmp(entry->d_name, token, strlen(token)) == 0) {
      char *val = entry->d_name;
      if(entry->d_type == DT_DIR) strcat(val, "/");
      else strcat(val, " ");
      entries[entryCount++] = strdup(val);
    }
  }
  closedir(d);
  return handleCompletion(entries, entryCount, token);
}

