#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tar.h"

/**
 * Computes the checksum for a tar header and encode it on the header
 * @param entry: The tar header
 * @return the value of the checksum
 */
unsigned calculate_checksum(tar_header *entry)
{
  // use spaces for the checksum bytes while calculating the checksum
  memset(entry->chksum, ' ', 8);

  // sum of entire metadata
  unsigned int check = 0;
  unsigned char *raw = (unsigned char *)entry;
  for (int i = 0; i < 512; i++)
  {
    check += raw[i];
  }

  snprintf(entry->chksum, sizeof(entry->chksum), "%06o0", check);

  entry->chksum[6] = '\0';
  entry->chksum[7] = ' ';
  return check;
}

void header_init(tar_header *entry)
{
  memset(entry, 0, sizeof(tar_header));
}
void set_random_name_header(tar_header *header)
{
  sprintf(header->name, "name_%06u.dat", rand() % 1000000);
}
void set_size_header(tar_header *header, unsigned long size)
{
  sprintf(header->size, "%011lo", size);
}
void set_simple_header(tar_header *entry, unsigned long size_buffer)
{
  header_init(entry);
  set_random_name_header(entry);
  sprintf(entry->mode, "0007777");
  sprintf(entry->uid, "0001750");
  sprintf(entry->gid, "0001750");
  set_size_header(entry, size_buffer);
  sprintf(entry->mtime, "14047760421");
  sprintf(entry->chksum, DO_CHKSUM);
  entry->typeflag = '0';
  sprintf(entry->magic, "ustar");
  memcpy(entry->version, "00", VERSION_LEN);
  sprintf(entry->uname, "user");
  sprintf(entry->gname, "user");
  sprintf(entry->devmajor, "0000000");
  sprintf(entry->devminor, "0000000");
}

void write_empty_tar(const char *filename, tar_header *entry)
{
  write_tar(filename, entry, "", 0);
}
void write_tar(const char *filename, tar_header *entry, const char *buffer, unsigned long size)
{
  char end_bytes[END_LEN];
  memset(end_bytes, 0, END_LEN);

  write_tar_end(filename, entry, buffer, size, end_bytes, END_LEN);
}

void write_header(FILE *f, tar_header *header)
{
  char old_chcksum[CHKSUM_LEN];
  strncpy(old_chcksum, header->chksum, CHKSUM_LEN);
  if (strncmp(DO_CHKSUM, header->chksum, CHKSUM_LEN) == 0)
    calculate_checksum(header);

  fwrite(header, sizeof(tar_header), 1, f);

  strncpy(header->chksum, old_chcksum, CHKSUM_LEN);
}

void write_tar_end(const char *filename,
                   tar_header *header,
                   const char *buffer,
                   unsigned long size,
                   const char *end_bytes,
                   unsigned long end_size)
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
    if(e->content)
      free(e->content);

    unsigned size_padding = 512 - (e->size%512);
    char padding[size_padding];
    memset(padding, 0, size_padding);
    fwrite(padding, size_padding, 1, f);

  }

  char end_bytes[END_LEN];
  memset(end_bytes, 0, END_LEN);
  fwrite(end_bytes, END_LEN, 1, f);

  fclose(f);
}
