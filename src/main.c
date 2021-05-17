#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "tar.h"
#include "fuzz.h"

int main(int argc, char *argv[])
{
  if (argc <= 1)
  {
    printf("Please give the filename of an extractor in argument.\n");
    return -1;
  }
  printf("\n--- FUZZER ---\n");
  printf("%s\n", argv[1]);

  FILE *test_f = fopen(argv[1], "rb");
  if (!test_f)
  {
    printf("The extractor \"%s\" doesn't exist\n", argv[1]);
    return -1;
  }
  fclose(test_f);

  srand(time(NULL));

  fuzz(argv[1]);

  return 0;
}