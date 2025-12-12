#define TOOLTIPS_CLASS          "tooltips_class32"

#define HDS_BUTTONS             0x0002
#define HDS_HOTTRACK            0x0004
#define HDS_HIDDEN              0x0008
#define HDS_DRAGDROP            0x0040
#define HDS_FULLDRAG            0x0080


#define LVCOLUMN	LVCOLUMNA
#define LVITEM		LVITEMA

#define LVM_GETITEM	LVM_GETITEMA
#define LVM_GETITEMTEXT	LVM_GETITEMTEXTA
#define LVM_EDITLABEL   LVM_EDITLABELA

#define LVN_BEGINLABELEDIT LVN_BEGINLABELEDITA
#define LVN_GETDISPINFO LVN_GETDISPINFOA

#define TOOLTIPTEXT 	TOOLTIPTEXTA
#define TOOLTIPTEXTA 	NMTTDISPINFOA

#define TOOLINFO	TOOLINFOA

#define TB_ADDBUTTONS	TB_ADDBUTTONSA
#define TB_BUTTONSTRUCTSIZE     (WM_USER + 30)


#define LV_ITEM		LVITEM
#define NMLVDISPINFO	NMLVDISPINFOA
#define LV_DISPINFO     NMLVDISPINFO

#define BYTE			char
#define HINST_COMMCTRL          ((HINSTANCE)-1)
#define IDB_STD_SMALL_COLOR     0
#define IDB_STD_LARGE_COLOR     1

#define STATUSCLASSNAMEA        "msctls_statusbar32"
#define STATUSCLASSNAME         STATUSCLASSNAMEA
#define LVM_SETITEMTEXT         LVM_SETITEMTEXTA

#define LVN_BEGINLABELEDIT	LVN_BEGINLABELEDITA 
#define LVN_ENDLABELEDIT	LVN_ENDLABELEDITA 

#define TCM_FIRST               0x1300      	// Tab control messages
#define TCN_FIRST               -550       	// tab control
#define TBN_FIRST               -700       	// toolbar
#define NM_FIRST                0       	// generic to all controls
#define LVN_FIRST               -100       	// listview
#define LVM_FIRST               0x1000      	// ListView messages
#define HDM_FIRST               0x1200      	// Header messages

#define LVIF_STATE              0x0008


#define LVN_BEGINLABELEDITA     (LVN_FIRST-5)
#define LVN_ENDLABELEDITA       (LVN_FIRST-6)
#define LVN_GETDISPINFOA        (LVN_FIRST-50)

#define LVM_GETNEXTITEM         (LVM_FIRST + 12)
#define LVM_EDITLABELA          (LVM_FIRST + 23)
#define LVM_GETSUBITEMRECT      (LVM_FIRST + 56)
#define LVM_SUBITEMHITTEST      (LVM_FIRST + 57)

#define LVM_ENSUREVISIBLE       (LVM_FIRST + 19)
#define LVM_SCROLL              (LVM_FIRST + 20)
#define LVM_GETTOPINDEX         (LVM_FIRST + 39)

//#define ListView_Scroll(hwndLV, dx, dy) \
//    (BOOL)SNDMSG((hwndLV), LVM_SCROLL, (WPARAM)(int)(dx), (LPARAM)(int)(dy))


/***************************************************
#define NM_FIRST                (0U-  0U)       // generic to all controls
#define NM_LAST                 (0U- 99U)
#define LVN_LAST                (0U-199U)
#define HDN_FIRST               (0U-300U)       // header
#define HDN_LAST                (0U-399U)
#define TVN_FIRST               (0U-400U)       // treeview
#define TVN_LAST                (0U-499U)
#define TTN_FIRST               (0U-520U)       // tooltips
#define TTN_LAST                (0U-549U)
#define TCN_FIRST               (0U-550U)       // tab control
#define TCN_LAST                (0U-580U)
#define CDN_FIRST               (0U-601U)       // common dialog (new)
#define CDN_LAST                (0U-699U)
#define TBN_FIRST               (0U-700U)       // toolbar
#define TBN_LAST                (0U-720U)
#define SBN_FIRST               (0U-880U)       // status bar
#define SBN_LAST                (0U-899U)
*******************************************************/
#define TTF_SUBCLASS            0x0010

#define TTM_ADDTOOL 		TTM_ADDTOOLA
#define TTM_ADDTOOLA            (WM_USER + 4)

#define TTN_FIRST               (-520)       // tooltips
#define TTN_GETDISPINFO         TTN_GETDISPINFOA
#define TTN_GETDISPINFOA        (TTN_FIRST - 0)

#define TTS_ALWAYSTIP           0x01
#define TTS_NOPREFIX            0x02


#define TC_ITEM		TCITEMA

//====== COMMON CONTROL STYLES ================================================
#define CCS_TOP                 0x00000001
#define CCS_NOMOVEY             0x00000002
#define CCS_BOTTOM              0x00000003

#define CDDS_ITEM               0x00010000
#define CDDS_SUBITEM            0x00020000
#define CDDS_PREPAINT           0x00000001
#define CDDS_ITEMPREPAINT       0x00010001	//(CDDS_ITEM | CDDS_PREPAINT)

#define CDRF_NOTIFYITEMDRAW     0x00000020
#define CDRF_NOTIFYSUBITEMDRAW  0x00000020  // flags are the same, we can distinguish by context
#define CDRF_NEWFONT            0x00000002

#define SBARS_SIZEGRIP          0x0100
#define SB_SETPARTS             (WM_USER+4)
#define SB_GETPARTS             (WM_USER+6)
#define SB_SIMPLE               (WM_USER+9)

#define LVCF_FMT                0x0001
#define LVCF_WIDTH              0x0002
#define LVCF_TEXT               0x0004
#define LVCF_SUBITEM            0x0008

#define LVCFMT_LEFT             0x0000
#define LVCFMT_RIGHT            0x0001
#define LVCFMT_CENTER           0x0002

#define LVIF_TEXT               0x0001

#define LVIR_BOUNDS             0
#define LVIR_ICON               1
#define LVIR_LABEL              2
#define LVIR_SELECTBOUNDS       3

#define LVIS_FOCUSED            0x0001
#define LVIS_SELECTED           0x0002
#define LVIS_CUT                0x0004
#define LVIS_DROPHILITED        0x0008
#define LVIS_ACTIVATING         0x0020
#define LVIS_OVERLAYMASK        0x0F00
#define LVIS_STATEIMAGEMASK     0xF000

#define LVM_SETITEM            		(LVM_FIRST + 6)		// 1030
#define LVM_SETEXTENDEDLISTVIEWSTYLE 	(LVM_FIRST + 54)   	// 1078 optional wParam == mask
#define LVM_GETEXTENDEDLISTVIEWSTYLE 	(LVM_FIRST + 55)	// 1079
#define LVM_INSERTCOLUMN       		(LVM_FIRST + 27)	// 1051
#define LVM_INSERTITEM         		(LVM_FIRST + 7)		// 1031
#define LVM_GETEDITCONTROL      	(LVM_FIRST + 24)
#define LVM_GETITEMA            	(LVM_FIRST + 5)
#define LVM_GETITEMTEXTA        	(LVM_FIRST + 45)
#define LVM_SETITEMTEXTA        	(LVM_FIRST + 46)
#define LVM_DELETEITEM          	(LVM_FIRST + 8)
#define LVM_DELETEALLITEMS      	(LVM_FIRST + 9)
#define LVM_GETHEADER               	(LVM_FIRST + 31)
#define LVM_SETITEMSTATE        	(LVM_FIRST + 43)
/*
#define ListView_SetItemState(hwndLV, i, data, mask) \
{ LV_ITEM _ms_lvi;\
  _ms_lvi.stateMask = mask;\
  _ms_lvi.state = data;\
  SNDMSG((hwndLV), LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&_ms_lvi);\
}
*/
#define LVM_GETNEXTITEM         	(LVM_FIRST + 12)
//#define ListView_GetNextItem(hwnd, i, flags) \
//    (int)SNDMSG((hwnd), LVM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((flags), 0))
//    case LVN_ITEMACTIVATE:
//      row = ListView_GetNextItem(g.hList,-1,LVIS_SELECTED);   // 選択行を求める
//    SenMessage(hList, LVM_GETNEXTITEM, -1, LVIS_SELECTED); 
#define LVM_UPDATE              (LVM_FIRST + 42)
//#define ListView_Update(hwndLV, i) \
//    (BOOL)SNDMSG((hwndLV), LVM_UPDATE, (WPARAM)i, 0L)

#define LVM_GETCOLUMNWIDTH      (LVM_FIRST + 29)
//#define ListView_GetColumnWidth(hwnd, iCol) \
//(int)SNDMSG((hwnd), LVM_GETCOLUMNWIDTH, (WPARAM)(int)(iCol), 0)

#define LVM_SETCOLUMNWIDTH      (LVM_FIRST + 30)
//#define ListView_SetColumnWidth(hwnd, iCol, cx) \
//    (BOOL)SNDMSG((hwnd), LVM_SETCOLUMNWIDTH, (WPARAM)(int)(iCol), MAKELPARAM((cx), 0))

#define LVM_GETTOPINDEX         (LVM_FIRST + 39)
//#define ListView_GetTopIndex(hwndLV) \
//    (int)SNDMSG((hwndLV), LVM_GETTOPINDEX, 0, 0)

#define LVM_SETITEMSTATE        (LVM_FIRST + 43)
#define LVM_GETITEMSTATE        (LVM_FIRST + 44)
/*
#define ListView_SetItemState(hwndLV, i, data, mask) \
{ LV_ITEM _ms_lvi;\
  _ms_lvi.stateMask = mask;\
  _ms_lvi.state = data;\
  SNDMSG((hwndLV), LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&_ms_lvi);\
}
#define ListView_GetItemState(hwndLV, i, mask) \
   (UINT)SNDMSG((hwndLV), LVM_GETITEMSTATE, (WPARAM)i, (LPARAM)mask)
*/

#define LVM_SETITEMCOUNT        (LVM_FIRST + 47)
/*
#define ListView_SetItemCount(hwndLV, cItems) \
  SNDMSG((hwndLV), LVM_SETITEMCOUNT, (WPARAM)cItems, 0)

#if (_WIN32_IE >= 0x0300)
#define ListView_SetItemCountEx(hwndLV, cItems, dwFlags) \
  SNDMSG((hwndLV), LVM_SETITEMCOUNT, (WPARAM)cItems, (LPARAM)dwFlags)
#endif

#define LVM_ENSUREVISIBLE       (LVM_FIRST + 19)
#define ListView_EnsureVisible(hwndLV, i, fPartialOK) \
    (BOOL)SNDMSG((hwndLV), LVM_ENSUREVISIBLE, (WPARAM)(int)(i), MAKELPARAM((fPartialOK), 0))
*/
#define LVM_ENSUREVISIBLE       (LVM_FIRST + 19)



#define HDITEM 	HDITEMA

#define HDM_GETITEM             	HDM_GETITEMA
#define HDM_GETITEMA            	(HDM_FIRST + 3)

#define HDM_GETITEMRECT         	(HDM_FIRST + 7)
//#define Header_GetItemRect(hwnd, iItem, lprc) \
//        (BOOL)SNDMSG((hwnd), HDM_GETITEMRECT, (WPARAM)iItem, (LPARAM)lprc)
//#define Header_GetItem(hwndHD, i, phdi) \
//    (BOOL)SNDMSG((hwndHD), HDM_GETITEM, (WPARAM)(int)(i), (LPARAM)(HD_ITEM FAR*)(phdi))

#define LVNI_FOCUSED            0x0001
#define LVNI_SELECTED           0x0002

#define LVS_REPORT              0x0001
#define LVS_EDITLABELS          0x0200
#define LVS_OWNERDRAWFIXED      0x0400
#define LVS_OWNERDATA           0x1000
#define LVS_NOCOLUMNHEADER      0x4000

#define LVS_EX_GRIDLINES        0x00000001
#define LVS_EX_HEADERDRAGDROP   0x00000010
#define LVS_EX_CHECKBOXES       0x00000004
#define LVS_EX_FULLROWSELECT    0x00000020 // applies to report mode only
#define LVS_EX_ONECLICKACTIVATE 0x00000040
#define LVS_EX_FLATSB           0x00000100
#define LVS_EX_REGIONAL         0x00000200
#define LVS_EX_INFOTIP          0x00000400 // listview does InfoTips for you
#define LVS_EX_UNDERLINEHOT     0x00000800
#define LVS_EX_UNDERLINECOLD    0x00001000
#define LVS_EX_MULTIWORKAREAS   0x00002000

#define LVSICF_NOINVALIDATEALL  0x00000001

/********************************************************************
#define LVS_ICON                0x0000
#define LVS_SMALLICON           0x0002
#define LVS_LIST                0x0003
#define LVS_TYPEMASK            0x0003
#define LVS_SINGLESEL           0x0004
#define LVS_SHOWSELALWAYS       0x0008
#define LVS_SORTASCENDING       0x0010
#define LVS_SORTDESCENDING      0x0020
#define LVS_SHAREIMAGELISTS     0x0040
#define LVS_NOLABELWRAP         0x0080
#define LVS_AUTOARRANGE         0x0100
#if (_WIN32_IE >= 0x0300)
#define LVS_OWNERDATA           0x1000
#endif
#define LVS_NOSCROLL            0x2000

#define LVS_TYPESTYLEMASK       0xfc00

#define LVS_ALIGNTOP            0x0000
#define LVS_ALIGNLEFT           0x0800
#define LVS_ALIGNMASK           0x0c00

#define LVS_NOCOLUMNHEADER      0x4000
#define LVS_NOSORTHEADER        0x8000

// end_r_commctrl
****************************************************/

#define NM_CLICK        	(NM_FIRST-2) //-2 uses NMCLICK struct
#define NM_DBLCLK               (NM_FIRST-3)
#define NM_RETURN               (NM_FIRST-4)
#define NM_RCLICK               (NM_FIRST-5)    // uses NMCLICK struct
#define NM_RDBLCLK              (NM_FIRST-6)
#define NM_SETFOCUS             (NM_FIRST-7)
#define NM_KILLFOCUS            (NM_FIRST-8)
#define NM_CUSTOMDRAW           (NM_FIRST-12)

#define SB_SETTEXT              SB_SETTEXTA
#define SB_SETTEXTA             (WM_USER+1)

#define SBT_OWNERDRAW            0x1000

#define STD_CUT                 0
#define STD_COPY                1
#define STD_PASTE               2
#define STD_UNDO                3
#define STD_REDOW               4
#define STD_DELETE              5
#define STD_FILENEW             6
#define STD_FILEOPEN            7
#define STD_FILESAVE            8
#define STD_PRINTPRE            9
#define STD_PROPERTIES          10
#define STD_HELP                11
#define STD_FIND                12
#define STD_REPLACE             13
#define STD_PRINT               14

#define TBSTATE_CHECKED         0x01
#define TBSTATE_PRESSED         0x02
#define TBSTATE_ENABLED         0x04
#define TBSTATE_HIDDEN          0x08
#define TBSTATE_INDETERMINATE   0x10
#define TBSTATE_WRAP            0x20
#define TBSTATE_ELLIPSES        0x40
#define TBSTATE_MARKED          0x80

#define TBSTYLE_BUTTON          0x0000
#define TBSTYLE_SEP             0x0001
#define TBSTYLE_DROPDOWN        0x0008
#define TBSTYLE_TOOLTIPS        0x0100
#define TBSTYLE_FLAT            0x0800
#define TBSTYLE_EX_DRAWDDARROWS 0x00000001

#define TBN_DROPDOWN            (TBN_FIRST - 10)

#define TCIF_TEXT		0x0001

#define TCM_GETCURSEL		(TCM_FIRST + 11)
#define TCM_INSERTITEM  	(TCM_FIRST + 7)
#define TCM_SETITEMSIZE 	(TCM_FIRST + 41)
#define TCM_HIGHLIGHTITEM       (TCM_FIRST + 51)
#define TCM_SETCURSEL           (TCM_FIRST + 12)

#define TCN_SELCHANGE		(TCN_FIRST - 1)

#define WC_TABCONTROL		WC_TABCONTROLA
#define WC_TABCONTROLA          "SysTabControl32"
#define WC_LISTVIEW            	"SysListView32"
#define TOOLBARCLASSNAME        "ToolbarWindow32"

#define TB_SETSTATE             (WM_USER + 17)
#define TB_GETSTATE             (WM_USER + 18)
#define TB_GETSTYLE 		(WM_USER + 57)
#define TB_SETSTYLE		(WM_USER + 56)
#define TB_SETEXTENDEDSTYLE     (WM_USER + 84)  // For TBSTYLE_EX_*
#define TB_ADDSTRINGA           (WM_USER + 28)
#define TB_ADDBUTTONSA          (WM_USER + 20)
#define TB_ADDBITMAP            (WM_USER + 19)
#define TB_SETTOOLTIPS          (WM_USER + 36)

#define TB_ADDSTRING            TB_ADDSTRINGA

#define TCS_BOTTOM              0x0002
#define TCS_BUTTONS             0x0100
#define TCS_HOTTRACK            0x0040
#define TCS_RIGHTJUSTIFY        0x0000
#define TCS_FIXEDWIDTH          0x0400
#define TCS_OWNERDRAWFIXED      0x2000
#define TCS_TOOLTIPS            0x4000
#define TCS_FLATBUTTONS         0x0008
#define TCS_EX_FLATSEPARATORS   0x00000001

#define TBSTYLE_BUTTON          0x0000
#define TBSTYLE_SEP             0x0001
#define TBSTYLE_CHECK           0x0002
#define TBSTYLE_GROUP           0x0004

#define BTNS_BUTTON          0x0000
#define BTNS_SEP             0x0001
#define BTNS_CHECK           0x0002
#define BTNS_GROUP           0x0004
#define BTNS_CHECKGROUP      (BTNS_GROUP | BTNS_CHECK)
#define BTNS_DROPDOWN        0x0008
#define BTNS_AUTOSIZE        0x0010 // automatically calculate the cx of the button
#define BTNS_NOPREFIX        0x0020 // if this button should not have accel prefix
#define BTNS_SHOWTEXT        0x0040
#define BTNS_TOOLTIPS        0x0100
#define BTNS_WRAPABLE        0x0200
#define BTNS_ALTDRAG         0x0400
#define BTNS_FLAT            0x0800
#define BTNS_LIST            0x1000
#define BTNS_CUSTOMERASE     0x2000
#define BTNS_REGISTERDROP    0x4000
#define BTNS_TRANSPARENT     0x8000
#define BTNS_EX_DRAWDDARROWS 0x00000001

typedef struct tagNMTTDISPIFNOA {
    NMHDR hdr;
    LPSTR lpszText;
    char szText[80];
    HINSTANCE hinst;
    UINT uFlags;
    LPARAM lParam;
} NMTTDISPINFOA;

typedef struct tagTOOLINFOA {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT uId;
    RECT rect;
    HINSTANCE hinst;
    LPSTR lpszText;
    LPARAM lParam;
} TOOLINFOA;

typedef struct _TCITEMA {
    UINT mask;
    DWORD dwState;
    DWORD dwStateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
} TCITEMA;

typedef struct _LVCOLUMNA {	// commctrl.h
    int mask;
    int fmt;
    int cx;
    char* pszText;
    int cchTextMax;
    int iSubItem;
    int iImage;
    int iOrder;
} LVCOLUMNA;


typedef struct _LVITEMA {
    int mask;
    int iItem;
    int iSubItem;
    int state;
    int stateMask;
    char* pszText;
    int cchTextMax;
    int iImage;
    int lParam;
    int iIndent;
} LVITEMA;

typedef struct _HD_ITEMA {
    UINT    mask;
    int     cxy;
    LPSTR   pszText;
    HBITMAP hbm;
    int     cchTextMax;
    int     fmt;
    LPARAM  lParam;
    int     iImage;        // index of bitmap in ImageList
    int     iOrder;        // where to draw this item
    UINT    type;
    LPVOID  pvFilter;
} HDITEMA;


typedef struct _TBBUTTON {
    int iBitmap;
    int idCommand;
    BYTE fsState;
    BYTE fsStyle;
    BYTE bReserved[2];
    DWORD dwData;
    int iString;
} TBBUTTON;

typedef struct tagTBADDBITMAP {
        HINSTANCE       hInst;
        UINT            nID;
} TBADDBITMAP;

/***********************************************
typedef struct _NMHDR {	// winuser.h
    HWND     hwndFrom;
    UINT_PTR idFrom;
    UINT     code;
} NMHDR;
****************************************************/

typedef struct tagNMCUSTOMDRAWINFO {	// 48バイト
    NMHDR hdr;		// 12
    DWORD dwDrawStage;
    HDC hdc;
    RECT rc;		// 16
    DWORD dwItemSpec;  
    UINT  uItemState;
    LPARAM lItemlParam;
} NMCUSTOMDRAW;

typedef struct tagNMLVCUSTOMDRAW {	// 60バイト
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
    COLORREF clrTextBk;
    int iSubItem;
} NMLVCUSTOMDRAW;

typedef struct tagLVDISPINFO {
    NMHDR hdr;
    LVITEMA item;
} NMLVDISPINFOA;

typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;             // size of this structure
    DWORD dwICC;              // flags indicating which classes to be initialized
} INITCOMMONCONTROLSEX;

typedef struct tagNMITEMACTIVATE {
    NMHDR   hdr;
    int     iItem;
    int     iSubItem;
    UINT    uNewState;
    UINT    uOldState;
    UINT    uChanged;
    POINT   ptAction;
    LPARAM  lParam;
    UINT    uKeyFlags;
} NMITEMACTIVATE;

#define ICC_LISTVIEW_CLASSES 0x00000001 // listview, header
#define ICC_TREEVIEW_CLASSES 0x00000002 // treeview, tooltips
#define ICC_BAR_CLASSES      0x00000004 // toolbar, statusbar, trackbar, tooltips
#define ICC_TAB_CLASSES      0x00000008 // tab, tooltips

#define FSB_FLAT_MODE           2
#define FSB_ENCARTA_MODE        1
#define FSB_REGULAR_MODE        0

#define WSB_PROP_CYVSCROLL  0x00000001
#define WSB_PROP_CXHSCROLL  0x00000002
#define WSB_PROP_CYHSCROLL  0x00000004
#define WSB_PROP_CXVSCROLL  0x00000008
#define WSB_PROP_CXHTHUMB   0x00000010
#define WSB_PROP_CYVTHUMB   0x00000020
#define WSB_PROP_VBKGCOLOR  0x00000040
#define WSB_PROP_HBKGCOLOR  0x00000080
#define WSB_PROP_VSTYLE     0x00000100
#define WSB_PROP_HSTYLE     0x00000200
#define WSB_PROP_WINSTYLE   0x00000400
#define WSB_PROP_PALETTE    0x00000800
#define WSB_PROP_MASK       0x00000FFF

void 	WINAPI	InitCommonControls();
BOOL 	WINAPI	InitCommonControlsEx(INITCOMMONCONTROLSEX *picex);
HWND 	WINAPI	CreateStatusWindow(LONG style, LPCTSTR lpszText, HWND hwndParent, UINT wID);
HWND 	WINAPI	CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, int nBitmaps, 
		HINSTANCE hBMInst, UINT wBMID, TBBUTTON *lpButtons, int iNumButtons,
		int dxButton, int dyButton, int dxBitmap, int dyBitmap, UINT uStructSize);
int 	WINAPI	FlatSB_SetScrollInfo(HWND hwnd, int fnBar, SCROLLINFO *lpsi, BOOL fRedraw);
BOOL 	WINAPI 	FlatSB_EnableScrollBar(HWND, int, UINT);
BOOL 	WINAPI 	FlatSB_ShowScrollBar(HWND, int code, BOOL);
BOOL 	WINAPI 	FlatSB_GetScrollRange(HWND, int code, INT *pn, INT *pm);
BOOL 	WINAPI 	FlatSB_GetScrollInfo(HWND, int code, SCROLLINFO *si);
int 	WINAPI 	FlatSB_GetScrollPos(HWND, int code);
BOOL 	WINAPI 	FlatSB_GetScrollProp(HWND, int propIndex, INT *pn);
int 	WINAPI 	FlatSB_SetScrollPos(HWND, int code, int pos, BOOL fRedraw);
int 	WINAPI 	FlatSB_SetScrollInfo(HWND, int code, SCROLLINFO *si, BOOL fRedraw);
int 	WINAPI 	FlatSB_SetScrollRange(HWND, int code, int min, int max, BOOL fRedraw);
BOOL 	WINAPI 	FlatSB_SetScrollProp(HWND, UINT index, int newValue, BOOL);
BOOL 	WINAPI 	InitializeFlatSB(HWND);
HRESULT WINAPI 	UninitializeFlatSB(HWND);
