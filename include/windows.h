#define WM_USER 	 1024
#define va_list   char*

#define ATOM  	  int
#define HANDLE	  void*
//typedef struct HANDLE__    { int unused; } *HANDLE;
typedef struct HBITMAP__   { int unused; } *HBITMAP;
typedef struct HBRUSH__    { int unused; } *HBRUSH;
typedef struct HCURSOR__   { int unused; } *HCURSOR;
typedef struct HDC__       { int unused; } *HDC;
typedef struct HFONT__     { int unused; } *HFONT;
typedef struct HGDIOBJ__   { int unused; } *HGDIOBJ;
typedef struct HGLOBAL__   { int unused; } *HGLOBAL;
typedef struct HHOOK__     { int unused; } *HHOOK;
typedef struct HICON__     { int unused; } *HICON;
typedef struct HINSTANCE__ { int unused; } *HINSTANCE;
typedef struct HKEY__      { int unused; } *HKEY;
typedef struct HMENU__     { int unused; } *HMENU;
typedef struct HMODULE__   { int unused; } *HMODULE;
typedef struct HPEN__      { int unused; } *HPEN;
typedef struct HRESULT__   { int unused; } *HRESULT;
typedef struct HRGN__      { int unused; } *HRGN;
typedef struct HWND__      { int unused; } *HWND;
typedef struct HWAVEOUT__  { int unused; } *HWAVEOUT;

#define CONST	  const
#define wchar_t   short
#define COLORREF  int
#define BYTE	  char
#define CHAR	  char
#define SHORT	  short
#define LPSTR	  char*
#define WPARAM    int
#define LPARAM    int
#define WORD	  short
#define DWORD	  int
#define LONG	  int
#define INT       int
#define INT_PTR   int
#define DWORD_PTR int
#define LONG_PTR  int
#define UINT      int
#define UINT_PTR  int
#define ULONG     int
#define SIZE_T    int
#define BOOL      int
#define LRESULT   int
#define PCVOID	  const void*
#define PCTSTR	  const char*
#define LPCSTR	  const char*
#define LPCTSTR	  const char*
#define LPCWSTR	  const short*
#define LPTSTR	  char*
#define LPWSTR	  short*
#define LPDWORD   int*
#define LPBOOL	  int*
#define CWSTR	  const short*
#define PTSTR	  char*
#define LPVOID    void*
#define PVOID     void*
#define REGSAM	  int
#define CALLBACK  WINAPI
#define APIENTRY  WINAPI

#define PROC      void*
#define DLGPROC   void*
#define WNDPROC	  void*
#define HOOKPROC  void*

#define NULL  	  ((void*)0)
#define MAX_PATH  260    // stdlib.h
#define TRUE	  1
#define FALSE	  0

#define AppendMenu 	AppendMenuA
#define CallWindowProc	CallWindowProcA
#define CreateFile 	CreateFileA
#define CreateFont 	CreateFontA
#define CreateFontIndirect	CreateFontIndirectA
#define CreateProcess	CreateProcessA
#define CreateWindowEx 	CreateWindowExA
#define DefWindowProc	DefWindowProcA
#define DispatchMessage DispatchMessageA
#define DrawText	DrawTextA
#define DialogBoxParam	DialogBoxParamA
#define ExtTextOut	ExtTextOutA
#define FindWindow	FindWindowA
#define GetCommandLine	GetCommandLineA
#define GetMenuItemInfo	GetMenuItemInfoA
#define GetMessage 	GetMessageA
#define GetModuleFileName GetModuleFileNameA
#define GetModuleHandle GetModuleHandleA
#define GetObject	GetObjectA
#define GetStartupInfo	GetStartupInfoA
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetWindowLong	GetWindowLongA
#define GetWindowText	GetWindowTextA
#define InsertMenu      InsertMenuA
#define InsertMenuItem  InsertMenuItemA
#define LoadCursor	LoadCursorA
#define LoadIcon	LoadIconA
#define LoadImage	LoadImageA
#define LoadLibrary	LoadLibraryA
#define MessageBox	MessageBoxA
#define PostMessage	PostMessageA
#define RegOpenKeyEx    RegOpenKeyExA
#define RegQueryValueEx RegQueryValueExA
#define SetMenuItemInfo SetMenuItemInfoA
#define SendMessage	SendMessageA

#define	SetClassLong	SetClassLongA
#define SetWindowLong	SetWindowLongA
#define SetWindowLongPtr SetWindowLongA
#define SetWindowText	SetWindowTextA
#define	RegisterClass	RegisterClassA
#define	RegisterClassEx	RegisterClassExA
#define TextOut		TextOutA
#define WindowText 	WindowTextA
#define WIN32_FIND_DATA WIN32_FIND_DATAA
#define wsprintf	wsprintfA
#define lstrcat		lstrcatA
#define lstrcpy		lstrcpyA
#define lstrlen		lstrlenA

#define CREATESTRUCT	CREATESTRUCTA
#define LOGFONT		LOGFONTA
#define MENUITEMINFO	MENUITEMINFOA
#define STARTUPINFO	STARTUPINFOA
#define ZeroMemory 	RtlZeroMemory

/**
int HIWORD(int dw) { return dw >> 16; }
int LOWORD(int dw) { return dw & 0xFFFF; }
int RGB(int r, int g, int b) { return r + (g<<8) + (b<<16); }
**/

typedef struct _FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME;

typedef struct _WIN32_FIND_DATAA {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	CHAR cFileName[MAX_PATH];
	CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA;

typedef struct tagSIZE {
  LONG cx;
  LONG cy;
} SIZE;

typedef struct tagMEASUREITEMSTRUCT {
   UINT CtlType;
   UINT CtlID;
   UINT itemID;
   UINT itemWidth;
   UINT itemHeight;
   DWORD itemData;
} MEASUREITEMSTRUCT;


#define LF_FACESIZE	32

typedef struct _LOGFONTA {		// wingdi.h
	LONG	lfHeight;
	LONG	lfWidth;
	LONG	lfEscapement;
	LONG	lfOrientation;
	LONG	lfWeight;
	BYTE	lfItalic;
	BYTE	lfUnderline;
	BYTE	lfStrikeOut;
	BYTE	lfCharSet;
	BYTE	lfOutPrecision;
	BYTE	lfClipPrecision;
	BYTE	lfQuality;
	BYTE	lfPitchAndFamily;
	CHAR	lfFaceName[LF_FACESIZE];
} LOGFONTA;

typedef struct _SECURITY_ATTRIBUTES {	// winnt.h
	DWORD nLength;
	void *lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES;

typedef struct _IMAGE_DOS_HEADER {
	WORD e_magic;
	WORD e_cblp;
	WORD e_cp;
	WORD e_crlc;
	WORD e_cparhdr;
	WORD e_minalloc;
	WORD e_maxalloc;
	WORD e_ss;
	WORD e_sp;
	WORD e_csum;
	WORD e_ip;
	WORD e_cs;
	WORD e_lfarlc;
	WORD e_ovno;
	WORD e_res[4];
	WORD e_oemid;
	WORD e_oeminfo;
	WORD e_res2[10];
	LONG e_lfanew;
} IMAGE_DOS_HEADER;
typedef struct _IMAGE_FILE_HEADER {
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} IMAGE_FILE_HEADER;
typedef struct _IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_OPTIONAL_HEADER {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Reserved1;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	DWORD SizeOfStackReserve;
	DWORD SizeOfStackCommit;
	DWORD SizeOfHeapReserve;
	DWORD SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER;
typedef struct _IMAGE_NT_HEADERS {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS;

typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	DWORD Name;
	DWORD Base;
	DWORD NumberOfFunctions;
	DWORD NumberOfNames;
	DWORD AddressOfFunctions;
	DWORD AddressOfNames;
	DWORD AddressOfNameOrdinals;
/*********************************************
	PDWORD *AddressOfFunctions;
	PDWORD *AddressOfNames;
	PWORD *AddressOfNameOrdinals;
******************************************************/
} IMAGE_EXPORT_DIRECTORY;

typedef struct _Misc { DWORD VirtualSize; } Misc;
#define IMAGE_SIZEOF_SHORT_NAME 8
typedef struct _IMAGE_SECTION_HEADER {
	BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
//	union {
//		DWORD PhysicalAddress;
//		DWORD VirtualSize;
//	} Misc;
        Misc  misc;
	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD PointerToRawData;
	DWORD PointerToRelocations;
	DWORD PointerToLinenumbers;
	WORD NumberOfRelocations;
	WORD NumberOfLinenumbers;
	DWORD Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct _PROCESS_INFORMATION {	// winbase.h
	HANDLE hProcess;
	HANDLE hThread;
	DWORD dwProcessId;
	DWORD dwThreadId;
} PROCESS_INFORMATION;

typedef struct _WNDCLASS { 
    UINT    style; 
    PROC    lpfnWndProc; 
    int     cbClsExtra; 
    int     cbWndExtra; 
    HANDLE  hInstance; 
    HICON   hIcon; 
    HCURSOR hCursor; 
    HBRUSH  hbrBackground; 
    LPCTSTR lpszMenuName; 
    LPCTSTR lpszClassName; 
} WNDCLASS; 

typedef struct tagWNDCLASSEX {
  UINT      cbSize;
  UINT      style;
  WNDPROC   lpfnWndProc;
  int       cbClsExtra;
  int       cbWndExtra;
  HINSTANCE hInstance;
  HICON     hIcon;
  HCURSOR   hCursor;
  HBRUSH    hbrBackground;
  LPCTSTR   lpszMenuName;
  LPCTSTR   lpszClassName;
  HICON     hIconSm;
} WNDCLASSEX;

typedef struct _POINT {
    int  x;      // x座標
    int  y;      // y座標
} POINT;

typedef struct _MSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG;

typedef struct _RECT { 
    int left, top, right, bottom; 
} RECT;

typedef struct _PAINTSTRUCT {    // 64 byte
   int hdc;
   int fErase;
   int left, right, top, bottom;
   int fRestore;
   int fIncUpdate;
   char rgbReserved[32];
} PAINTSTRUCT;

typedef struct _WNDCLASSA {
    int style;
    void *lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    int hInstance;
    int hIcon;
    int hCursor;
    int hbrBackground;
    char* lpszMenuName;
    char* lpszClassName;
} WNDCLASSA;

//	WNDCLASSEX 

typedef struct tagCOPYDATASTRUCT {	// winuser.h
	DWORD dwData;
	DWORD cbData;
	PVOID lpData;
} COPYDATASTRUCT;

typedef struct _CREATESTRUCTA {
    void*  	lpCreateParams;
    HINSTANCE	hInstance;
    HMENU    	hMenu;
    HWND    	hwndParent;
    int    	cy;
    int    	cx;
    int    	y;
    int    	x;
    int    	style;
    char*  	lpszName;
    char*  	lpszClass;
    int    	dwExStyle;
} CREATESTRUCTA;

typedef struct _MEMORYSTATUS {    // winbase.h
	DWORD dwLength;
	DWORD dwMemoryLoad;
	DWORD dwTotalPhys;
	DWORD dwAvailPhys;
	DWORD dwTotalPageFile;
	DWORD dwAvailPageFile;
	DWORD dwTotalVirtual;
	DWORD dwAvailVirtual;
} MEMORYSTATUS;

typedef struct _SYSTEMTIME {    // winbase.h
    short wYear, wMonth, wDayOfWeek, wDay;
    short wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME;

typedef struct _STARTUPINFOA {    // winbase.h
    int    cb;
    char*  lpReserved;
    char*  lpDesktop;
    char*  lpTitle;
    int    dwX, dwY;
    int    dwXSize, dwYSize;
    int    dwXCountChars, dwYCountChars;
    int    dwFillAttribute;
    int    dwFlags;
    short  wShowWindow;
    short  cbReserved2;
    char*  lpReserved2;
    int    hStdInput;
    int    hStdOutput;
    int    hStdError;
} STARTUPINFOA;

typedef struct _OPENFILENAME { // ofn 
    int	    lStructSize; 
    int     hwndOwner; 
    int     hInstance; 
    char*   lpstrFilter; 
    char*   lpstrCustomFilter; 
    int     nMaxCustFilter; 
    int     nFilterIndex; 
    char*   lpstrFile; 
    int     nMaxFile; 
    char*   lpstrFileTitle; 
    int     nMaxFileTitle; 
    char*   lpstrInitialDir; 
    char*   lpstrTitle; 
    int     Flags; 
    short   nFileOffset; 
    short   nFileExtension; 
    char*   lpstrDefExt; 
    int     lCustData; 
    void*   lpfnHook; 
    char*   lpTemplateName; 
} OPENFILENAME;

typedef struct _CHOOSECOLORA {	// commdlg.h
   int   lStructSize;
   int   hwndOwner;
   int   hInstance;
   int   rgbResult;
   int*  lpCustColors;
   int   Flags;
   int   lCustData;
   void* lpfnHook;
   char* lpTemplateName;
} CHOOSECOLORA;

typedef struct _MENUITEMINFOA {	// winuser.h
    int cbSize;
    int fMask;
    int fType;
    int fState;
    int wID;
    int hSubMenu;
    int hbmpChecked;
    int hbmpUnchecked;
    int dwItemData;
    char* dwTypeData;
    int cch;
    int hbmpItem;
} MENUITEMINFOA;

typedef struct _NMHDR {	// winuser.h
    HWND     hwndFrom;
    UINT_PTR idFrom;
    UINT     code;
} NMHDR;

typedef struct tagTRACKMOUSEEVENT {
	DWORD cbSize;
	DWORD dwFlags;
	HWND  hwndTrack;
	DWORD dwHoverTime;
} TRACKMOUSEEVENT;

typedef struct _SCROLLINFO {
	UINT cbSize;
	UINT fMask;
	int nMin;
	int nMax;
	UINT nPage;
	int nPos;
	int nTrackPos;
} SCROLLINFO;

typedef struct _CREATESTRUCTA {
	LPVOID	lpCreateParams;
	HINSTANCE	hInstance;
	HMENU	hMenu;
	HWND	hwndParent;
	int	cy;
	int	cx;
	int	y;
	int	x;
	LONG	style;
	LPCSTR	lpszName;
	LPCSTR	lpszClass;
	DWORD	dwExStyle;
} CREATESTRUCTA;

typedef struct tagTRACKMOUSEEVENT {
	DWORD cbSize;
	DWORD dwFlags;
	HWND  hwndTrack;
	DWORD dwHoverTime;
} TRACKMOUSEEVENT;


typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME;

typedef struct tagBITMAPINFOHEADER{	// wingdi.h
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {		// wingdi.h
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {		// wingdi.h
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO;

typedef struct tagBITMAP {
	LONG	bmType;
	LONG	bmWidth;
	LONG	bmHeight;
	LONG	bmWidthBytes;
	WORD	bmPlanes;
	WORD	bmBitsPixel;
	LPVOID	bmBits;
} BITMAP;

typedef struct tagDRAWITEMSTRUCT {	// winuser.h
	UINT CtlType;
	UINT CtlID;
	UINT itemID;
	UINT itemAction;
	UINT itemState;
	HWND hwndItem;
	HDC  hDC;
	RECT rcItem;
	DWORD itemData;
} DRAWITEMSTRUCT;


BOOL 	WINAPI	AppendMenuA(HMENU hMenu, UINT uFlags, UINT uIDItem, PCTSTR pItem);
BOOL 	WINAPI	Beep(DWORD dwFreq, DWORD dwDuration);
HDC  	WINAPI 	BeginPaint(HWND  hwnd, PAINTSTRUCT *lpPaint);
BOOL 	WINAPI	BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
			HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
LRESULT WINAPI	CallWindowProcA(WNDPROC lpWndFunc, HWND hWnd, UINT Msg, 
			WPARAM wParam, LPARAM lParam);
BOOL 	WINAPI	ClientToScreen(HWND hWnd, POINT *lpPoint);
BOOL	WINAPI 	CloseClipboard();
BOOL 	WINAPI  CloseHandle(HANDLE h);
HBITMAP WINAPI	CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight);
HDC 	WINAPI	CreateCompatibleDC(HDC hdc);
HBITMAP WINAPI  CreateDIBSection(HDC hdc, BITMAPINFO *pbmi, UINT iUsage, void **ppvBits, 
			HANDLE hSection, DWORD dwOffset);
HANDLE 	WINAPI	CreateFileA(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,                          // 共有モード
  			SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwCreationDisposition,                // 作成方法
  			DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HFONT 	WINAPI 	CreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation, 
  			int nWeight, DWORD dwItalic, DWORD dwUnderline, DWORD dwStrikeOut,
  			DWORD dwCharSet, DWORD dwOutputPrecision, DWORD dwClipPrecision, 
 			DWORD dwQuality, DWORD dwPitchAndFamily, LPCTSTR lpszFace);
HFONT 	WINAPI	CreateFontIndirectA(const LOGFONT *lplf);
HMENU 	WINAPI	CreateMenu();
HBRUSH 	WINAPI	CreatePatternBrush(HBITMAP hbmp);
HPEN 	WINAPI	CreatePen(int fnPenStyle, int nWidth, COLORREF crColor);
HMENU 	WINAPI	CreatePopupMenu();
BOOL 	WINAPI  CreateProcessA(LPCSTR a,LPSTR b, SECURITY_ATTRIBUTES *c, 
			SECURITY_ATTRIBUTES *d,	BOOL e,DWORD f, void *g, LPCSTR h, 
			STARTUPINFO *i, PROCESS_INFORMATION *j);
HBRUSH 	WINAPI 	CreateSolidBrush(COLORREF crColor);
HWND 	WINAPI  CreateWindowExA(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
			DWORD dwStyle, int x, int y, int nWidth, int nHeight, 
			HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
LRESULT WINAPI 	DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL 	WINAPI 	DeleteDC(HDC hdc);	// wingdi.h
BOOL 	WINAPI 	DeleteObject(HGDIOBJ hObject);
BOOL 	WINAPI  DestroyIcon(HICON hIcon);
BOOL 	WINAPI	DestroyMenu(HMENU hMenu);
INT_PTR WINAPI	DialogBoxParamA(HINSTANCE hI, LPCTSTR lpTemplate, HWND hWndParent,
				DLGPROC lpDialogFunc LPARAM dwInitParam);
LRESULT WINAPI 	DispatchMessageA(const MSG *lpmsg);
BOOL 	WINAPI	DrawEdge(HDC hdc, RECT *qrc, UINT edge, UINT grfFlags);
BOOL 	WINAPI	DrawFocusRect(HDC hDC, CONST RECT *lprc);
BOOL 	WINAPI 	DrawIconEx(HDC a,int b,int c,HICON d,int e,int f,UINT g,HBRUSH h,UINT i);	// winuser.h
BOOL 	WINAPI	DrawMenuBar(HWND hWnd);
int 	WINAPI	DrawTextA(HDC hDC, LPCTSTR lpString, int nCount, RECT *lpRect, UINT uFormat);
BOOL 	WINAPI	EmptyClipboard();
BOOL 	WINAPI	EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable);
BOOL 	WINAPI	EndDialog(HWND hDlg, INT_PTR nResult);
BOOL 	WINAPI 	EndPaint(HWND hWnd, const PAINTSTRUCT *lpPaint);
BOOL 	WINAPI	ExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, const RECT *lprc, 
			LPCTSTR lpString, UINT cbCount, const int *lpDx);
BOOL 	WINAPI	FileTimeToSystemTime(CONST FILETIME *lpFileTime, SYSTEMTIME *lpSystemTime);
int  	WINAPI 	FillRect(HDC hDC, const RECT *lprc, HBRUSH hbr);
BOOL 	WINAPI 	FindClose(HANDLE hFindFile);
HANDLE 	WINAPI 	FindFirstFile(LPCTSTR lpFileName, WIN32_FIND_DATA *lpFindFileData);
BOOL 	WINAPI	FindNextFile(HANDLE hFindFile, WIN32_FIND_DATA *lpFindFileData);
HWND 	WINAPI	FindWindowA(LPCTSTR lpClassName, LPCTSTR lpWindowName);
DWORD 	WINAPI	FormatMessageA(DWORD dwFlags, PCVOID  pSource, DWORD dwMessageID, 
			DWORD dwLanguageID, PTSTR pszBuffer, DWORD nSize, va_list *Arguments);
BOOL 	WINAPI	FreeLibrary(HMODULE hModule);
BOOL 	WINAPI	GetCaretPos(POINT *lpPoint);
BOOL 	WINAPI	GetClientRect(HWND hWnd, RECT *lpRect);
BOOL 	WINAPI	GetCursorPos(POINT *lpPoint);
DWORD 	WINAPI	GetCurrentProcessId();
HDC 	WINAPI	GetDC(HWND hWnd);
SHORT 	WINAPI	GetKeyState(int nVirtKey);
DWORD 	WINAPI  GetLastError();
void 	WINAPI	GetLocalTime(SYSTEMTIME *lpSystemTime);
BOOL 	WINAPI 	GetMessageA(MSG *lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
DWORD 	WINAPI	GetModuleFileNameA(HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
BOOL 	WINAPI	GetCaretPos(POINT *lpPoint);
HANDLE  WINAPI  GetClipboardData(UINT);
char* 	WINAPI  GetCommandLineA();
BOOL 	WINAPI	GetCursorPos(POINT *lpPoint);
HWND 	WINAPI	GetDesktopWindow();
HWND 	WINAPI	GetForegroundWindow();
BOOL 	WINAPI	GetMenuItemInfoA(HMENU hMenu, UINT uItem, BOOL fByPos, MENUITEMINFO *pmii);
HMODULE	WINAPI  GetModuleHandleA(char *lpModuleName);
int	WINAPI 	GetObjectA(HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject);
HWND 	WINAPI	GetParent(HWND hWnd);
COLORREF WINAPI GetPixel(HDC hdc, int nXPos, int nYPos);
BOOL 	WINAPI	GetScrollInfo(HWND hwnd, int fnBar, SCROLLINFO *lpsi);
int 	WINAPI	GetScrollPos(HWND hWnd, int nBar);
HGDIOBJ WINAPI	GetStockObject(int fnObject);
void 	WINAPI 	GetStartupInfoA(STARTUPINFOA *lpStartupInfo);
HMENU   WINAPI  GetSubMenu(HMENU hMenu, int nPos);
DWORD 	WINAPI 	GetSysColor(int nIndex);
int 	WINAPI	GetSystemMetrics(int nIndex);
BOOL 	WINAPI 	GetSystemTimes(FILETIME *lpIdleTime, FILETIME *lpKernelTime, FILETIME *lpUserTime);
BOOL 	WINAPI	GetTextExtentPoint32(HDC hdc, LPCTSTR lpString, int cbString, SIZE *lpSize);
HDC 	WINAPI	GetWindowDC(HWND hWnd);
LONG 	WINAPI	GetWindowLongA(HWND hWnd, int nIndex);
BOOL 	WINAPI	GetWindowRect(HWND hWnd, RECT *lpRect);
int 	WINAPI	GetWindowTextA(HWND hWnd, LPTSTR lpString, int nMaxCount);
HGLOBAL WINAPI	GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
HGLOBAL WINAPI	GlobalFree(HGLOBAL hMem);
LPVOID 	WINAPI	GlobalLock(HGLOBAL hMem);
BOOL	WINAPI  GlobalMemoryStatus(MEMORYSTATUS *lpBuffer);
//BOOL	WINAPI  GlobalMemoryStatusEx(MEMORYSTATUSEX *lpBuffer);
BOOL 	WINAPI	GlobalUnlock(HGLOBAL hMem);
BOOL 	WINAPI	InflateRect(RECT *lprc, int dx, int dy);
BOOL 	WINAPI  InsertMenuA(HMENU hMenu, UINT uPos, UINT uFlags, UINT uIDNewItem, LPCTSTR lpNewItem);
BOOL 	WINAPI 	InsertMenuItemA(HMENU h, UINT i, BOOL b, MENUITEMINFOA *mi);
BOOL 	WINAPI 	InvalidateRect(HWND hWnd, const RECT *lpRect,BOOL bErase);
BOOL 	WINAPI	IsClipboardFormatAvailable(UINT format);
BOOL 	WINAPI	KillTimer(HWND hWnd, UINT_PTR uIDEvent);
BOOL 	WINAPI	LineTo(HDC hdc, int nXEnd, int nYEnd);
HCURSOR WINAPI	LoadCursorA(HINSTANCE hInstance, LPCTSTR lpCursorName);
HICON 	WINAPI	LoadIconA(HINSTANCE hInstance, LPCTSTR lpIconName);
HANDLE  WINAPI  LoadImage(HINSTANCE hinst, LPCTSTR lpszName, UINT uType, int cx, int cy, UINT fuLoad);
HMODULE WINAPI	LoadLibraryA(LPCTSTR lpFileName);
int  	WINAPI 	MessageBoxA(HWND hWnd, char *lpText, char *lpCaption, int uType);
BOOL 	WINAPI	MoveToEx(HDC hdc, int X, int Y, POINT *lpPoint);
BOOL 	WINAPI	MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL 	WINAPI	OpenClipboard(HWND hWndNewOwner);
HANDLE 	WINAPI	OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
BOOL 	WINAPI	PatBlt(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, DWORD dwRop);
BOOL 	WINAPI	PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void 	WINAPI 	PostQuitMessage(int nExitCode);
BOOL 	WINAPI	Rectangle(HDC hdc, int nLeft, int nTop, int nRight, int nBottom);
ATOM 	WINAPI	RegisterClassA(const WNDCLASS *lpwcx);
ATOM 	WINAPI	RegisterClassExA(CONST WNDCLASSEX *lpwcx);
LONG 	WINAPI	RegCloseKey(HKEY hKey);
LONG 	WINAPI 	RegConnectRegistry(LPCTSTR lpMachineName, HKEY hKey, HKEY *phkResult);
LONG 	WINAPI	RegOpenKeyEx(HKEY hKey, LPCTSTR pSubKey, DWORD ulOpts, REGSAM samDesired, HKEY *phkRes);
LONG 	WINAPI	RegQueryValueEx(HKEY hKey, LPCTSTR lpValueName, DWORD *lpReserved, 
				DWORD *lpType, BYTE *lpData, DWORD *lpcbData);
BOOL 	WINAPI	ReleaseCapture();
int 	WINAPI	ReleaseDC(HWND hWnd, HDC hDC);
BOOL 	WINAPI	RestoreDC(HDC hdc, int nSavedDC);
int 	WINAPI	SaveDC(HDC hdc);
void 	WINAPI  ScrollWindow(HWND hWnd, int xAmount, int yAmount, RECT *pRect, RECT *pClipRect);
int 	WINAPI	ScrollWindowEx(HWND hWnd, int dx, int dy, RECT *prcScroll, RECT *prcClip,
			HRGN hrgnUpdate, RECT *prcUpdate, UINT flags);
HGDIOBJ WINAPI  SelectObject(HDC hdc, HGDIOBJ hgdiobj);
LRESULT WINAPI	SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
COLORREF WINAPI SetBkColor(HDC hdc, COLORREF crColor);
int 	WINAPI	SetBkMode(HDC hdc, int iBkMode);
HWND 	WINAPI	SetCapture(HWND hWnd);
BOOL 	WINAPI	SetCaretPos(int X, int Y);
HANDLE 	WINAPI	SetClipboardData(UINT uFormat, HANDLE hMem);
DWORD 	WINAPI	SetClassLongA(HWND hWnd, int nIndex, LONG dwNewLong);
HCURSOR WINAPI	SetCursor(HCURSOR hCursor);
BOOL 	WINAPI	SetFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, 
			CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime);
HWND 	WINAPI 	SetFocus(HWND hwnd);
BOOL 	WINAPI	SetMenu(HWND hWnd, HMENU hMenu);
BOOL 	WINAPI	SetMenuItemInfoA(HMENU hMenu, UINT uItem, BOOL fByPos, MENUITEMINFO *pmii);
COLORREF WINAPI SetPixel(HDC hdc, int X, int Y, COLORREF crColor);
BOOL 	WINAPI	SetRect(RECT *lprc, int xLeft, int yTop, int xRight, int yBottom);
int 	WINAPI 	SetScrollInfo(HWND hWnd,int i, SCROLLINFO *psi, BOOL b);
int 	WINAPI 	SetScrollPos(HWND hWnd, int nBar, int nPos, BOOL bRedraw);

COLORREF WINAPI SetTextColor(HDC hdc, COLORREF crColor);
UINT_PTR WINAPI SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, PROC lpTimerFunc);
LONG 	WINAPI	SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
LONG_PTR WINAPI	SetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
HHOOK 	WINAPI	SetWindowsHookEx(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
BOOL 	WINAPI 	SetWindowTextA(HWND hWnd, char *lpString);
BOOL 	WINAPI	ShowScrollBar(HWND hWnd, int wBar, BOOL bShow);
BOOL 	WINAPI	ShowWindow(HWND hWnd, int nCmdShow);
void 	WINAPI  Sleep(DWORD dwMilliseconds);
//HRESULT WINAPI	SHGetStockIconInfo(SHSTOCKICONID siid, UINT uFlags, SHSTOCKICONINFO *psii);
BOOL 	WINAPI	SystemTimeToFileTime(CONST SYSTEMTIME *lpSystemTime, FILETIME *lpFileTime);
BOOL 	WINAPI	TextOutA(HDC hdc, int nXStart, int nYStart, LPCTSTR lpString, int cbString);
BOOL 	WINAPI	TrackMouseEvent(TRACKMOUSEEVENT *lpEventTrack);
BOOL 	WINAPI	TrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, 
			HWND hWnd, const RECT *pRect);
BOOL 	WINAPI 	TranslateMessage(const MSG *lpMsg);
BOOL 	WINAPI	UpdateWindow(HWND hWnd);
DWORD 	WINAPI	WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
UINT 	WINAPI  WinExec(LPCSTR lpCmdLine, UINT uCmdShow);

int wsprintfA(char *lpOut, const char *lpFmt, ...);	// __cdecl

LPTSTR 	WINAPI	lstrcatA(LPTSTR lpString1, LPCTSTR lpString2);
LPTSTR 	WINAPI	lstrcpyA(LPTSTR lpString1, LPCTSTR lpString2);
int 	WINAPI	lstrlenA(char *str);		// winbase.h

int 	WINAPI  MultiByteToWideChar(
  UINT CodePage,         // コードページ
  DWORD dwFlags,         // 文字の種類を指定するフラグ
  LPCSTR lpMultiByteStr, // マップ元文字列のアドレス
  int cchMultiByte,      // マップ元文字列のバイト数
  LPWSTR lpWideCharStr,  // マップ先ワイド文字列を入れるバッファのアドレス
  int cchWideChar        // バッファのサイズ
);
int 	WINAPI WideCharToMultiByte(
  UINT CodePage,         // コードページ
  DWORD dwFlags,         // 処理速度とマッピング方法を決定するフラグ
  LPCWSTR lpWideCharStr, // ワイド文字列のアドレス
  int cchWideChar,       // ワイド文字列の文字数
  LPSTR lpMultiByteStr,  // 新しい文字列を受け取るバッファのアドレス
  int cchMultiByte,      // 新しい文字列を受け取るバッファのサイズ
  LPCSTR lpDefaultChar,  // マップできない文字の既定値のアドレス
  LPBOOL lpUsedDefaultChar   // 既定の文字を使ったときにセットするフラグのアドレス
);

#define TPM_LEFTALIGN 	0


#define CF_TEXT			1
#define CF_BITMAP		2

#define COLOR_WINDOW 		5
#define	COLOR_BTNFACE 		15
#define COLOR_BTNHILIGHT 	20

#define CREATE_NEW		1
#define CREATE_ALWAYS		2
#define OPEN_EXISTING		3
#define OPEN_ALWAYS		4
#define TRUNCATE_EXISTING	5

#define GENERIC_READ	0x80000000
#define GENERIC_WRITE	0x40000000
#define GENERIC_EXECUTE	0x20000000
#define GENERIC_ALL	0x10000000

// winbase.h
#define GMEM_FIXED 0
#define GMEM_MOVEABLE 2
#define GMEM_MODIFY 128
#define GPTR 64
#define GHND 66
#define GMEM_DDESHARE 8192
#define GMEM_DISCARDABLE 256
#define GMEM_LOWER 4096
#define GMEM_NOCOMPACT 16
#define GMEM_NODISCARD 32
#define GMEM_NOT_BANKED 4096
#define GMEM_NOTIFY 16384
#define GMEM_SHARE 8192
#define GMEM_ZEROINIT 64
#define GMEM_DISCARDED 16384
#define GMEM_INVALID_HANDLE 32768
#define GMEM_LOCKCOUNT 255


#define FILE_READ_DATA	1
#define FILE_LIST_DIRECTORY	1
#define FILE_WRITE_DATA	2
#define FILE_ADD_FILE	2
#define FILE_APPEND_DATA	4
#define FILE_ADD_SUBDIRECTORY	4
#define FILE_CREATE_PIPE_INSTANCE	4
#define FILE_READ_EA	8
#define FILE_READ_PROPERTIES	8
#define FILE_WRITE_EA	16
#define FILE_WRITE_PROPERTIES	16
#define FILE_EXECUTE	32
#define FILE_TRAVERSE	32
#define FILE_DELETE_CHILD	64
#define FILE_READ_ATTRIBUTES	128
#define FILE_WRITE_ATTRIBUTES	256
#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x1FF)
#define FILE_GENERIC_READ (STANDARD_RIGHTS_READ|FILE_READ_DATA|FILE_READ_ATTRIBUTES|FILE_READ_EA|SYNCHRONIZE)
#define FILE_GENERIC_WRITE (STANDARD_RIGHTS_WRITE|FILE_WRITE_DATA|FILE_WRITE_ATTRIBUTES|FILE_WRITE_EA|FILE_APPEND_DATA|SYNCHRONIZE)
#define FILE_GENERIC_EXECUTE (STANDARD_RIGHTS_EXECUTE|FILE_READ_ATTRIBUTES|FILE_EXECUTE|SYNCHRONIZE)
#define FILE_SHARE_READ	1
#define FILE_SHARE_WRITE 2
#define FILE_SHARE_DELETE 4
#define FILE_ATTRIBUTE_READONLY	1
#define FILE_ATTRIBUTE_HIDDEN	2
#define FILE_ATTRIBUTE_SYSTEM	4
#define FILE_ATTRIBUTE_DIRECTORY	16
#define FILE_ATTRIBUTE_ARCHIVE	32
#define FILE_ATTRIBUTE_DEVICE	64
#define FILE_ATTRIBUTE_NORMAL	128
#define FILE_ATTRIBUTE_TEMPORARY	256
#define FILE_ATTRIBUTE_SPARSE_FILE	512
#define FILE_ATTRIBUTE_REPARSE_POINT	1024
#define FILE_ATTRIBUTE_COMPRESSED	2048
#define FILE_ATTRIBUTE_OFFLINE	0x1000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED	0x2000
#define FILE_ATTRIBUTE_ENCRYPTED	0x4000

#define INVALID_HANDLE_VALUE 	(HANDLE)(-1)	// winbase.h

#define CREATE_NO_WINDOW 	0x8000000	// winbase.h
#define INFINITE		0xFFFFFFFF

#define OPAQUE 			2		// wingdi.h
#define SRCCOPY 		0xCC0020
#define TRANSPARENT 		1
#define DI_NORMAL		3
#define DSTINVERT		0x550009
#define PATINVERT		0x5A0049

#define GCL_HBRBACKGROUND 	(-10)

#define GWL_ID 		 	(-12)
#define GWL_WNDPROC 		(-4)
#define GWL_STYLE 		(-16)
#define GWLP_WNDPROC 		(-4)

#define BM_GETCHECK 	 	240
#define BM_SETCHECK 		241

#define BS_PUSHBUTTON		0
#define BS_AUTOCHECKBOX		3
#define BS_AUTORADIOBUTTON	9
#define BS_PUSHLIKE		4096
#define BS_CHECKBOX		2
#define BS_OWNERDRAW		0xb
#define BS_FLAT			0x8000


#define BST_UNCHECKED		0
#define BST_CHECKED		1

#define BDR_RAISEDOUTER	1
#define BDR_SUNKENOUTER	2
#define BDR_RAISEDINNER	4
#define BDR_SUNKENINNER	8
#define BDR_OUTER	3
#define BDR_INNER	0xc
#define BDR_RAISED	5
#define BDR_SUNKEN	10
#define EDGE_RAISED	(BDR_RAISEDOUTER|BDR_RAISEDINNER)
#define EDGE_SUNKEN	(BDR_SUNKENOUTER|BDR_SUNKENINNER)
#define EDGE_ETCHED	(BDR_SUNKENOUTER|BDR_RAISEDINNER)
#define EDGE_BUMP	(BDR_RAISEDOUTER|BDR_SUNKENINNER)
#define BF_LEFT	1
#define BF_TOP	2
#define BF_RIGHT	4
#define BF_BOTTOM	8
#define BF_TOPLEFT	(BF_TOP|BF_LEFT)
#define BF_TOPRIGHT	(BF_TOP|BF_RIGHT)
#define BF_BOTTOMLEFT	(BF_BOTTOM|BF_LEFT)
#define BF_BOTTOMRIGHT	(BF_BOTTOM|BF_RIGHT)
#define BF_RECT	(BF_LEFT|BF_TOP|BF_RIGHT|BF_BOTTOM)
#define BF_DIAGONAL	16
#define BF_DIAGONAL_ENDTOPRIGHT	(BF_DIAGONAL|BF_TOP|BF_RIGHT)
#define BF_DIAGONAL_ENDTOPLEFT	(BF_DIAGONAL|BF_TOP|BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMLEFT	(BF_DIAGONAL|BF_BOTTOM|BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMRIGHT	(BF_DIAGONAL|BF_BOTTOM|BF_RIGHT)
#define BF_MIDDLE	0x800
#define BF_SOFT	0x1000
#define BF_ADJUST	0x2000
#define BF_FLAT	0x4000
#define BF_MONO	0x8000



#define CB_ADDSTRING 		323
#define CB_DELETESTRING 	324
#define CB_DIR 			325
#define CB_GETCOUNT 		326
#define CB_GETCURSEL 		327

#define CBN_SELCHANGE 		1

#define CBS_DROPDOWN		2
#define CBS_DROPDOWNLIST	3
#define CBS_SIMPLE		1
#define CBS_SORT		256

#define CS_DBLCLKS 		0x0008
#define CS_VREDRAW 		0x0001
#define CS_HREDRAW 		0x0002

#define DDL_READWRITE		0

// Scroll Bar
#define SB_HORZ			0
#define SB_VERT			1
#define SB_CTL			2
#define SB_BOTH			3
#define SBS_HORZ 		0	// winuser.h
#define SBS_VERT 		1
#define SBS_SIZEBOX 		8
#define SIF_PAGE 		2
#define SIF_POS 		4
#define SIF_RANGE 		1
#define SB_LINEUP		0
#define SB_LINEDOWN		1
#define SB_LINELEFT		0
#define SB_LINERIGHT		1
#define SB_PAGEUP		2
#define SB_PAGEDOWN		3
#define SB_PAGELEFT		2
#define SB_PAGERIGHT		3
#define SB_THUMBPOSITION	4
#define SB_THUMBTRACK		5
#define SB_ENDSCROLL		8
#define SB_LEFT			6
#define SB_RIGHT		7
#define SB_BOTTOM		7
#define SB_TOP			6

// DrawTextで使用 winuser.h
#define DT_LEFT	     		0	// 
#define DT_CENTER	     	1	// 水平方向にセンタリングする
#define DT_RIGHT	     	2
#define DT_VCENTER	     	4	// 垂直方向にセンタリング(DT_SINGLELINEと共に指定)
#define DT_BOTTOM	     	8	// 矩形の下辺に揃える(DT_SINGLELINE と共に指定)
#define DT_WORDBREAK   		16	// 単語の切れ目で折り返さない
#define DT_SINGLELINE  		32	// 改行コードを無視して一行で表示

#define EN_CHANGE 		768
#define EN_KILLFOCUS 		512	// winuser.h
#define EN_UPDATE 		1024

#define SM_CXSMICON 		49	// winuser.h
#define SM_CYSMICON 		50

#define ES_AUTOHSCROLL		128
#define ES_AUTOVSCROLL		64
#define ES_CENTER		1
#define ES_LEFT			0
#define ES_LOWERCASE 		16
#define ES_MULTILINE 		4
#define ES_RIGHT 		2

#define EM_SETMARGINS          	211   //左右のマージンの設定
#define EM_GETMARGINS           212   //左右のマージンの取得
#define EM_SETSEL 		177
#define EM_CHARFROMPOS 		215

#define EC_LEFTMARGIN 		1
#define EC_RIGHTMARGIN 		2

#define LBS_NOTIFY 		1
#define LBS_SORT 		2
#define LBS_OWNERDRAWFIXED 	16
#define LBS_STANDARD 		0xA00003

#define LB_ADDSTRING 		384
#define LB_INSERTSTRING 	385
#define LB_RESETCONTENT 	388
#define LB_GETCURSEL 		392
#define LB_GETTEXT 		393

#define LBN_SELCHANGE 		1
#define LBN_DBLCLK 		2

#define FW_DONTCARE		0
#define FW_THIN			100
#define FW_EXTRALIGHT		200
#define FW_ULTRALIGHT		200
#define FW_LIGHT		300
#define FW_NORMAL		400
#define FW_REGULAR		400
#define FW_MEDIUM		500
#define FW_SEMIBOLD		600
#define FW_DEMIBOLD		600
#define FW_BOLD			700
#define FW_EXTRABOLD		800
#define FW_ULTRABOLD		800
#define FW_HEAVY		900
#define FW_BLACK		900


#define PS_SOLID		0
#define PS_DASH			1
#define PS_DOT			2

#define IDC_ARROW	 	(LPTSTR)32512	// 標準矢印カーソル
#define IDC_IBEAM	 	(LPTSTR)32513	// アイビーム (縦線) カーソル
#define IDC_WAIT	 	(LPTSTR)32514	// 砂時計カーソル
#define IDC_CROSS	 	(LPTSTR)32515	// 十字カーソル 
#define IDC_UPARROW 		(LPTSTR)32516 	// 垂直の矢印カーソル 
#define IDC_APPSTARTING		(LPTSTR)32650   // 標準の矢印＋小さな砂時計
#define IDC_HELP		(LPTSTR)32651 	// 矢印＋疑問符

#define IDI_APPLICATION 	(LPTSTR)32512
#define IDI_HAND 		(LPTSTR)32513
#define IDI_QUESTION 		(LPTSTR)32514
#define IDI_EXCLAMATION 	(LPTSTR)32515
#define IDI_ASTERISK 		(LPTSTR)32516
#define IDI_WINLOGO 		(LPTSTR)32517

#define IMAGE_BITMAP		0	// winuser.h
#define IMAGE_ICON		1
#define IMAGE_CURSOR		2

#define LR_DEFAULTSIZE 		64	// winuser.h
#define LR_LOADFROMFILE 	16
#define LR_LOADTRANSPARENT 	32
#define LR_CREATEDIBSECTION 	8192

#define DIB_RGB_COLORS		0

#define MIIM_ID 		2	// winuser.h
#define MIIM_TYPE 		16
#define MIIM_SUBMENU 		4
#define MIIM_STATE 		1
#define MIIM_STRING 		64
#define MIIM_BITMAP 		128
#define MIIM_CHECKMARKS 	8

#define MFS_CHECKED 		8
#define MFS_DEFAULT 		4096
#define MFS_DISABLED 		3
#define MFS_ENABLED 		0
#define MFS_GRAYED 		3
#define MFS_HILITE 		128
#define MFS_UNCHECKED 		0
#define MFS_UNHILITE 		0

#define MFT_STRING 		0
#define MFT_SEPARATOR 		0x800
#define MFT_OWNERDRAW 		256

#define MF_POPUP		16
#define MF_SEPARATOR		0x800
#define MF_STRING		0
#define MF_ENABLED		0
#define MF_GRAYED		1
#define MF_DISABLED		2
#define MF_BITMAP		4
#define MF_CHECKED		8
#define MF_BYCOMMAND		0
#define MF_BYPOSITION		1024


#define WHITE_BRUSH 		0	// wingdi.h
#define BLACK_BRUSH 		4
#define DKGRAY_BRUSH 		3
#define GRAY_BRUSH 		2
#define HOLLOW_BRUSH 		5
#define LTGRAY_BRUSH 		1
#define NULL_BRUSH 		5

// Owner Draw
#define ODS_SELECTED 1
#define ODS_GRAYED 2
#define ODS_DISABLED 4
#define ODS_CHECKED 8
#define ODS_FOCUS 16


#define SM_CXSCREEN 		0
#define SM_CYSCREEN 		1


// winbase.h
#define STARTF_USESHOWWINDOW 	1
#define STARTF_USESIZE 		2
#define STARTF_USEPOSITION 	4
#define STARTF_USECOUNTCHARS 	8
#define STARTF_USEFILLATTRIBUTE 16
#define STARTF_RUNFULLSCREEN 	32
#define STARTF_FORCEONFEEDBACK 	64
#define STARTF_FORCEOFFFEEDBACK 128
#define STARTF_USESTDHANDLES 	256
#define STARTF_USEHOTKEY 	512

// kernel32.h ShowWindow
#define SW_HIDE  		0
#define SW_NORMAL 		1
#define SW_SHOWNORMAL 	 	1
#define SW_SHOWMINIMIZED 	2
#define SW_MAXIMIZE 	 	3
#define SW_SHOWMAXIMIZED 	3
#define SW_SHOWNOACTIVATE 	4
#define SW_SHOW 		5
#define SW_MINIMIZE 	 	6
#define SW_SHOWMINNOACTIVE  	7
#define SW_SHOWNA 		8
#define SW_RESTORE 	 	9
#define SW_SHOWDEFAULT 	       	10
#define SW_FORCEMINIMIZE       	11
#define SW_MAX  	       	11

// winuser.h ScrollWindow
#define SW_ERASE 		4
#define SW_INVALIDATE 		2

// winuser.h
#define CW_USEDEFAULT	 	0x80000000	// winuser.h	
#define MB_ICONINFORMATION     	64
#define MB_OK 	  		0		// winuser.h
#define MB_YESNOCANCEL 		3
#define MB_USERICON 		128
#define MB_ICONASTERISK 	64
#define MB_ICONEXCLAMATION 	0x30
#define MB_ICONWARNING 		0x30
#define MB_ICONERROR 		16
#define MB_ICONHAND 		16
#define MB_ICONQUESTION 	32

#define ICON_SMALL 		0
#define ICON_BIG 		1

#define IDABORT 		3
#define IDCANCEL 		2
#define IDCLOSE 		8
#define IDHELP 			9
#define IDIGNORE 		5
#define IDNO 			7
#define IDOK 			1
#define IDRETRY 		4
#define IDYES 			6

#define VK_LBUTTON		1	// winuser.h
#define VK_RBUTTON		2
#define VK_CANCEL		3
#define VK_MBUTTON		4
#define VK_BACK			8
#define VK_TAB			9
#define VK_CLEAR		12
#define VK_RETURN		13
#define VK_KANA			15
#define VK_SHIFT		16
#define VK_CONTROL		17
#define VK_MENU			18
#define VK_PAUSE		19
#define VK_CAPITAL		20
#define VK_ESCAPE		0x1B
#define VK_SPACE		32
#define VK_PRIOR		33
#define VK_NEXT			34
#define VK_END			35
#define VK_HOME			36
#define VK_LEFT			37
#define VK_UP			38
#define VK_RIGHT		39
#define VK_DOWN			40
#define VK_SELECT		41
#define VK_PRINT		42
#define VK_EXECUTE		43
#define VK_SNAPSHOT		44
#define VK_INSERT		45
#define VK_DELETE		46
#define VK_F1			0x70
#define VK_F2			0x71
#define VK_F3			0x72
#define VK_F4			0x73
#define VK_F5			0x74
#define VK_F6			0x75
#define VK_F7			0x76
#define VK_F8			0x77
#define VK_F9			0x78
#define VK_F10			0x79
#define VK_F11			0x7A
#define VK_F12			0x7B

#define TME_HOVER		1	// Track Mouse Event
#define TME_LEAVE		2

#define WM_ACTIVATE 		6
#define WM_CHAR 		258
#define WM_CLEAR 		771
#define WM_CLOSE 	 	16
#define WM_COMMAND  		273
#define WM_COPY 		769
#define WM_COPYDATA 		74
#define WM_CREATE 	 	1
#define WM_CTLCOLORSTATIC  	312
#define WM_CTLCOLOREDIT 	307
#define WM_CTLCOLORSCROLLBAR 	311
#define WM_DESTROY  		2
#define WM_DRAWITEM 		43
#define WM_GETICON 		127
#define WM_GETTEXT 		13
#define WM_GETTEXTLENGTH 	14
#define WM_HSCROLL 		276
#define WM_INITDIALOG 		272
#define WM_KEYDOWN 		256
#define WM_KEYUP 		257
#define WM_KILLFOCUS 		8
#define WM_LBUTTONDOWN 		513
#define WM_LBUTTONUP 		514
#define WM_LBUTTONDBLCLK 	515
#define WM_MEASUREITEM 		44
#define WM_MOUSEMOVE		512
#define WM_MOUSEHOVER		0x2A1
#define WM_MOUSELEAVE		0x2A3
#define WM_NOTIFY   		78
#define WM_PAINT 	 	15
#define WM_PASTE 		770
#define WM_RBUTTONDOWN  	516
#define WM_RBUTTONUP    	517
#define WM_SETFOCUS 		7
#define WM_SETFONT  		48
#define WM_SETTEXT 		12
#define WM_SIZE 	 	5
#define WM_SYSKEYDOWN 		260
#define WM_SYSKEYUP 		261
#define WM_TIMER 	 	275
#define WM_USER 	 	1024
#define WM_VSCROLL 		277

#define WS_BORDER		0x00800000
#define WS_CAPTION		0x00c00000
#define WS_CHILD		0x40000000	// winuser.h
#define WS_CLIPSIBLINGS 	0x04000000
#define WS_GROUP		0x00020000
#define WS_VISIBLE		0x10000000
#define WS_OVERLAPPED	 	0
#define WS_OVERLAPPEDWINDOW 	0x00cf0000

#define WS_EX_TOOLWINDOW	128
#define WS_EX_ACCEPTFILES 	16
#define WS_EX_APPWINDOW		0x40000
#define WS_EX_CLIENTEDGE 	512
#define WS_EX_COMPOSITED 	0x2000000 /* XP */
#define WS_EX_CONTEXTHELP 	0x400
#define WS_EX_CONTROLPARENT 	0x10000
#define WS_EX_DLGMODALFRAME 	1
#define WS_EX_LAYERED 		0x80000   /* w2k */
#define WS_EX_LAYOUTRTL 	0x400000 /* w98, w2k */
#define WS_EX_LEFT		0
#define WS_EX_LEFTSCROLLBAR	0x4000
#define WS_EX_LTRREADING	0
#define WS_EX_MDICHILD		64
#define WS_EX_NOACTIVATE 	0x8000000 /* w2k */
#define WS_EX_NOINHERITLAYOUT 	0x100000 /* w2k */
#define WS_EX_NOPARENTNOTIFY	4
#define WS_EX_OVERLAPPEDWINDOW	0x300
#define WS_EX_PALETTEWINDOW	0x188
#define WS_EX_RIGHT		0x1000
#define WS_EX_RIGHTSCROLLBAR	0
#define WS_EX_RTLREADING	0x2000
#define WS_EX_STATICEDGE	0x20000
#define WS_EX_TOOLWINDOW	128
#define WS_EX_TOPMOST		8
#define WS_EX_TRANSPARENT	32
#define WS_EX_WINDOWEDGE	256

#define WS_HSCROLL		0x100000
#define WS_VSCROLL		0x200000
#define WS_CHILDWINDOW		0x40000000
#define WS_CLIPCHILDREN 	0x2000000
#define WS_DISABLED		0x8000000
#define WS_DLGFRAME		0x400000
#define WS_ICONIC		0x20000000
#define WS_MAXIMIZE		0x1000000
#define WS_MAXIMIZEBOX		0x10000
#define WS_MINIMIZE		0x20000000
#define WS_MINIMIZEBOX		0x20000
#define WS_POPUP		0x80000000
#define WS_POPUPWINDOW		0x80880000
#define WS_SIZEBOX		0x40000
#define WS_SYSMENU		0x80000
#define WS_TABSTOP		0x10000
#define WS_THICKFRAME		0x40000
#define WS_TILED		0
#define WS_TILEDWINDOW		0xcf0000


#define SS_BLACKFRAME 7
#define SS_BLACKRECT 4
#define SS_CENTER 1
#define SS_RIGHT 2
#define SS_RIGHTJUST 0x400


#define SIF_ALL 		23

// Scroll Bar
#define SB_HORZ			0
#define SB_VERT			1
#define SB_LINEUP		0
#define SB_LINEDOWN		1
#define SB_PAGEUP		2
#define SB_PAGEDOWN		3
#define SB_THUMBTRACK		5

#define ETO_OPAQUE 		2	// wingdi.h
#define DEFAULT_QUALITY		0	// wingdi.h
#define FF_DONTCARE		0
#define DEFAULT_PITCH		0

#define ANSI_FIXED_FONT 	11	// wingdi.h
#define ANSI_VAR_FONT 		12
#define DEVICE_DEFAULT_FONT 	14
#define DEFAULT_GUI_FONT 	17
#define OEM_FIXED_FONT 		10
#define SYSTEM_FONT 		13
#define SYSTEM_FIXED_FONT 	16
#define DEFAULT_PALETTE 	15
#define SYSPAL_NOSTATIC 	2
#define SYSPAL_STATIC 		1
#define SYSPAL_ERROR 		0


#define ANSI_CHARSET		0
#define DEFAULT_CHARSET		1
#define SYMBOL_CHARSET		2
#define SHIFTJIS_CHARSET	128
#define OUT_DEFAULT_PRECIS	0
#define OUT_STRING_PRECIS	1
#define OUT_CHARACTER_PRECIS	2
#define OUT_STROKE_PRECIS	3
#define OUT_TT_PRECIS		4
#define OUT_DEVICE_PRECIS	5
#define OUT_RASTER_PRECIS	6
#define OUT_TT_ONLY_PRECIS	7
#define OUT_OUTLINE_PRECIS	8
#define CLIP_DEFAULT_PRECIS	0
#define PROOF_QUALITY		2
#define FIXED_PITCH		1
#define VARIABLE_PITCH		2
#define FF_MODERN		48

#define PROCESS_QUERY_INFORMATION	1024	// winnt.h
#define KEY_ALL_ACCESS 		0xf003f
#define KEY_QUERY_VALUE 	1

#define REG_DWORD 		4		// winreg.h
#define HKEY_DYN_DATA		((HKEY)0x80000006)	// winreg.h

#define CP_ACP 			0		// winnls.h
#define CP_OEMCP 		1
#define CP_MACCP 		2
#define CP_THREAD_ACP 		3
#define CP_SYMBOL 		42
#define CP_UTF7 		65000
#define CP_UTF8 		65001
