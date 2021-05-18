#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fuzz.h"
#include "tar.h"

/**
 * Computes the checksum for a tar header and encode it on the header
 * @param header: The tar header
 * @return the value of the checksum
 */
unsigned calculate_checksum(tar_header *header)
{
  // use spaces for the checksum bytes while calculating the checksum
  memset(header->chksum, ' ', 8);

  // sum of entire metadata
  unsigned int check = 0;
  unsigned char *raw = (unsigned char *)header;
  for (int i = 0; i < 512; i++)
  {
    check += raw[i];
  }

  snprintf(header->chksum, sizeof(header->chksum), "%06o0", check);

  header->chksum[6] = '\0';
  header->chksum[7] = ' ';
  return check;
}

/*Write a random name in a tar_header*/
void set_random_name_header(tar_header *header)
{
  sprintf(header->name, "name_%06u" EXT, rand() % 1000000);
}

/*Set the size of the file content in the tar_header*/
void set_size_header(tar_header *header, size_t size)
{
  sprintf(header->size, "%0*lo", SIZE_LEN - 1, size);
}

/*Write arbitrary values in the tar_header that will likely work with a simple extractor*/
void set_simple_header(tar_header *header)
{
  memset(header, 0, sizeof(tar_header));

  set_random_name_header(header);
  sprintf(header->mode, "0007777");
  sprintf(header->uid, "0001750");
  sprintf(header->gid, "0001750");
  set_size_header(header, 0);
  sprintf(header->mtime, "14047760421");

  //chksum is set to DO_CHKSUM so that the write functions below know that they need to compute it
  sprintf(header->chksum, DO_CHKSUM);
  header->typeflag = '0';
  sprintf(header->magic, "ustar");
  memcpy(header->version, "00", VERSION_LEN);
  sprintf(header->uname, "user");
  sprintf(header->gname, "user");
  sprintf(header->devmajor, "0000000");
  sprintf(header->devminor, "0000000");
}

/*Writes a tar_header in a tar archive without content*/
void write_empty_tar(const char *filename, tar_header *header)
{
  write_tar(filename, header, "", 0);
}

/** 
 * Writes a tar_header and the content of a file in an tar archive
 * @param filename: path of the archive
 * @param header: tar_header of the file
 * @param buffer: content of the file
 * @param size: size of the file
 */
void write_tar(const char *filename, tar_header *header, const char *buffer, size_t size)
{
  char end_bytes[END_LEN];
  memset(end_bytes, 0, END_LEN);

  write_tar_end(filename, header, buffer, size, end_bytes, END_LEN);
}

/**
 * Write a header in the file f. 
 * Compute the chcksum in the header if it is equal to DO_CHKSUM and put back its old value after.
 */
void write_header(FILE *f, tar_header *header)
{
  if (strncmp(DO_CHKSUM, header->chksum, CHKSUM_LEN) == 0)
  {
    char old_chcksum[CHKSUM_LEN];
    strncpy(old_chcksum, header->chksum, CHKSUM_LEN);
    calculate_checksum(header);
    fwrite(header, sizeof(tar_header), 1, f);
    strncpy(header->chksum, old_chcksum, CHKSUM_LEN);
    return;
  }
  fwrite(header, sizeof(tar_header), 1, f);
}

/**
 * Write a header in a file (using the write_header function). 
 * And add *end_size* bytes of *end_bytes* to the end.
 */
void write_tar_end(const char *filename,
                   tar_header *header, const char *buffer, size_t size,
                   const char *end_bytes, size_t end_size)
{
  FILE *f = fopen(filename, "wb");
  if (!f)
  {
    puts("Could not write to file");
    return;
  }

  write_header(f, header);
  fwrite(buffer, size, 1, f);
  fwrite(end_bytes, end_size, 1, f);
  fclose(f);
}

/**
 * Write *count* tar entries in a file. It uses entries[].size to set the field size in the header
 * Also adds the appropriate number of null bytes at the end.
 * ❗ This function frees the pointer of each entries[].content
 */
void write_tar_entries(const char *filename, tar_entry entries[], size_t count)
{
  FILE *f = fopen(filename, "wb");
  if (!f)
  {
    puts("Could not write to file");
    return;
  }

  for (size_t i = 0; i < count; i++)
  {
    tar_entry *e = &entries[i];
    set_size_header(&e->header, e->size);
    write_header(f, &e->header);

    fwrite(e->content, e->size, 1, f);
    if (e->content)
      free(e->content);

    unsigned size_padding = 512 - (e->size % 512);
    char padding[size_padding];
    memset(padding, 0, size_padding);
    fwrite(padding, size_padding, 1, f);
  }

  char end_bytes[END_LEN];
  memset(end_bytes, 0, END_LEN);
  fwrite(end_bytes, END_LEN, 1, f);

  fclose(f);
}
