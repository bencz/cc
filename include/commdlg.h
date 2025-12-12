#define CHOOSEFONT		CHOOSEFONTA
#define CHOOSECOLOR		CHOOSECOLORA

#define ChooseColor		ChooseColorA
#define	ChooseFont 		ChooseFontA
#define	GetOpenFileName 	GetOpenFileNameA

#define CF_SCREENFONTS             0x00000001
#define CF_EFFECTS                 0x00000100
#define CF_INITTOLOGFONTSTRUCT     0x00000040

#define SCREEN_FONTTYPE       	   0x2000


#define OFN_READONLY                 0x00000001
#define OFN_OVERWRITEPROMPT          0x00000002
#define OFN_HIDEREADONLY             0x00000004
#define OFN_NOCHANGEDIR              0x00000008
#define OFN_SHOWHELP                 0x00000010
#define OFN_ENABLEHOOK               0x00000020
#define OFN_ENABLETEMPLATE           0x00000040
#define OFN_ENABLETEMPLATEHANDLE     0x00000080
#define OFN_NOVALIDATE               0x00000100
#define OFN_ALLOWMULTISELECT         0x00000200
#define OFN_EXTENSIONDIFFERENT       0x00000400
#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_CREATEPROMPT             0x00002000
#define OFN_SHAREAWARE               0x00004000
#define OFN_NOREADONLYRETURN         0x00008000
#define OFN_NOTESTFILECREATE         0x00010000
#define OFN_NONETWORKBUTTON          0x00020000
#define OFN_NOLONGNAMES              0x00040000     // force no long names for 4.x modules
#define OFN_EXPLORER                 0x00080000     // new look commdlg
#define OFN_NODEREFERENCELINKS       0x00100000
#define OFN_LONGNAMES                0x00200000     // force long names for 3.x modules
#define OFN_ENABLEINCLUDENOTIFY      0x00400000     // send include message to callback
#define OFN_ENABLESIZING             0x00800000

#define CC_RGBINIT               0x00000001
#define CC_FULLOPEN              0x00000002

typedef struct _CHOOSEFONTA {
   DWORD           lStructSize;
   HWND            hwndOwner;          // caller's window handle
   HDC             hDC;                // printer DC/IC or NULL
   LOGFONTA        *lpLogFont;          // ptr. to a LOGFONT struct
   INT             iPointSize;         // 10 * size in points of selected font
   DWORD           Flags;              // enum. type flags
   COLORREF        rgbColors;          // returned text color
   LPARAM          lCustData;          // data passed to hook fn.
   //HOOKPROC        *lpfnHook;          // ptr. to hook function
   LRESULT         *lpfnHook;          // ptr. to hook function
   LPCSTR          lpTemplateName;     // custom template name
   HINSTANCE       hInstance;          // instance handle of.EXE that
                                       //   contains cust. dlg. template
   LPSTR           lpszStyle;          // return the style field here
                                       // must be LF_FACESIZE or bigger
   WORD            nFontType;          // same value reported to the EnumFonts
                                       //   call back with the extra FONTTYPE_
                                       //   bits added
   WORD            ___MISSING_ALIGNMENT__;
   INT             nSizeMin;           // minimum pt size allowed &
   INT             nSizeMax;           // max pt size allowed if
                                       //   CF_LIMITSIZE is used
} CHOOSEFONTA;

typedef struct tagCHOOSECOLORA {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HWND         hInstance;
   COLORREF     rgbResult;
   COLORREF*    lpCustColors;
   DWORD        Flags;
   LPARAM       lCustData;
   //LPCCHOOKPROC lpfnHook;
   LRESULT	*lpfnHook;
   LPCSTR       lpTemplateName;
} CHOOSECOLORA;

BOOL 	WINAPI	GetOpenFileNameA(OPENFILENAME *lpofn);
BOOL 	WINAPI	ChooseFontA(CHOOSEFONT *lpcf);
BOOL  APIENTRY  ChooseColorA(CHOOSECOLOR *lpcc);

