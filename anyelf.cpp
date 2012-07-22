// listplug.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "listplug.h"
#include "cunicode.h"

#define supportedextension1 L".c"
#define supportedextension2 L".cpp"
#define supportedextension3 L".h"
#define supportedextension4 L".pas"
/* Note: in C, double quotes inside a string need to be escaped with a backslash!  */
#define parsefunction "force | (ext=\"C\" | ext=\"CPP\") & FIND(\"{\") | (ext=\"H\") | (ext=\"PAS\" & FINDI(\"BEGIN\"))"

HINSTANCE hinst;
HMODULE FLibHandle=0;
char inifilename[MAX_PATH]="lsplugin.ini";  // Unused in this plugin, may be used to save data

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hinst=(HINSTANCE)hModule;
			break;
		case DLL_PROCESS_DETACH:
			if (FLibHandle)
				FreeLibrary(FLibHandle);
			FLibHandle=NULL;
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
    }
    return TRUE;
}

char* strlcpy(char* p,char*p2,int maxlen)
{
	if ((int)strlen(p2)>=maxlen) {
		strncpy(p,p2,maxlen);
		p[maxlen]=0;
	} else
		strcpy(p,p2);
	return p;
}

char* AppendCurrentLine(char* inbuf,int currentline,int numdigits)
{
	char out1[16];

	itoa(currentline,out1,10);
	int fill=numdigits-strlen(out1);
	for (int i=0;i<fill;i++)
		inbuf[i]='0';
	strcpy(inbuf+fill,out1);
	strcat(inbuf,": ");
	return inbuf+strlen(inbuf);
}

char* InsertLineNumbers(char* inbuf,int buflen)
{
	int numbreaks=1;   //At least one line
	char lastchar=0;
	char* p;
	
	// First, count the number of line breaks
	p=inbuf;
	while (p[0]) {
		if (p[0]==13 || (p[0]==10 && lastchar!=13))
			numbreaks++;
		lastchar=p[0];
		p++;
	}
	// Get number of digits needed
	int numdigits=(int)floor(1+log10(numbreaks));
	
	int spaceneeded=buflen+numbreaks*(numdigits+2)+1;

	char* pout=(char*)malloc(spaceneeded);
	if (!pout)
		return NULL;

	int currentline=1;
	p=inbuf;
	char* p2=AppendCurrentLine(pout,currentline,numdigits);
	while (p[0]) {
		if (p[0]==13 || (p[0]==10 && lastchar!=13)) {
			currentline++;
			p2[0]=13;
			p2++;
			p++;
			lastchar=13;
			if (p[0]==10) {
				p2[0]=10;
				p2++;
				p++;
				lastchar=10;
			}
			p2=AppendCurrentLine(p2,currentline,numdigits);
		} else {
			lastchar=p[0];
			p2[0]=p[0];
			p++;
			p2++;
		}
	}
	p2[0]=0;
	return pout;
}

int lastloadtime=0;   // Workaround to RichEdit bug

int __stdcall ListNotificationReceived(HWND ListWin,int Message,WPARAM wParam,LPARAM lParam)
{
	switch (Message) {
	case WM_COMMAND:
		if (HIWORD(wParam)==EN_UPDATE && abs(GetCurrentTime()-lastloadtime)>1000) {
			int firstvisible=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
			int linecount=SendMessage(ListWin,EM_GETLINECOUNT,0,0);
			if (linecount>0) {
				int percent=MulDiv(firstvisible,100,linecount);
				PostMessage(GetParent(ListWin),WM_COMMAND,MAKELONG(percent,itm_percent),(LPARAM)ListWin);
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

HWND __stdcall ListLoadW(HWND ParentWin,WCHAR* FileToLoad,int ShowFlags)
{
	HWND hwnd;
	RECT r;
	DWORD w2;
	char *pdata;
	WCHAR *p;
	BOOL success=false;

	if (ShowFlags & lcp_forceshow==0) {  // don't check extension in this case!
		p=wcsrchr(FileToLoad,'\\');
		if (!p)
			return NULL;
		p=wcsrchr(p,'.');
		if (!p || (wcsicmp(p,supportedextension1)!=0 && wcsicmp(p,supportedextension2)!=0
			   && wcsicmp(p,supportedextension3)!=0 && wcsicmp(p,supportedextension4)!=0))
			return NULL;
	}
	// Extension is supported -> load file
	HANDLE f=CreateFileT(FileToLoad,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
							FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (f==INVALID_HANDLE_VALUE)
		return NULL;

	if (!FLibHandle) {
		int OldError = SetErrorMode(SEM_NOOPENFILEERRORBOX);
		FLibHandle = LoadLibrary("Riched20.dll");
		if ((int)FLibHandle < HINSTANCE_ERROR) 
			FLibHandle = LoadLibrary("RICHED32.DLL");
		if ((int)FLibHandle < HINSTANCE_ERROR) 
			FLibHandle = NULL;
		SetErrorMode(OldError);
	}

	lastloadtime=GetCurrentTime();

	GetClientRect(ParentWin,&r);
	// Create window invisbile, only show when data fully loaded!
	hwnd=CreateWindow("RichEdit20A","",WS_CHILD | ES_MULTILINE | ES_READONLY |
		                        WS_HSCROLL | WS_VSCROLL | ES_NOHIDESEL,
		r.left,r.top,r.right-r.left,
		r.bottom-r.top,ParentWin,NULL,hinst,NULL);
	if (!hwnd)
		hwnd=CreateWindow("RichEdit","",WS_CHILD | ES_MULTILINE | ES_READONLY |
		                        WS_HSCROLL | WS_VSCROLL | ES_NOHIDESEL,
		r.left,r.top,r.right-r.left,
		r.bottom-r.top,ParentWin,NULL,hinst,NULL);
	if (hwnd) {
		SendMessage(hwnd, EM_SETMARGINS, EC_LEFTMARGIN, 8);
		SendMessage(hwnd, EM_SETEVENTMASK, 0, ENM_UPDATE); //ENM_SCROLL doesn't work for thumb movements!

		PostMessage(ParentWin,WM_COMMAND,MAKELONG(lcp_ansi,itm_fontstyle),(LPARAM)hwnd);

		int l=GetFileSize(f,NULL);
		pdata=(char*)malloc(l+1);
		if (pdata) {
			ReadFile(f,pdata,l,&w2,NULL);
			if (w2<0) w2=0;
			if (w2>(DWORD)l) w2=l;
			pdata[w2]=0;
			if (strlen(pdata)==w2) {     // Make sure the file doesn't contain any 0x00 char!
				char *p2=InsertLineNumbers(pdata,w2);
				if (p2) {
					SetWindowText(hwnd,p2);
					free(p2);
					PostMessage(ParentWin,WM_COMMAND,MAKELONG(0,itm_percent),(LPARAM)hwnd);
					success=true;
				}
			}
			free(pdata);
		}
		if (!success) {
			DestroyWindow(hwnd);
			hwnd=NULL;
		}
	}
	CloseHandle(f);
	lastloadtime=GetCurrentTime();
	if (hwnd)
		ShowWindow(hwnd,SW_SHOW);
	return hwnd;
}

HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags)
{
	WCHAR FileToLoadW[wdirtypemax];
	return ListLoadW(ParentWin,awfilenamecopy(FileToLoadW,FileToLoad),ShowFlags);
}

int __stdcall ListLoadNextW(HWND ParentWin,HWND ListWin,WCHAR* FileToLoad,int ShowFlags)
{
	RECT r;
	DWORD w2;
	char *pdata;
	WCHAR *p;
	BOOL success=false;

	if (ShowFlags & lcp_forceshow==0) {  // don't check extension in this case!
		p=wcsrchr(FileToLoad,'\\');
		if (!p)
			return LISTPLUGIN_ERROR;
		p=wcsrchr(p,'.');
		if (!p || (wcsicmp(p,supportedextension1)!=0 && wcsicmp(p,supportedextension2)!=0
			   && wcsicmp(p,supportedextension3)!=0 && wcsicmp(p,supportedextension4)!=0))
			return LISTPLUGIN_ERROR;
	}
	// Extension is supported -> load file
	HANDLE f=CreateFileT(FileToLoad,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
							FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (f==INVALID_HANDLE_VALUE)
		return LISTPLUGIN_ERROR;

	lastloadtime=GetCurrentTime();

	GetClientRect(ParentWin,&r);
	// Create window invisbile, only show when data fully loaded!
	if (ListWin) {
		SetWindowText(ListWin,"");  // clear
		SendMessage(ListWin, EM_SETMARGINS, EC_LEFTMARGIN, 8);
		SendMessage(ListWin, EM_SETEVENTMASK, 0, ENM_UPDATE); //ENM_SCROLL doesn't work for thumb movements!

		PostMessage(ParentWin,WM_COMMAND,MAKELONG(lcp_ansi,itm_fontstyle),(LPARAM)ListWin);

		int l=GetFileSize(f,NULL);
		pdata=(char*)malloc(l+1);
		if (pdata) {
			ReadFile(f,pdata,l,&w2,NULL);
			if (w2<0) w2=0;
			if (w2>(DWORD)l) w2=l;
			pdata[w2]=0;
			if (strlen(pdata)==w2) {     // Make sure the file doesn't contain any 0x00 char!
				char *p2=InsertLineNumbers(pdata,w2);
				if (p2) {
					SetWindowText(ListWin,p2);
					free(p2);
					PostMessage(ParentWin,WM_COMMAND,MAKELONG(0,itm_percent),(LPARAM)ListWin);
					success=true;
				}
			}
			free(pdata);
		}
		if (!success) {
			return LISTPLUGIN_ERROR;
		}
	}
	CloseHandle(f);
	lastloadtime=GetCurrentTime();
	return LISTPLUGIN_OK;
}

int __stdcall ListLoadNext(HWND ParentWin,HWND ListWin,char* FileToLoad,int ShowFlags)
{
	WCHAR FileToLoadW[wdirtypemax];
	return ListLoadNextW(ParentWin,ListWin,awfilenamecopy(FileToLoadW,FileToLoad),ShowFlags);
}

int __stdcall ListSendCommand(HWND ListWin,int Command,int Parameter)
{
	switch (Command) {
	case lc_copy:
		SendMessage(ListWin,WM_COPY,0,0);
		return LISTPLUGIN_OK;
	case lc_newparams:
		PostMessage(GetParent(ListWin),WM_COMMAND,MAKELONG(0,itm_next),(LPARAM)ListWin);
		return LISTPLUGIN_ERROR;
	case lc_selectall:
		SendMessage(ListWin,EM_SETSEL,0,-1);
		return LISTPLUGIN_OK;
	case lc_setpercent:
		int firstvisible=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
		int linecount=SendMessage(ListWin,EM_GETLINECOUNT,0,0);
		if (linecount>0) {
			int pos=MulDiv(Parameter,linecount,100);
			SendMessage(ListWin,EM_LINESCROLL,0,pos-firstvisible);
			firstvisible=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
			// Place caret on first visible line!
			int firstchar=SendMessage(ListWin,EM_LINEINDEX,firstvisible,0);
			SendMessage(ListWin,EM_SETSEL,firstchar,firstchar);
			pos=MulDiv(firstvisible,100,linecount);
			// Update percentage display
			PostMessage(GetParent(ListWin),WM_COMMAND,MAKELONG(pos,itm_percent),(LPARAM)ListWin);
			return LISTPLUGIN_OK;
		}
		break;
	}
	return LISTPLUGIN_ERROR;
}

int _stdcall ListSearchText(HWND ListWin,char* SearchString,int SearchParameter)
{
	FINDTEXT find;
	int StartPos,Flags;

	if (SearchParameter & lcs_findfirst) {
		//Find first: Start at top visible line
		StartPos=SendMessage(ListWin,EM_LINEINDEX,SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0),0);
		SendMessage(ListWin,EM_SETSEL,StartPos,StartPos);
	} else {
		//Find next: Start at current selection+1
		SendMessage(ListWin,EM_GETSEL,(WPARAM)&StartPos,0);
		StartPos+=1;
	}

	find.chrg.cpMin=StartPos;
	find.chrg.cpMax=SendMessage(ListWin,WM_GETTEXTLENGTH,0,0);
	Flags=0;
	if (SearchParameter & lcs_wholewords)
		Flags |= FR_WHOLEWORD;
	if (SearchParameter & lcs_matchcase)
		Flags |= FR_MATCHCASE;
	if (!(SearchParameter & lcs_backwards))
		Flags |= FR_DOWN;
	find.lpstrText=SearchString;
	int index=SendMessage(ListWin, EM_FINDTEXT, Flags, (LPARAM)&find);
	if (index!=-1) {
	  int indexend=index+strlen(SearchString);
	  SendMessage(ListWin,EM_SETSEL,index,indexend);
	  int line=SendMessage(ListWin,EM_LINEFROMCHAR,index,0)-3;
	  if (line<0)
		  line=0;
      line-=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
      SendMessage(ListWin,EM_LINESCROLL,0,line);
	  return LISTPLUGIN_OK;
	} else {
		SendMessage(ListWin,EM_SETSEL,-1,-1);  // Restart search at the beginning
	}
	return LISTPLUGIN_ERROR;
}

void __stdcall ListCloseWindow(HWND ListWin)
{
	DestroyWindow(ListWin);
	return;
}

int __stdcall ListPrint(HWND ListWin,char* FileToPrint,char* DefPrinter,int PrintFlags,RECT* Margins)
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

void __stdcall ListGetDetectString(char* DetectString,int maxlen)
{
	strlcpy(DetectString,parsefunction,maxlen);
}

void __stdcall ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	strlcpy(inifilename,dps->DefaultIniName,MAX_PATH-1);
}

HBITMAP __stdcall ListGetPreviewBitmapW(WCHAR* FileToLoad,int width,int height,
    char* contentbuf,int contentbuflen)
{
	RECT r;
	WCHAR *p;
	HBITMAP retval=NULL;
	char* newbuf;
	int bigx,bigy,fntsize;
	HDC maindc,dc_small,dc_big;
	HBITMAP bmp_big,bmp_small,oldbmp_big,oldbmp_small;
	HFONT font,oldfont;
	POINT pt;
	char ch;
	OSVERSIONINFO vx;
	BOOL is_nt;

	// check for operating system:
	// Windows 9x does NOT support the HALFTONE stretchblt mode!
	vx.dwOSVersionInfoSize=sizeof(vx);
	GetVersionEx(&vx);
	is_nt=vx.dwPlatformId==VER_PLATFORM_WIN32_NT;

	p=wcsrchr(FileToLoad,'\\');
	if (!p)
		return NULL;
	p=wcsrchr(p,'.');
	if (!p || (wcsicmp(p,supportedextension1)!=0 && wcsicmp(p,supportedextension2)!=0
		   && wcsicmp(p,supportedextension3)!=0 && wcsicmp(p,supportedextension4)!=0))
		return NULL;
	
	if (!contentbuf || contentbuflen<=0)
		return NULL;

	
	ch=contentbuf[contentbuflen];
	contentbuf[contentbuflen]=0;
	newbuf=InsertLineNumbers(contentbuf,contentbuflen);
	contentbuf[contentbuflen]=ch;  // make sure that contentbuf is NOT modified!

	if (is_nt) {
		bigx=width*2;
		bigy=height*2;
		fntsize=10;
	} else {
		bigx=width*2;
		bigy=height*2;
		fntsize=5;
	}

	maindc=GetDC(GetDesktopWindow());
    dc_small=CreateCompatibleDC(maindc);
    dc_big=CreateCompatibleDC(maindc);
    bmp_big=CreateCompatibleBitmap(maindc,bigx,bigy);
    bmp_small=CreateCompatibleBitmap(maindc,width,height);
    ReleaseDC(GetDesktopWindow(),maindc);
    oldbmp_big=(HBITMAP)SelectObject(dc_big,bmp_big);
    r.left=0;
    r.top=0;
    r.right=bigx;
    r.bottom=bigy;
    font=CreateFont(-MulDiv(fntsize, GetDeviceCaps(dc_big, LOGPIXELSY), 72),0,0,0,FW_NORMAL,
      0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,"Arial");
    oldfont=(HFONT)SelectObject(dc_big,font);
    FillRect(dc_big,&r,(HBRUSH)GetStockObject(WHITE_BRUSH));
    DrawText(dc_big,newbuf,strlen(newbuf),&r,DT_EXPANDTABS);
    SelectObject(dc_big,oldfont);
    DeleteObject(font);

    if (is_nt) {
		oldbmp_small=(HBITMAP)SelectObject(dc_small,bmp_small);
		SetStretchBltMode(dc_small,HALFTONE);
		SetBrushOrgEx(dc_small,0,0,&pt);
		StretchBlt(dc_small,0,0,width,height,dc_big,0,0,bigx,bigy,SRCCOPY);
		SelectObject(dc_small,oldbmp_small);
	    SelectObject(dc_big,oldbmp_big);
		DeleteObject(bmp_big);
    } else {
	    SelectObject(dc_big,oldbmp_big);
		DeleteObject(bmp_small);
		bmp_small=bmp_big;
    }	
    DeleteDC(dc_big);
    DeleteDC(dc_small);
	free(newbuf);
	return bmp_small;
}

HBITMAP __stdcall ListGetPreviewBitmap(char* FileToLoad,int width,int height,
    char* contentbuf,int contentbuflen)
{
	WCHAR FileToLoadW[wdirtypemax];
	return ListGetPreviewBitmapW(FileToLoadW,width,height,
		contentbuf,contentbuflen);
}

int _stdcall ListSearchDialog(HWND ListWin,int FindNext)
{
/*	if (FindNext)
		MessageBox(ListWin,"Find Next","test",0);
	else
		MessageBox(ListWin,"Find First","test",0);*/
	return LISTPLUGIN_ERROR;  // use ListSearchText instead!
}
