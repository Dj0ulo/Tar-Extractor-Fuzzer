#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "fuzz.h"
#include "tar.h"

#define TEST_FILE "test.tar"
#define CRASH_MSG "*** The program has crashed ***\n"
#define LEN_CRASH_MSG strlen(CRASH_MSG) + 1

static char extractor_file[100];
static char current_test[50];
static tar_header header;
static const char WEIRD_CHARS[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 127, 128, 130, 200, 255};

static const unsigned ALL_MODE[] = {
    TSUID,
    TSGID,
    TSVTX,
    TUREAD,
    TUWRITE,
    TUEXEC,
    TGREAD,
    TGWRITE,
    TGEXEC,
    TOREAD,
    TOWRITE,
    TOEXEC};

int test_file_extractor()
{
  static unsigned test_num = 0;
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

  if (fgets(buf, LEN_CRASH_MSG, fp) != NULL && strncmp(buf, CRASH_MSG, LEN_CRASH_MSG) == 0)
  {
    char new_name[100];
    sprintf(new_name, "success_%03u_%s.tar", test_num, current_test);
    printf(KGRN "Crash message n°%u -> %s" KNRM "\n", test_num, current_test);
    rename(TEST_FILE, new_name);
    test_num++;
    rv = 1;
  }
  if (pclose(fp) == -1)
  {
    puts("Command not found");
    rv = -1;
  }
  return rv;
}

void test_header()
{
  write_empty_tar(TEST_FILE, &header);
  test_file_extractor();
}

void test_name(const char *name, const char *field_name)
{
  sprintf(current_test, "%s_%s", field_name, name);
}

/* TESTS */
void generic_field_tests(const char *field_name, char *field, unsigned size)
{
  test_name("empty", field_name);
  strncpy(field, "", size);
  test_header();

  test_name("not_numeric", field_name);
  strncpy(field, "hello", size);
  test_header();

  test_name("big", field_name);
  memset(field, '7', size - 1);
  field[size - 1] = 0;
  test_header();

  test_name("not_octal", field_name);
  memset(field, '9', size - 1);
  field[size - 1] = 0;
  test_header();

  test_name("not_terminated", field_name);
  memset(field, '4', size);
  test_header();

  test_name("terminated_before", field_name);
  memset(field, 0, size);
  memset(field, '2', size/2);
  test_header();

  test_name("not_ascii", field_name);
  strncpy(field, "😂", size);
  test_header();

  test_name("all_0", field_name);
  memset(field, '0', size - 1);
  field[size - 1] = 0;
  test_header();

  test_name("all_null_but_end_0", field_name);
  memset(field, 0, size - 1);
  field[size - 1] = '0';
  test_header();
}

void name(int linkname)
{
  set_simple_header(&header, 0);

  char *field = header.linkname;
  char field_name[] = "linkname";
  unsigned size = LINKNAME_LEN;

  if (!linkname)
  {
    field = header.name;
    sprintf(field_name, "name");
    size = NAME_LEN;
  }else{
    test_name("same_as_name", field_name);
    strncpy(field, header.name, size);
    test_header();
  }


  test_name("empty", field_name);
  strncpy(field, "", size);
  test_header();

  strncpy(field, "0.dat", size);

  for (unsigned i = 0; i < sizeof(WEIRD_CHARS); i++)
  {
    field[0] = WEIRD_CHARS[i];
    sprintf(current_test, "%s_weird_char='%c'", field_name, field[0]);
    test_header();
  }

  char forbidden_char[] = {'*', '\\', '/', '"', '?', '.', ' '};
  for (unsigned i = 0; i < sizeof(forbidden_char); i++)
  {
    char s[2];
    sprintf(s, "%c", forbidden_char[i]);
    strncpy(field, s, size);
    sprintf(current_test, "%s_weird_char='%c'", field_name, field[0]);
    test_header();
  }

  test_name("fill_all", field_name);
  memset(field, 'a', size);
  test_header();

  test_name("non_ascii", field_name);
  strncpy(field, "😂😎.dat", size);
  test_header();
}

void mode()
{
  set_simple_header(&header, 0);
  char *field = header.mode;
  generic_field_tests("mode", field, MODE_LEN);

  for (unsigned i = 0; i < sizeof(ALL_MODE)/sizeof(ALL_MODE[0]); i++)
  {
    set_simple_header(&header, 0);
    sprintf(field, "%07o", ALL_MODE[i]);
    sprintf(current_test, "mode='%s'", field);
    test_header();
  }
}
void uid()
{
  set_simple_header(&header, 0);
  char *field = header.uid;
  generic_field_tests("uid", field, UID_LEN);
}
void gid()
{
  set_simple_header(&header, 0);
  char *field = header.gid;
  generic_field_tests("gid", field, GID_LEN);
}
void size()
{
  set_simple_header(&header, 0);
  char *field = header.size;
  generic_field_tests("size", field, SIZE_LEN);

  char buffer[] = "hello";
  unsigned long len_buffer = strlen(buffer);

  test_name("0", "size");
  set_size_header(&header, 0);
  write_tar(TEST_FILE, &header, buffer, len_buffer);
  test_file_extractor();

  test_name("too_small", "size");
  set_size_header(&header, 2);
  write_tar(TEST_FILE, &header, buffer, len_buffer);
  test_file_extractor();

  test_name("too_big", "size");
  set_size_header(&header, 20);
  write_tar(TEST_FILE, &header, buffer, len_buffer);
  test_file_extractor();

  test_name("far_too_big", "size");
  set_size_header(&header, END_LEN * 2);
  write_tar(TEST_FILE, &header, buffer, len_buffer);
  test_file_extractor();

  test_name("far_far_too_big", "size");
  set_size_header(&header, END_LEN * 2);
  write_tar(TEST_FILE, &header, buffer, len_buffer);
  test_file_extractor();

  test_name("negative", "size");
  sprintf(field, "%011o", -2);
  write_tar(TEST_FILE, &header, buffer, len_buffer);
  test_file_extractor();
}

void mtime()
{
  set_simple_header(&header, 0);
  char *field = header.mtime;
  char field_name[] = "mtime";
  generic_field_tests(field_name, field, MTIME_LEN);

  test_name("current", field_name);
  sprintf(field,"%lo",(unsigned long)time(NULL));
  test_header();

  test_name("later", field_name);
  sprintf(field,"%lo",(unsigned long)time(NULL) + 50*3600);
  test_header();

  test_name("sooner", field_name);
  sprintf(field,"%lo",(unsigned long)time(NULL) + 50*3600);
  test_header();

  test_name("far_future", field_name);
  sprintf(field,"%lo",(unsigned long)time(NULL)*2);
  test_header();
}
void chksum()
{
  set_simple_header(&header, 0);
  char *field = header.chksum;
  generic_field_tests("chksum", field, CHKSUM_LEN);
}
void typeflag()
{
  set_simple_header(&header, 0);
  char field_name[] = "typeflag";
  char name_current_test[30];

  for (unsigned i = 0; i < 0x100; i++)
  {
    sprintf(name_current_test, "value=0x%02x", i);
    test_name(name_current_test, field_name);
    header.typeflag = (char)i;
    test_header();
  }
}

void linkname()
{
  set_simple_header(&header, 0);
  char *field = header.linkname;
  generic_field_tests("linkname", field, LINKNAME_LEN);

  name(1);
}

void magic()
{
  set_simple_header(&header, 0);
  char *field = header.magic;
  generic_field_tests("magic", field, MAGIC_LEN);
}
void version()
{
  set_simple_header(&header, 0);
  char *field = header.version;
  generic_field_tests("version", field, VERSION_LEN);

  for (unsigned i = 0; i < 64; i++)
  {
    field[1] = i % 8 + '0';
    field[0] = i / 8 + '0';
    sprintf(current_test, "version=\'%c%c\'", field[0], field[1]);
    test_header();
  }
}

void uname(int gname)
{
  char *field = header.uname;
  char field_name[] = "uname";
  unsigned size = UNAME_LEN;

  if (!gname)
  {
    field = header.gname;
    sprintf(field_name, "gname");
    size = GNAME_LEN;
  }
  set_simple_header(&header, 0);
  generic_field_tests(field_name, field, size);
}

void end_bytes()
{
  char end_bytes[END_LEN * 2];
  memset(end_bytes, 0, END_LEN * 2);

  char buffer[] = "hello";
  unsigned long len_buffer = strlen(buffer);
  set_simple_header(&header, strlen(buffer));

  int lengths[] = {END_LEN * 2, END_LEN, 1023, 512, 511, 256, 10, 1, 0};

  for (unsigned i = 0; i < sizeof(lengths) / sizeof(int); i++)
  {
    sprintf(current_test, "end_bytes(%d)_with_file", lengths[i]);
    write_tar_end(TEST_FILE, &header, buffer, len_buffer, end_bytes, lengths[i]);
    test_file_extractor();

    sprintf(current_test, "end_bytes(%d)_wo_file", lengths[i]);
    write_tar_end(TEST_FILE, &header, "", 0, end_bytes, lengths[i]);
    test_file_extractor();
  }
}

void fuzz(const char *extractor)
{
  strcpy(extractor_file, extractor);
  // name(0);
  mode();
  uid();
  gid();
  size();
  mtime();
  chksum();
  typeflag();
  linkname();
  magic();
  version();
  uname(0);
  uname(1);
  // end_bytes();
}
