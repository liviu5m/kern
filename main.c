#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *builtinCmds[] = {
  "exit",
  "echo",
  "type"
};
char *pathValues[256];
int pathCount = 0;

char* findPathValues(char *command) {
  char *pathEnv = getenv("PATH");
  if(pathEnv == NULL) return NULL;
  char *token = strstr(pathEnv, ":");
  char *value = malloc(256); 
  while(token != NULL) {
    sscanf(token, "%255[^:]", value);
    pathValues[pathCount] = malloc(strlen(value) + 1);
    strcpy(pathValues[pathCount], value);
    token = strstr(token + 1, ":");
    pathCount++;
    if(token != NULL) {
      token++;
    }
  }
  return pathEnv;
}

int isBuiltinCommand(char *command) {

  findPathValues(command);
  
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
          return 1;
        }
      }
    }
    closedir(d);
  }
  return 0;
}

int commands(char command[]) {
  if(strcmp(command, "exit\n") == 0) {
    return -1;
  }else if(strncmp(command, "echo ", 5) == 0) {
    printf("%s", command + 5);
  }else if(strncmp(command, "type ", 5) == 0) {
    char *cmd = command + 5;
    cmd[strlen(cmd) - 1] = '\0';
    int val = isBuiltinCommand(cmd);
    if(val == 0) printf("%s: not found\n", cmd);
    return 0;
  }else {
    command[strlen(command) - 1] = '\0';
    printf("%s: command not found\n", command);
  }
  return 0;
}

int main()
{
  while(1) {
    printf("$ ");
    fflush(stdout);
    char command[1024];
    fgets(command, sizeof(command), stdin);
    if(command[0] == '\n') {
      continue;
    }
    int val = commands(command);
    if(val == -1) break;
  }
  return 0;
}