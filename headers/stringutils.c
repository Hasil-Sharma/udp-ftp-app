#include "stringutils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

schar* get_second_string(schar* string){
  char *temp = (char *) malloc(sizeof(*string));
  strcpy(temp, string);
  strtok(temp, " ");
  return (schar *)strtok(NULL, " ");
}
