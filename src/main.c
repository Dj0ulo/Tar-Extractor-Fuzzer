#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "tar.h"
#include "fuzz.h"

int main(int argc, char *argv[])
{
  if(argc <= 1){
    printf("Please give the filename of an extractor in argument.\n");
    return -1;
  }
  printf("\n--- FUZZER ---\n");
  printf("%s\n",argv[1]);

  srand(time(NULL));

  fuzz(argv[1]);

  char name[10];
  memset(name, 0, 10);
  name[0] = -1;
  remove(name);
  name[0] = -10;
  remove(name);
  name[0] = -100;
  remove(name);

  return 0;
}