#include "stringutils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char* get_second_string(char* string){
  char *temp = (char *) malloc((strlen(string) + 1)*sizeof(char));
  strcpy(temp, string);
  strtok(temp, " ");
  return strtok(NULL, " ");
}
