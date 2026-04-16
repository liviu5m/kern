#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

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

char* findPathValues() {
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
    if(command[i] == ' ') {
      if(!quote && !dquote) {
        if(k == 0) {
          i++;
          continue;
        }
        buffer[k] = '\0';
        args[counter] = strdup(buffer);
        k = 0;
        counter++;
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
  command[strlen(command)-1] = '\0';
  char *args[1024];
  int ind = parseCommand(command, args);
  char path[BUFFER_SIZE];
  getPwd(path);

  if(strcmp(command, "exit\n") == 0) {
    return -1;
  }else if(strncmp(command, "echo ", 5) == 0) {
    for(int i = 1;i<ind;i++) {
      printf("%s", args[i]);
      if(i != ind-1) printf(" ");
      else printf("\n");
    }
  }else if(strncmp(command, "type ", 5) == 0) {
    char *cmd = command + 5;
    int val = isBuiltinCommand(cmd);
    if(val == 0) printf("%s: not found\n", cmd);
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
        command[strlen(command) - 1] = '\0';
        printf("%s: command not found\n", command);
      }
      exit(EXIT_FAILURE);
    }else {
      int status;
      waitpid(pid, &status, 0);
    }
  }
  return 0;
}

void formatPath(char *path) {

}

int main()
{
  homePath = getenv("HOME");
  if(homePath == NULL) homePath = "/";
  currentDirectory = malloc(BUFFER_SIZE);
  getPwd(currentDirectory);
  findPathValues();
  while(1) {
    char *formattedDirectory;
    formattedDirectory = malloc(1024);
    char *token = strstr(currentDirectory, homePath);
    if(token != NULL) {
      formattedDirectory[0] = '~';
      if(strlen(token) >= strlen(homePath)) token += strlen(homePath);
      strcat(formattedDirectory, token);
    }
    printf("%s $ ", formattedDirectory);
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