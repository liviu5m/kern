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


char* findPathValues(char *pathValues[], int* pathCount) {
  char *pathEnv = getenv("PATH");
  if(pathEnv == NULL) return NULL;
  char *token = strstr(pathEnv, ":");
  char *value = malloc(256); 
  while(token != NULL) {
    sscanf(token, "%255[^:]", value);
    pathValues[*pathCount] = malloc(strlen(value) + 1);
    strcpy(pathValues[*pathCount], value);
    token = strstr(token + 1, ":");
    (*pathCount)++;
    if(token != NULL) {
      token++;
    }
  }
  return pathEnv;
}


int parseCommand(char command[], char *args[]) {
  int i = 0,k = 0, counter = 0;
  bool quote = false, dquote = false;
  char *buffer = malloc(strlen(command) + 1);
  while(command[i] != '\0') {
    if(command[i] == '&') {
      if(k != 0) {
        buffer[k] = '\0';
        args[counter] = strdup(buffer);
        k = 0;
        counter++;
      }
      buffer[k++] = '&';
      if(command[i+1] == '&') {
        buffer[k++] = '&';
        i++;
      }
      buffer[k] = '\0';
      args[counter] = strdup(buffer);
      k = 0;
      counter++;
    }
    else if(command[i] == '>') {
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

void getPwd(char *buffer) {
  if(getcwd(buffer, BUFFER_SIZE) == NULL)  {
    printf("Error on pwd");
  }
}

