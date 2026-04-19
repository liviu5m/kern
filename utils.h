#ifndef UTILS_H
#define UTILS_H

char* findPathValues(char *pathValues[], int* pathCount);
int parseCommand(char command[], char *args[]);
char* find_lcp(char **matches, int count);
void getPwd(char *buffer);

#endif