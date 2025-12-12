struct ffblk { 
    int ff_reserved; 
    int ff_fsize; /* ファイル サイズ */ 
    int ff_attrib; /* 見つかった属性 */ 
    short ff_ftime; /* ファイルの時刻 */ 
    short ff_fdate; /* ファイルの日付 */ 
    char ff_name[256]; /* 見つかったファイル名 */ 
}; 
