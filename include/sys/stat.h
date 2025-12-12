#define stat	_stat

#define time_t	int
#define _dev_t	int
#define _ino_t  short
#define _mode_t short
#define _off_t  int

typedef struct __stat {
    _dev_t  st_dev;        /* Equivalent to drive number 0=A 1=B ... */
    _ino_t  st_ino;        /* Always zero ? */
    _mode_t st_mode;       /* See above constants */
    short   st_nlink;      /* Number of links. */
    short   st_uid;        /* User: Maybe significant on NT ? */
    short   st_gid;        /* Group: Ditto */
    _dev_t  st_rdev;       /* Seems useless (not even filled in) */
    _off_t  st_size;       /* File size in bytes */
    time_t  st_atime;      /* Accessed date (always 00:00 hrs local on FAT) */
    time_t  st_mtime;      /* Modified time */
    time_t  st_ctime;      /* Creation time */
} _stat;

int	_stat(const char *filename, _stat *stbuf);
