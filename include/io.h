#define long int

typedef struct __finddata_t {
    int  attrib;        /* ファイル属性 */
    int  time_create;   /* -1 for FAT file systems */
    int  time_access;   /* -1 for FAT file systems */
    int  time_write;    /* 最終書き込み時刻 */
    int  size;          /* ファイルサイズ   */
    char name[260];     /* ファイル名       */
} _finddata_t;

#define _O_RDONLY  0
#define _O_WRONLY  1
#define _O_RDWR    2
#define _O_BINARY  0x8000

int open(char *filename, int amode, ...);
/* NOTE: Text is the default even if the given _O_TEXT bit is not on. */
#define	_O_TEXT		0x4000	/* CR-LF in file becomes LF in memory. */
#define	_O_BINARY	0x8000	/* Input and output is not translated. */


// attrib は、次の値の組合せ
long _findfirst(char *ptn, _finddata_t *tbl);	// ファイルの検索
long _findnext(long fh, _finddata_t *tbl);	// ファイル検索の継続
int  _findclose(long fh);			// ファイル検索終了
#define    _A_NORMAL   0x00        // Normal file, no attributes 
#define    _A_RDONLY   0x01        // Read only attribute 
#define    _A_HIDDEN   0x02        // Hidden file 
#define    _A_SYSTEM   0x04        // System file 
#define    _A_VOLID    0x08        // Volume label 
#define    _A_SUBDIR   0x10        // Directory 
#define    _A_ARCH     0x20        // Archive 

