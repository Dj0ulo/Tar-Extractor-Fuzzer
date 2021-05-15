#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tar.h"

#define TEST_FILE "test.tar"
#define CRASH_MSG "*** The program has crashed ***\n"
#define LEN_CRASH_MSG strlen(CRASH_MSG) + 1

static char extractor_file[100];
static char current_test[20];
int test_file_extractor()
{
  static unsigned i = 0;
  int rv = 0;
  char cmd[250];
  sprintf(cmd, "%s %s", extractor_file, TEST_FILE);
  char buf[LEN_CRASH_MSG];
  FILE *fp;

  if ((fp = popen(cmd, "r")) == NULL)
  {
    puts("Error opening pipe!");
    return -1;
  }

  if(fgets(buf, LEN_CRASH_MSG, fp) != NULL && strncmp(buf, CRASH_MSG, LEN_CRASH_MSG) == 0)
  {
    printf("Crash message n°%u !\n", i);
    char new_name[50];
    sprintf(new_name, "success_%03u_%s.tar", i, current_test);
    rename(TEST_FILE, new_name);
    i++;
    rv = 1;
  }
  if (pclose(fp) == -1)
  {
    puts("Command not found");
    rv = -1;
  }
  return rv;
}

void test_header(tar_header *header)
{
  write_empty_tar(TEST_FILE, header);
  test_file_extractor();
}

/* TESTS */
void name()
{
  sprintf(current_test, "name");
  tar_header header;
  set_simple_header(&header, 0);

  //empty name
  strncpy(header.name, "", 100);
  test_header(&header);

  strncpy(header.name, "0.dat", 100);

  //non-ascii
  header.name[0] = (char)-1;
  test_header(&header);
  header.name[0] = (char)-10;
  test_header(&header);
  header.name[0] = (char)-100;
  test_header(&header);
}

void end_bytes()
{
  sprintf(current_test, "end_bytes");
  tar_header header;

  char end_bytes[END_LEN * 2];
  memset(end_bytes, 0, END_LEN * 2);

  char buffer[] = "hello";
  unsigned long len_buffer = strlen(buffer);
  set_simple_header(&header, strlen(buffer));

  int lengths[] = {END_LEN * 2, END_LEN, 1023, 512, 511, 256, 10, 1, 0};

  for(unsigned i=0;i<sizeof(lengths);i++){
    write_tar_end(TEST_FILE, &header, buffer, len_buffer, end_bytes, lengths[i]);
    test_file_extractor();
    
    write_tar_end(TEST_FILE, &header, "", 0, end_bytes, lengths[i]);
    test_file_extractor();
  }
}

void fuzz(const char *extractor)
{
  strcpy(extractor_file, extractor);
  name();
  end_bytes();
}
