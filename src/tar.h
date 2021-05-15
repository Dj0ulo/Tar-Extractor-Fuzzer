#ifndef TAR_H
#define TAR_H

#define END_LEN 1024

typedef struct {          /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    char padding[12];             /* 500 */
} tar_header;


unsigned calculate_checksum(tar_header* entry);
void header_init(tar_header *entry);
void set_simple_header(tar_header *entry, unsigned long size_buffer);
void write_empty_tar(const char *filename, tar_header *entry);
void write_tar_end(const char *filename,
                   tar_header *entry,
                   const char *buffer,
                   unsigned long size,
                   const char *end_bytes,
                   unsigned long end_size);
void write_tar(const char *filename, tar_header* entry, const char *buffer, unsigned long size);
void write_file(const char *filename, const char *buffer, unsigned long size);
#endif