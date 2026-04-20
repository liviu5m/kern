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
#include <signal.h>
#include "utils.h"
#include "completion.h"

#define BUFFER_SIZE 4096

typedef struct {
    int id;  
    pid_t pid;       
    char command[1024]; 
    bool isActive;      
} Job;

char *builtinCmds[] = {
  "exit",
  "echo",
  "type",
  "jobs",
  "history"
};
char *pathValues[BUFFER_SIZE];
int pathCount = 0;
char *currentDirectory;
char *homePath;
int autoCompletionType = 0;
int jobsCount = 0;
Job jobs[256];

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

int execute(char *args[],int ind, char * command) {
  int isJob = -1;
  if(strcmp(args[ind-1], "&") == 0) {
    isJob = jobsCount;
    args[ind-1] = '\0';
    ind--;
  }
  if(isJob != -1) {
    pid_t jobId = fork();
    if(jobId > 0) {
      char cmd[1024] = "";
      for(int i = 0;i<ind;i++) {
        strcat(cmd, args[i]);
        if(i != ind-1) strcat(cmd, " ");
      }
      jobs[jobsCount].id = jobsCount;
      jobs[jobsCount].pid = jobId;
      jobs[jobsCount].isActive = true;
      strncpy(jobs[jobsCount].command, cmd, sizeof(jobs[jobsCount].command) - 1);
      jobs[jobsCount].command[sizeof(jobs[jobsCount].command) - 1] = '\0';
      jobsCount++;
      
      printf("[%d] %d \n", jobsCount, jobId);

      return 0;
    }
  }

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

  if(strcmp(args[0], "exit") == 0) {
    return -1;
  }else if(strcmp(args[0], "echo") == 0) {
    for(int i = 1;i<ind;i++) {
      printf("%s", args[i]);
      if(i != ind-1) printf(" ");
      else printf("\n");
    }
  }else if(strcmp(args[0], "type") == 0) {
    int val = isBuiltinCommand(args[1]);
    if(val == 0) printf("%s: not found\n", args[1]);
    return 0;
  }else if(strcmp(args[0], "pwd") == 0) {
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
  }else if(strcmp(args[0], "jobs")==0) {
    for(int i = 0;i<jobsCount;i++) {
      printf("[%d]  + Running       %s\n", jobs[i].id + 1, jobs[i].command);
    }
  }else if(strcmp(args[0], "history")==0) {
    HIST_ENTRY **the_list = history_list();

    if (the_list) {
      if(ind > 1) {
        int totalItems = 0;
        while (the_list[totalItems]) {
          totalItems++;
        }
        for (int i = totalItems-1; i>=totalItems-atoi(args[1]); i--) {
          printf("%d  %s\n", i + history_base, the_list[i]->line);
        }
      }else {
        for (int i = 0; the_list[i]; i++) {
          printf("%d  %s\n", i + history_base, the_list[i]->line);
        }
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
  if(isJob != -1) {
    exit(EXIT_SUCCESS);
  }
}

int pipelineCheck(char *args[], int argsCount, char command[]) {
  int pipefds[2], input_fd = 0;
  int num_pipes = 0;
  int status = 0;
  for(int i = 0; i < argsCount; i++) if(strcmp(args[i], "|") == 0) num_pipes++;
  if (num_pipes == 0) {
    status = execute(args, argsCount, command);
  } else {
    int start = 0;
    for (int i = 0; i <= num_pipes; i++) {
      int end = start;
      while (end < argsCount && strcmp(args[end], "|") != 0) end++;
      char *current_cmd_args[64];
      int curr_count = 0;
      for (int j = start; j < end; j++) current_cmd_args[curr_count++] = args[j];
      current_cmd_args[curr_count] = NULL;

      if (i < num_pipes) pipe(pipefds);

      pid_t pid = fork();
      if (pid == 0) {
        if (i > 0) { dup2(input_fd, 0); close(input_fd); }
        if (i < num_pipes) { close(pipefds[0]); dup2(pipefds[1], 1); close(pipefds[1]); }
        status = execute(current_cmd_args, curr_count, current_cmd_args[0]); 
        exit(0);
      } else {
        if (i > 0) close(input_fd);
        if (i < num_pipes) { close(pipefds[1]); input_fd = pipefds[0]; }
        start = end + 1;
      }
    }
    for (int i = 0; i <= num_pipes; i++) wait(NULL);
  }
  return status;
}


int commands(char command[]) {
  char *args[1024];
  int ind = parseCommand(command, args);
  if(ind == 0) return 0;
  
  int startI = 0;
  char *perArgs[1024];
  int argsCount = 0;
  int status = 0;
  while(startI<ind) {
    for(int i = startI;i<ind;i++) {
      if(strcmp(args[i], "&&") == 0) break;
      else {
        perArgs[argsCount++] = args[i];
      }
    }

    status = pipelineCheck(perArgs, argsCount, command);

    if(status == -1) return 0;
    startI += argsCount + 1;
    argsCount = 0;
    perArgs[0] = '\0';
  }
  return 0;
}


void removeIndexFromArray(int ind) {
  for (int i = ind; i < jobsCount - 1; i++) {
      jobs[i] = jobs[i + 1];
  }
  
  jobsCount--;

  memset(&jobs[jobsCount], 0, sizeof(Job));
  jobs[jobsCount].isActive = false;
}

void handle_sigchld(int sig) {
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    for (int i = 0; i < jobsCount; i++) {
      if (jobs[i].pid == pid && jobs[i].isActive) {
        printf("\r\033[K");
        fflush(stdout);
        printf("[%d]  %c %d done       %s\n", jobs[i].id + 1, i == 0 ? '-' : '+' ,jobs[i].pid, jobs[i].command);
        rl_on_new_line();
        rl_redisplay();
        removeIndexFromArray(i);
        i--;
      }
    }
  }
}


int detectSpace() {
  int c = 0;
  for(int i = 0;rl_line_buffer[i] != '\0';i++) {
    if(rl_line_buffer[i] == ' ') c++;
  }
  if(c == 0) autoCompletionType = 0;
  else autoCompletionType = 1;
  return 0;
}

int handleTabCompletion() {
  if(!autoCompletionType) {
    if(handleTabCommandCompletion(pathValues, pathCount) == 1) {
      handleTabFileCompletion();
    }
  }
  else handleTabFileCompletion();
  return 0;
}

void initialize_history(const char *path) {
  if (access(path, F_OK) == -1) {
    int fd = open(path, O_CREAT | O_WRONLY, 0600);
    if (fd != -1) {
      close(fd);
    }
  }

  read_history(path);
}

int main()
{
  homePath = getenv("HOME");
  char historyPath[1024];
  snprintf(historyPath, sizeof(historyPath), "%s/.kern_shell_history", homePath);
  initialize_history(historyPath);
  stifle_history(500);
  signal(SIGCHLD, handle_sigchld);
  rl_bind_key('\t', handleTabCompletion);
  rl_event_hook = detectSpace;
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
    add_history(command);
    append_history(1, historyPath);
    int val = commands(command);
    free(command);
    if(val == -1) break;
  }
  return 0;
}