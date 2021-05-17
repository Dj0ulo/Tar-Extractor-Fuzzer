#ifndef TAR_H
#define TAR_H


#define NAME_LEN 100
#define MODE_LEN 8
#define UID_LEN 8
#define GID_LEN 8
#define SIZE_LEN 12
#define MTIME_LEN 12
#define CHKSUM_LEN 8
#define LINKNAME_LEN NAME_LEN
#define MAGIC_LEN 6
#define VERSION_LEN 2
#define UNAME_LEN 32
#define GNAME_LEN UNAME_LEN

#define END_LEN 1024

typedef struct {          /* byte offset */
    char name[NAME_LEN];               /*   0 */
    char mode[MODE_LEN];                 /* 100 */
    char uid[UID_LEN];                  /* 108 */
    char gid[GID_LEN];                  /* 116 */
    char size[SIZE_LEN];                /* 124 */
    char mtime[MTIME_LEN];               /* 136 */
    char chksum[CHKSUM_LEN];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[LINKNAME_LEN];           /* 157 */
    char magic[MAGIC_LEN];                /* 257 */
    char version[VERSION_LEN];              /* 263 */
    char uname[UNAME_LEN];               /* 265 */
    char gname[GNAME_LEN];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    char padding[12];             /* 500 */
} tar_header;

/* Bits used in the mode field, values in octal.  */
#define TSUID    04000          /* set UID on execution */
#define TSGID    02000          /* set GID on execution */
#define TSVTX    01000          /* reserved */
                                /* file permissions */
#define TUREAD   00400          /* read by owner */
#define TUWRITE  00200          /* write by owner */
#define TUEXEC   00100          /* execute/search by owner */
#define TGREAD   00040          /* read by group */
#define TGWRITE  00020          /* write by group */
#define TGEXEC   00010          /* execute/search by group */
#define TOREAD   00004          /* read by other */
#define TOWRITE  00002          /* write by other */
#define TOEXEC   00001          /* execute/search by other */



unsigned calculate_checksum(tar_header* entry);
void header_init(tar_header *entry);
void set_random_name_header(tar_header* header);
void set_size_header(tar_header* header, unsigned long size);
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