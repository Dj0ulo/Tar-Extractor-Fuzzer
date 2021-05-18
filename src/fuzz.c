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

static unsigned errors_number = 0;
static unsigned no_out_number = 0;
static unsigned crashes_number = 0;

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

/*Tests the extractor with the file TEST_FILE and count make some stats*/
int test_file_extractor()
{
  int rv = 0;
  char cmd[250];
  sprintf(cmd, "%s %s 2>&1", extractor_file, TEST_FILE);
  char buf[LEN_CRASH_MSG];
  FILE *fp;

  if ((fp = popen(cmd, "r")) == NULL)
  {
    puts("Error opening pipe!");
    return -1;
  }

  if (fgets(buf, LEN_CRASH_MSG, fp) == NULL)
    no_out_number++;
  else if (strncmp(buf, CRASH_MSG, LEN_CRASH_MSG) != 0)
    errors_number++;
  else
  {
    rv = 1;
    crashes_number++;

    char new_name[100];
    sprintf(new_name, "success_%03u_%s.tar", crashes_number, current_test);
    printf(KGRN "Crash message n°%u " KNRM "-> %s \n", crashes_number, current_test);
    rename(TEST_FILE, new_name);
  }
  if (pclose(fp) == -1)
  {
    puts("Command not found");
    rv = -1;
  }
  return rv;
}

/*Testing the header with no file content*/
void test_header()
{
  write_empty_tar(TEST_FILE, &header);
  test_file_extractor();
}

/*Sets the test name currently running*/
void test_name(const char *name, const char *field_name)
{
  sprintf(current_test, "%s_%s", field_name, name);
}

/* TESTS */

/*Generic tests to apply to almost every fields*/
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

  test_name("middle_null_termination", field_name);
  memset(field, 0, size);
  memset(field, '2', size / 2);
  test_header();

  test_name("0 and_middle_null_termination", field_name);
  memset(field, 0, size);
  memset(field, '0', size / 2);
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
  set_simple_header(&header);

  char *field = header.linkname;
  char field_name[] = "linkname";
  unsigned size = LINKNAME_LEN;

  if (!linkname)
  {
    field = header.name;
    sprintf(field_name, "name");
    size = NAME_LEN;
  }
  else
  {
    test_name("same_as_name", field_name);
    strncpy(field, header.name, size);
    test_header();
  }

  test_name("empty", field_name);
  strncpy(field, "", size);
  test_header();

  strncpy(field, "0" EXT, size);

  for (unsigned i = 0; i < sizeof(WEIRD_CHARS); i++)
  {
    field[0] = WEIRD_CHARS[i];
    sprintf(current_test, "%s_weird_char='%c'", field_name, field[0]);
    test_header();
  }

  char forbidden_char[] = {'*', '\\', '/', '"', '?', ' '};
  for (unsigned i = 0; i < sizeof(forbidden_char); i++)
  {
    field[0] = forbidden_char[i];
    sprintf(current_test, "%s_weird_char='%c'", field_name, field[0]);
    test_header();
  }

  test_name("not_terminated", field_name);
  memset(field, 'a', size);
  test_header();

  test_name("fill_all", field_name);
  sprintf(field, "%0*d" EXT, (int)(size - strlen(EXT) - 1), 0);
  test_header();

  test_name("non_ascii", field_name);
  strncpy(field, "😂😎" EXT, size);
  test_header();

  test_name("directory", field_name);
  strncpy(field, "tests" EXT "/", size);
  test_header();
}

void mode()
{
  set_simple_header(&header);
  char *field = header.mode;
  generic_field_tests("mode", field, MODE_LEN);

  for (unsigned i = 0; i < sizeof(ALL_MODE) / sizeof(ALL_MODE[0]); i++)
  {
    set_simple_header(&header);
    sprintf(field, "%07o", ALL_MODE[i]);
    sprintf(current_test, "mode='%s'", field);
    test_header();
  }
}

void uid()
{
  set_simple_header(&header);
  char *field = header.uid;
  generic_field_tests("uid", field, UID_LEN);
}

void gid()
{
  set_simple_header(&header);
  char *field = header.gid;
  generic_field_tests("gid", field, GID_LEN);
}

void size()
{
  set_simple_header(&header);
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
  set_simple_header(&header);
  char *field = header.mtime;
  char field_name[] = "mtime";
  generic_field_tests(field_name, field, MTIME_LEN);

  test_name("current", field_name);
  sprintf(field, "%lo", (unsigned long)time(NULL));
  test_header();

  test_name("later", field_name);
  sprintf(field, "%lo", (unsigned long)time(NULL) + 50 * 3600);
  test_header();

  test_name("sooner", field_name);
  sprintf(field, "%lo", (unsigned long)time(NULL) + 50 * 3600);
  test_header();

  test_name("far_future", field_name);
  sprintf(field, "%lo", (unsigned long)time(NULL) * 2);
  test_header();
}

void chksum()
{
  set_simple_header(&header);
  char *field = header.chksum;
  generic_field_tests("chksum", field, CHKSUM_LEN);
}

void typeflag()
{
  set_simple_header(&header);
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
  set_simple_header(&header);
  char *field = header.linkname;
  generic_field_tests("linkname", field, LINKNAME_LEN);

  name(1);
}

void magic()
{
  set_simple_header(&header);
  char *field = header.magic;
  generic_field_tests("magic", field, MAGIC_LEN);
}

void version()
{
  set_simple_header(&header);
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
  set_simple_header(&header);
  generic_field_tests(field_name, field, size);
}

void end_bytes()
{
  char end_bytes[END_LEN * 2];
  memset(end_bytes, 0, END_LEN * 2);

  char buffer[] = "hello";
  size_t len_buffer = strlen(buffer);
  set_simple_header(&header);
  set_size_header(&header, strlen(buffer));

  int lengths[] = {END_LEN * 2, END_LEN, 512, 1, 0};

  for (unsigned i = 0; i < sizeof(lengths) / sizeof(int); i++)
  {
    sprintf(current_test, "end_bytes(%d)_with_file", lengths[i]);
    write_tar_end(TEST_FILE, &header, buffer, len_buffer, end_bytes, lengths[i]);
    test_file_extractor();

    sprintf(current_test, "end_bytes(%d)_w-o_file", lengths[i]);
    write_tar_end(TEST_FILE, &header, "", 0, end_bytes, lengths[i]);
    test_file_extractor();
  }
}

void files()
{
  const size_t N = 50;
  tar_entry files[N];
  tar_entry *ff = &files[0];

  // init
  for (size_t i = 0; i < N; i++)
  {
    set_simple_header(&files[i].header);
    files[i].content = NULL;
    files[i].size = 0;
  }

  sprintf(current_test, "%lu_files", N);
  for (size_t i = 0; i < N; i++)
  {
    sprintf(files[i].header.name, "this_is_the_file_number_%lu" EXT, i);
    files[i].content = malloc(30);
    sprintf(files[i].content, "file number %lu", i);
    files[i].size = strlen(files[i].content);
  }
  write_tar_entries(TEST_FILE, files, N);
  test_file_extractor();

  test_name("same_name", "files");
  for (int i = 0; i < 5; i++)
  {
    strncpy(files[i].header.name, "same_name" EXT, NAME_LEN);
    files[i].content = malloc(50);
    sprintf(files[i].content, "file number %d", i);
    files[i].size = strlen(files[i].content);
  }
  write_tar_entries(TEST_FILE, files, 5);
  test_file_extractor();

  test_name("dir_with_data", "files");
  strncpy(ff->header.name, "test" EXT "/", NAME_LEN);
  ff->content = malloc(50);
  ff->size = sprintf(ff->content, "content of the directory like if it was a file");
  write_tar_entries(TEST_FILE, files, 1);
  test_file_extractor();

  FILE *f = fopen(TEST_FILE, "wb");
  if (f)
  {
    fclose(f);
    test_name("empty_tar", "files");
    test_file_extractor();
  }

  test_name("big_file", "files");
  set_simple_header(&ff->header);
  size_t big = 50 * 1000 * 1000;
  ff->content = malloc(big);
  memset(ff->content, 'A', big);
  ff->size = big;
  write_tar_entries(TEST_FILE, files, 1);
  test_file_extractor();
}

/*Execute all fuzzer tests*/
void fuzz(const char *extractor)
{
  strcpy(extractor_file, extractor);

  puts("Begin fuzzing...");
  clock_t start = clock();

  name(0);
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
  end_bytes();
  files();

  clock_t duration = clock() - start;

  puts("Cleaning extractor results...");
  system("rm -rf *" EXT" "TEST_FILE);

  printf("\n%u tests passed in %.3f s:\n", errors_number + no_out_number + crashes_number, (float)duration / CLOCKS_PER_SEC);
  printf(KYEL "%u without output" KNRM "\n", no_out_number);
  printf(KRED "%u errors" KNRM " catched by the extractor\n", errors_number);
  printf(KGRN "%u crashes" KNRM " detected by the fuzzer\n", crashes_number);
}
