#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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