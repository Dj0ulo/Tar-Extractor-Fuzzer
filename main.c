#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  printf("*** The program has crashed ***\n");
  printf("hello world !\n");
  if(argc >= 2)
    printf("%s\n",argv[1]);

  for (int i = 0; i < 10; i++)
    printf("%d\n", i);


  return 0;
}