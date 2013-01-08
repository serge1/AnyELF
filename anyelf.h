#ifndef _ANYELF__H
#define _ANYELF__H

#define ANYELF_VERSION_HI  1
#define ANYELF_VERSION_LOW 1

#include <string>

#define lc_copy            1
#define lc_newparams       2
#define lc_selectall       3
#define lc_setpercent      4

#define lcp_wraptext       1
#define lcp_fittowindow    2
#define lcp_ansi           4
#define lcp_ascii          8
#define lcp_variable      12
#define lcp_forceshow     16
#define lcp_fitlargeronly 32
#define lcp_center        64

#define lcs_findfirst      1
#define lcs_matchcase      2
#define lcs_wholewords     4
#define lcs_backwards      8

#define itm_percent   0xFFFE
#define itm_fontstyle 0xFFFD
#define itm_wrap      0xFFFC
#define itm_fit       0xFFFB
#define itm_next      0xFFFA
#define itm_center    0xFFF9

#define LISTPLUGIN_OK      0
#define LISTPLUGIN_ERROR   1

typedef struct {
    int size;
    DWORD PluginInterfaceVersionLow;
    DWORD PluginInterfaceVersionHi;
    char DefaultIniName[MAX_PATH];
} ListDefaultParamStruct;

HWND APIENTRY ListLoad( HWND parentWin, char* fileToLoad, int showFlags );
HWND APIENTRY ListLoadW( HWND parentWin, WCHAR* fileToLoad, int showFlags );
int  APIENTRY ListLoadNext( HWND parentWin, HWND pluginWin, char* fileToLoad, int showFlags );
int  APIENTRY ListLoadNextW( HWND parentWin, HWND pluginWin, WCHAR* fileToLoad, int showFlags );
void APIENTRY ListCloseWindow( HWND listWin );
void APIENTRY ListGetDetectString( char* detectString, int maxlen );
int  APIENTRY ListSearchText( HWND listWin, char* searchString, int searchParameter );
int  APIENTRY ListSearchTextW( HWND listWin, WCHAR* searchString, int searchParameter );
int  APIENTRY ListSearchDialog( HWND listWin, int findNext );
int  APIENTRY ListSendCommand( HWND listWin, int command, int parameter );
int  APIENTRY ListPrint( HWND listWin, char* fileToPrint, char* defPrinter,
                         int printFlags, RECT* margins );
int  APIENTRY ListPrintW( HWND listWin, WCHAR* fileToPrint, WCHAR* defPrinter,
                          int printFlags, RECT* margins );
int  APIENTRY ListNotificationReceived( HWND listWin, int message, WPARAM wParam, LPARAM lParam );
void APIENTRY ListSetDefaultParams( ListDefaultParamStruct* dps );

HBITMAP APIENTRY ListGetPreviewBitmap( char* fileToLoad, int width, int height,
                                       char* contentbuf, int contentbuflen );
HBITMAP APIENTRY ListGetPreviewBitmapW( WCHAR* fileToLoad, int width, int height,
                                        char* contentbuf,int contentbuflen);

std::string elfdump( std::string fileToLoad );

#endif // _ANYELF__H
