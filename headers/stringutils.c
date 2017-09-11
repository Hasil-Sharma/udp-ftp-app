#include "stringutils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char* get_second_string(char* string){
  char *temp = (char *) malloc(sizeof(string));
  strcpy(temp, string);
  strtok(temp, " ");
  return strtok(NULL, " ");
}
