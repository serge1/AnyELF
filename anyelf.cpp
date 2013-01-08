// anyelf.cpp : Defines the entry point for the DLL application.
//

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdlib.h>
#include <shellapi.h>
#include <malloc.h>
#include <richedit.h>
#include <commdlg.h>
#include <math.h> 

#include "anyelf.h"
#include "cunicode.h"

#define parsefunction "[0]=127 & [1]=\"E\" & [2]=\"L\" & [3]=\"F\""

HINSTANCE   hinst;
std::string g_text;
char        inifilename[MAX_PATH]="anyelf.ini";  // Unused in this plugin,
                                                 // may be used to save data

//---------------------------------------------------------------------------
static char*
strlcpy( char* p, char* p2, int maxlen )
{
    strncpy( p, p2, maxlen );
    p[maxlen] = 0;
    return p;
}


//---------------------------------------------------------------------------
static void
searchAndReplace( std::string& value, std::string const& search,
                  std::string const& replace ) 
{ 
    std::string::size_type  next; 
 
    for ( next = value.find( search );        // Try and find the first match 
          next != std::string::npos;          // next is npos if nothing was found 
          next = value.find( search, next )   // search for the next match starting after 
                                              // the last match that was found. 
        ) { 
        // Inside the loop. So we found a match. 
        value.replace( next, search.length(), replace );   // Do the replacement. 
        next += replace.length();                          // Move to just after the replace 
                                                           // This is the point were we start 
                                                           // the next search from.  
    }
}


//---------------------------------------------------------------------------
BOOL APIENTRY
DllMain( HANDLE hModule, 
         DWORD  ul_reason_for_call, 
         LPVOID lpReserved )
{
    switch ( ul_reason_for_call )
    {
        case DLL_PROCESS_ATTACH:
            hinst = (HINSTANCE)hModule;
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}


//---------------------------------------------------------------------------
void APIENTRY
ListGetDetectString( char* detectString, int maxlen )
{
    strlcpy( detectString, parsefunction, maxlen );
}


//---------------------------------------------------------------------------
void APIENTRY
ListSetDefaultParams( ListDefaultParamStruct* dps )
{
    dps->PluginInterfaceVersionHi  = ANYELF_VERSION_HI;
    dps->PluginInterfaceVersionLow = ANYELF_VERSION_LOW;
    strlcpy( inifilename, dps->DefaultIniName, MAX_PATH-1 );
}


//---------------------------------------------------------------------------
HWND APIENTRY
ListLoad( HWND parentWin, char* fileToLoad, int showFlags )
{
    HWND listWin = 0;
    RECT r;
    
    GetClientRect( parentWin, &r );
    // Create window invisbile, only show when data fully loaded!
    listWin = CreateWindow( "EDIT", "", WS_CHILD | ES_MULTILINE | ES_WANTRETURN |
                                        ES_READONLY | WS_HSCROLL | WS_VSCROLL |
                                        ES_AUTOVSCROLL | ES_NOHIDESEL,
                            r.left, r.top, r.right-r.left, r.bottom-r.top,
                            parentWin, NULL, hinst, NULL);

    if ( listWin != 0 ) {
        SendMessage( listWin, EM_SETMARGINS, EC_LEFTMARGIN, 8 );
        SendMessage( listWin, EM_SETEVENTMASK, 0, ENM_UPDATE ); //ENM_SCROLL doesn't work for thumb movements!

        ShowWindow( listWin, SW_SHOW );

        int ret = ListLoadNext( parentWin, listWin, fileToLoad, showFlags );
        if ( ret == LISTPLUGIN_ERROR ) {
            DestroyWindow( listWin );
            listWin = 0;
        }
    }

    return listWin;
}


//---------------------------------------------------------------------------
int APIENTRY
ListLoadNext( HWND parentWin, HWND listWin, char* fileToLoad, int showFlags)
{
    g_text = elfdump( fileToLoad );
    if ( g_text.empty() ) {
        return LISTPLUGIN_ERROR;
    }

    searchAndReplace( g_text, "\n", "\r\n" );

    HFONT font;
    if ( showFlags & lcp_ansi ) {
        font = (HFONT)GetStockObject( ANSI_FIXED_FONT );
    }
    else {
        font = (HFONT)GetStockObject( SYSTEM_FIXED_FONT );
    }
    SendMessage( listWin, WM_SETFONT, (WPARAM)font, MAKELPARAM( true, 0 ) );

    SendMessage( listWin, WM_SETTEXT, 0, (LPARAM)g_text.c_str() ); 

    PostMessage( parentWin, WM_COMMAND, MAKELONG( 0, itm_percent ), (LPARAM)listWin );

    return LISTPLUGIN_OK;
}


//---------------------------------------------------------------------------
void APIENTRY
ListCloseWindow( HWND listWin )
{
    DestroyWindow( listWin );
    return;
}


//---------------------------------------------------------------------------
int APIENTRY
ListNotificationReceived( HWND listWin, int message, WPARAM wParam, LPARAM lParam )
{
    int firstvisible;
    int linecount;

    switch ( message ) {
    case WM_COMMAND:
        switch ( HIWORD( wParam ) ) {
        case EN_UPDATE:
        case EN_VSCROLL:
            firstvisible = SendMessage( listWin, EM_GETFIRSTVISIBLELINE, 0, 0 );
            linecount    = SendMessage( listWin, EM_GETLINECOUNT, 0, 0 );
            if ( linecount > 0 ) {
                int percent = MulDiv( firstvisible, 100, linecount );
                PostMessage( GetParent( listWin ), WM_COMMAND,
                             MAKELONG( percent, itm_percent ), (LPARAM)listWin );
            }
            return 0;
        }
        break;
    case WM_NOTIFY:
        break;
    case WM_MEASUREITEM:
        break;
    case WM_DRAWITEM:
        break;
    }
    return 0;
}


//---------------------------------------------------------------------------
int APIENTRY
ListSendCommand( HWND listWin, int command, int parameter )
{
    switch ( command ) {
    case lc_copy:
        SendMessage( listWin, WM_COPY, 0, 0 );
        return LISTPLUGIN_OK;
    case lc_newparams:
        HFONT font;
        if ( parameter & lcp_ansi ) {
            font = (HFONT)GetStockObject( ANSI_FIXED_FONT );
        }
        else {
            font = (HFONT)GetStockObject( SYSTEM_FIXED_FONT );
        }
        SendMessage( listWin, WM_SETFONT, (WPARAM)font, MAKELPARAM( true, 0 ) );
        PostMessage( GetParent( listWin ), WM_COMMAND, MAKELONG( 0, itm_next ), (LPARAM)listWin );
        return LISTPLUGIN_ERROR;
    case lc_selectall:
        SendMessage( listWin, EM_SETSEL, 0, -1 );
        return LISTPLUGIN_OK;
    case lc_setpercent:
        int firstvisible = SendMessage( listWin, EM_GETFIRSTVISIBLELINE, 0, 0 );
        int linecount    = SendMessage( listWin, EM_GETLINECOUNT, 0, 0 );
        if ( linecount > 0 ) {
            int pos = MulDiv( parameter, linecount, 100 );
            SendMessage( listWin, EM_LINESCROLL, 0, pos - firstvisible );
            firstvisible = SendMessage( listWin, EM_GETFIRSTVISIBLELINE, 0, 0 );
            // Place caret on first visible line!
            int firstchar = SendMessage( listWin, EM_LINEINDEX, firstvisible, 0);
            SendMessage( listWin, EM_SETSEL, firstchar, firstchar );
            pos = MulDiv( firstvisible, 100, linecount );
            // Update percentage display
            PostMessage( GetParent( listWin ), WM_COMMAND, MAKELONG( pos, itm_percent ), (LPARAM)listWin );
            return LISTPLUGIN_OK;
        }
        break;
    }

    return LISTPLUGIN_ERROR;
}


//---------------------------------------------------------------------------
int APIENTRY
ListSearchText( HWND listWin,char* searchString,int searchParameter )
{
    FINDTEXT find;
    int      startPos, flags;

    if ( searchParameter & lcs_findfirst ) {
        //Find first: Start at top visible line
        startPos = SendMessage( listWin, EM_LINEINDEX,
            SendMessage( listWin, EM_GETFIRSTVISIBLELINE, 0, 0 ), 0 );
        SendMessage( listWin, EM_SETSEL, startPos, startPos );
    } else {
        //Find next: Start at current selection+1
        SendMessage( listWin, EM_GETSEL, (WPARAM)&startPos, 0 );
        ++startPos;
    }

    find.chrg.cpMin = startPos;
    find.chrg.cpMax = SendMessage( listWin, WM_GETTEXTLENGTH, 0, 0 );
    flags           = 0;
    if ( searchParameter & lcs_wholewords )
        flags |= FR_WHOLEWORD;
    if ( searchParameter & lcs_matchcase )
        flags |= FR_MATCHCASE;
    if ( !( searchParameter & lcs_backwards ) )
        flags |= FR_DOWN;
    find.lpstrText = searchString;
    int index = SendMessage( listWin, EM_FINDTEXT, flags, (LPARAM)&find );
    if ( index != -1 ) {
        int indexend = index+strlen( searchString );
        SendMessage( listWin, EM_SETSEL, index, indexend );
        int line = SendMessage( listWin, EM_LINEFROMCHAR, index, 0 ) - 3;
        if ( line < 0 )
            line = 0;
        line -= SendMessage( listWin, EM_GETFIRSTVISIBLELINE, 0, 0 );
        SendMessage( listWin, EM_LINESCROLL, 0, line );

        return LISTPLUGIN_OK;
    }

    SendMessage( listWin, EM_SETSEL, -1, -1);  // Restart search at the beginning

    return LISTPLUGIN_ERROR;
}


//---------------------------------------------------------------------------
int APIENTRY
ListPrint(HWND ListWin,char* FileToPrint,char* DefPrinter,int PrintFlags,RECT* Margins)
{
    PRINTDLG PrintDlgRec;
    memset(&PrintDlgRec,0,sizeof(PRINTDLG));
    PrintDlgRec.lStructSize=sizeof(PRINTDLG);

    PrintDlgRec.Flags= PD_ALLPAGES | PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    PrintDlgRec.nFromPage   = 0xFFFF; 
    PrintDlgRec.nToPage     = 0xFFFF; 
    PrintDlgRec.nMinPage	= 1;
    PrintDlgRec.nMaxPage	= 0xFFFF;
    PrintDlgRec.nCopies		= 1;
    PrintDlgRec.hwndOwner	= ListWin;// MUST be Zero, otherwise crash!
    if (PrintDlg(&PrintDlgRec)) 
    {
        HDC hdc=PrintDlgRec.hDC;
        DOCINFO DocInfo;
        POINT offset,physsize,start,avail,printable;
        int LogX,LogY;
        RECT rcsaved;

        // Warning: PD_ALLPAGES is zero!
        BOOL PrintSel=(PrintDlgRec.Flags & PD_SELECTION);
        BOOL PrintPages=(PrintDlgRec.Flags & PD_PAGENUMS);
        int PageFrom=1;
        int PageTo=0x7FFF;
        if (PrintPages) {
            PageFrom=PrintDlgRec.nFromPage;
            PageTo=PrintDlgRec.nToPage;
            if (PageTo<=0) PageTo=0x7FFF;
        }


        memset(&DocInfo,0,sizeof(DOCINFO));
        DocInfo.cbSize=sizeof(DOCINFO);
        DocInfo.lpszDocName=FileToPrint;
        if (StartDoc(hdc,&DocInfo)) {
            SetMapMode(hdc,MM_LOMETRIC);
            offset.x=GetDeviceCaps(hdc,PHYSICALOFFSETX);
            offset.y=GetDeviceCaps(hdc,PHYSICALOFFSETY);
            DPtoLP(hdc,&offset,1);
            physsize.x=GetDeviceCaps(hdc,PHYSICALWIDTH);
            physsize.y=GetDeviceCaps(hdc,PHYSICALHEIGHT);
            DPtoLP(hdc,&physsize,1);

            start.x=Margins->left-offset.x;
            start.y=-Margins->top-offset.y;
            if (start.x<0)
                start.x=0;
            if (start.y>0)
                start.y=0;
            avail.x=GetDeviceCaps(hdc,HORZRES);
            avail.y=GetDeviceCaps(hdc,VERTRES);
            DPtoLP(hdc,&avail,1);

            printable.x=min(physsize.x-(Margins->right+Margins->left),avail.x-start.x);
            printable.y=max(physsize.y+(Margins->top+Margins->bottom),avail.y-start.y);
            
            LogX=GetDeviceCaps(hdc, LOGPIXELSX);
            LogY=GetDeviceCaps(hdc, LOGPIXELSY);

            SendMessage(ListWin, EM_FORMATRANGE, 0, 0);

            FORMATRANGE Range;
            memset(&Range,0,sizeof(FORMATRANGE));
            Range.hdc=hdc;
            Range.hdcTarget=hdc;
            LPtoDP(hdc,&start,1);
            LPtoDP(hdc,&printable,1);
            Range.rc.left = start.x * 1440 / LogX;
            Range.rc.top = start.y * 1440 / LogY;
            Range.rc.right = (start.x+printable.x) * 1440 / LogX;
            Range.rc.bottom = (start.y+printable.y) * 1440 / LogY;
            SetMapMode(hdc,MM_TEXT);

            BOOL PrintAborted=false;
            Range.rcPage = Range.rc;
            rcsaved = Range.rc;
            int CurrentPage = 1;
            int LastChar = 0;
            int LastChar2= 0;
            int MaxLen = SendMessage(ListWin,WM_GETTEXTLENGTH,0,0);
            Range.chrg.cpMax = -1;
            if (PrintPages) {
                do {
                    Range.chrg.cpMin = LastChar;
                    if (CurrentPage<PageFrom) {
                        LastChar = SendMessage(ListWin, EM_FORMATRANGE, 0, (LPARAM)&Range);
                    } else {
                        //waitform.ProgressLabel.Caption:=spage+inttostr(CurrentPage);
                        //application.processmessages;
                        LastChar = SendMessage(ListWin, EM_FORMATRANGE, 1, (LPARAM)&Range);
                    }
                    // Warning: At end of document, LastChar may be<MaxLen!!!
                    if (LastChar!=-1 && LastChar < MaxLen) {
                        Range.rc=rcsaved;                // Check whether another page comes
                        Range.rcPage = Range.rc;
                        Range.chrg.cpMin = LastChar;
                        LastChar2= SendMessage(ListWin, EM_FORMATRANGE, 0, (LPARAM)&Range);
                        if (LastChar<LastChar2 && LastChar < MaxLen && LastChar != -1 &&
                          CurrentPage>=PageFrom && CurrentPage<PageTo) {
                            EndPage(hdc);
                        }
                    }

                    CurrentPage++;
                    Range.rc=rcsaved;
                    Range.rcPage = Range.rc;
                } while (LastChar < MaxLen && LastChar != -1 && LastChar<LastChar2 &&
                         (PrintPages && CurrentPage<=PageTo) && !PrintAborted);
            } else {
                if (PrintSel) {
                    SendMessage(ListWin,EM_GETSEL,(WPARAM)&LastChar,(LPARAM)&MaxLen);
                    Range.chrg.cpMax = MaxLen;
                }
                do {
                    Range.chrg.cpMin = LastChar;
                    //waitform.ProgressLabel.Caption:=spage+inttostr(CurrentPage);
                    //waitform.ProgressLabel.update;
                    //application.processmessages;
                    LastChar = SendMessage(ListWin, EM_FORMATRANGE, 1, (LPARAM)&Range);

                    // Warning: At end of document, LastChar may be<MaxLen!!!
                    if (LastChar!=-1 && LastChar < MaxLen) {
                        Range.rc=rcsaved;                // Check whether another page comes
                        Range.rcPage = Range.rc;
                        Range.chrg.cpMin = LastChar;
                        LastChar2= SendMessage(ListWin, EM_FORMATRANGE, 0, (LPARAM)&Range);
                        if (LastChar<LastChar2 && LastChar < MaxLen && LastChar != -1) {
                            EndPage(hdc);
                        }
                    }
                    CurrentPage++;
                    Range.rc=rcsaved;
                    Range.rcPage = Range.rc;
                } while (LastChar<LastChar2 && LastChar < MaxLen && LastChar != -1 && !PrintAborted);
            }
            if (PrintAborted)
                AbortDoc(hdc);
            else
                EndDoc(hdc);
        } //StartDoc
  
        SendMessage(ListWin, EM_FORMATRANGE, 0, 0);
        DeleteDC(PrintDlgRec.hDC);
    }
    if (PrintDlgRec.hDevNames)
        GlobalFree(PrintDlgRec.hDevNames);
    return 0;
}
