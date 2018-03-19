// Image_Explorer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Image_Explorer.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
char szDefaultPath[MAX_PATH];					//主程序默认路径
char szCurrentPath[MAX_PATH];					//打开文件的当前路径
char szCurrentFile[MAX_PATH];					//打开文件的全文件名
char szFontTIM2File[MAX_MULTIFILE];				//存放字库tim2的各子文件名
HWND hWndExplorer = NULL, hWndChild, hWndStatus, hWndPB;
FILE *fpIn,*fpOut,*fpIm,*fpBak, *fpRawEx, *fpRawIm, *fpIndex;
int fileNum, datFileNum, siImageNum;
FILEINFO *fi;
TXPINFO *ti;
PAL *pal;
TIM2 *tim2;
DATINFO *di;
SUBINFO *si;
SUBPALINFO *spi;
SUBIMAGEINFO *sii;
SUBIMAGEFILEINFO *sifi;
GM3IMAGEINFO *gii;
SORTDATA sd;
BYTE *pBufIm;
char szBpp[5];
int layer=0, iGPalNum=16;
bool layerChange=false, bFileChange=false, bSorted=false;
int i, j, k, tileWidth, tileHeight, iImPalNo;
int index, preIndex0, preIndex1;
bool bEx_Import=false, bExCurFile=false, bExAllFile=false, bImFile=false;
long fpos;
bool bHg2=false, bPrinny=true, bTRP1=false, bOpenHg2Data=false;
DWORD dwTRP1Offset;
HBITMAP hGraph;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_IMAGE_EXPLORER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IMAGE_EXPLORER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if(!IsWindow(hWndExplorer) || !IsDialogMessage(hWndExplorer, &msg))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return (int) msg.wParam;
}

void MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= 0;// CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMAGE_EXPLORER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_IMAGE_EXPLORER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassEx(&wcex);

	wcex.style			= 0;//CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= ChildWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= 0;
	wcex.hCursor		= 0;
	wcex.hbrBackground	= (HBRUSH)(COLOR_GRAYTEXT+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= "ChildWin";
	wcex.hIconSm		= 0;
	RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowEx(NULL, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME ofn;
	static char szFileIn[MAX_PATH],szFileInName[MAX_PATH],szSBText[MAX_PATH], szIndexFile[MAX_PATH], szWinTitle[MAX_PATH];
	int wmId, wmEvent, iCharCount;
	PAINTSTRUCT ps;
	HDC hdc;
	static int cxClient,cyClient;
	POINT pt;
	RECT rcClt, rcDlg, rcStatus, rcPB;
	char szFlag[8], szFileBak[MAX_PATH];
	int nSBWidth[3];
	BYTE *lpBakBuf;
	DWORD dwTest[2];

	switch (message)
	{
	case WM_CREATE:
		InitCommonControls();
		hWndChild = CreateWindowEx(WS_EX_CLIENTEDGE, "ChildWin", "ChildWin", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
									CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, NULL, hInst, NULL);
		ShowWindow(hWndChild, SW_SHOW);
		UpdateWindow(hWndChild);

		//创建状态栏
 		hWndStatus = CreateWindowEx(0, STATUSCLASSNAME, (LPCTSTR) "SB", WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | SBARS_SIZEGRIP,
									0, 0, 0, 0, hWnd, (HMENU) ID_STATUSBAR, hInst, NULL);

		//创建进度条
		GetClientRect(hWndStatus, &rcClt);
		hWndPB = CreateWindow(PROGRESS_CLASS, "PB", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
								2, 3, 200, rcClt.bottom-rcClt.top-5, hWndStatus, NULL, hInst, NULL);

		GetCurrentDirectory(MAX_PATH,(LPTSTR)szDefaultPath);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_OPEN:
			ZeroMemory(szFileIn, sizeof(szFileIn[MAX_PATH]));
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFileIn;
			ofn.nMaxFile = sizeof(szFileIn);
			ofn.lpstrFilter = "BIN File(*.bin)\0*.bin\0DAT File(*.dat)\0*.dat\0All Files(*.*)\0*.*\0\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = szFileInName;
			ofn.nMaxFileTitle = sizeof(szFileInName);
			ofn.lpstrInitialDir = NULL;
			//ofn.lpstrDefExt = "vlg";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

			if (GetOpenFileName(&ofn)!=TRUE)
				break;
			bFileChange=true;
			if(NULL!=fpIn)
				fclose(fpIn);
			if (fopen_s(&fpIn,szFileIn,"rb+")!=NULL)
			{
				MessageBox(hWnd, "Can't open file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
				bFileChange=false;
				break;
			}
			sprintf_s(szWinTitle, MAX_PATH, "%s-%s", szTitle, szFileIn);
			SetWindowText(hWnd, szWinTitle);
			GetCurrentDirectory(MAX_PATH,(LPTSTR)szCurrentPath);
			strcpy_s(szCurrentFile, MAX_PATH, szFileIn);
			if(NULL!=fpIn)
			{
				free(fi);
				fi=NULL;
				free(ti);
				ti=NULL;
				free(di);
				di=NULL;
				free(si);
				si=NULL;
				free(spi);
				spi=NULL;
				free(sii);
				sii=NULL;
				free(sifi);
				sifi=NULL;
			}
			//初始化全局标志变量
			bHg2=false;
			bPrinny=true;
			bTRP1=false;
			bOpenHg2Data=false;
			//Get opened file information
			fread(szFlag, 8, 1, fpIn);
			if(strcmp(szFlag, "NISPACK"))//非NISPACK文件
			{
				rewind(fpIn);
				fread(&dwTest[0], 4, 1, fpIn);
				fread(&dwTest[1], 4, 1, fpIn);
				if(0==dwTest[1])
				{
					fread(&dwTest[1], 4, 1, fpIn);
					if(0==dwTest[1])
					{
						fread(&dwTest[1], 4, 1, fpIn);
						if(0==dwTest[1])
						{//处理单个的hg2 dat文件
							fileNum=1;
							fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
							if (NULL==fi)
							{
								MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
								bFileChange=false;
								break;
							}
							memset(fi, 0, fileNum*sizeof(FILEINFO));
							ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
							if (NULL==ti)
							{
								MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
								bFileChange=false;
								break;
							}
							memset(ti, 0, fileNum*sizeof(TXPINFO));
							fi[0].Offset=0;
							sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
							sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
							sprintf_s(&fi[0].szType[0], 8, "DAT");
							fseek(fpIn, 0L, 2);
							fi[0].Size=ftell(fpIn);
						}
						else if (0x010000==dwTest[1])//pa中的DXT格式TXP图片
						{
							fileNum=1;
							fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
							if (NULL==fi)
							{
								MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
								bFileChange=false;
								break;
							}
							memset(fi, 0, fileNum*sizeof(FILEINFO));
							ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
							if (NULL==ti)
							{
								MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
								bFileChange=false;
								break;
							}
							memset(ti, 0, fileNum*sizeof(TXPINFO));
							fi[0].Offset=0;
							sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
							sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
							sprintf_s(&fi[0].szType[0], 8, "TXP");
							fseek(fpIn, 0L, 2);
							fi[0].Size=ftell(fpIn);
						}
					}
				}
				else if ((dwTest[0]>>3)-1==dwTest[1] || (dwTest[0]>>3)-2==dwTest[1])
				{
					fileNum=1;
					fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
					if (NULL==fi)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(fi, 0, fileNum*sizeof(FILEINFO));
					ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
					if (NULL==fi)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(ti, 0, fileNum*sizeof(TXPINFO));
					fi[0].Offset=0;
					sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
					sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
					sprintf_s(&fi[0].szType[0], 8, "MTXP");
					fseek(fpIn, 0L, 2);
					fi[0].Size=ftell(fpIn);
				}
				else if (0x31505254==dwTest[0])
				{
					fileNum=1;
					fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
					if (NULL==fi)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(fi, 0, fileNum*sizeof(FILEINFO));
					ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
					if (NULL==ti)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(ti, 0, fileNum*sizeof(TXPINFO));
					fi[0].Offset=0;
					sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
					sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
					sprintf_s(&fi[0].szType[0], 8, "TRP1");
					fseek(fpIn, 0L, 2);
					fi[0].Size=ftell(fpIn);
				}
				else if (0x10==dwTest[1] || 0x100==dwTest[1])////普通TXP图片
				{
					fileNum=1;
					fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
					if (NULL==fi)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(fi, 0, fileNum*sizeof(FILEINFO));
					ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
					if (NULL==ti)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(ti, 0, fileNum*sizeof(TXPINFO));
					fi[0].Offset=0;
					sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
					sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
					sprintf_s(&fi[0].szType[0], 8, "TXP");
					fseek(fpIn, 0L, 2);
					fi[0].Size=ftell(fpIn);
				}
				else if ((0==(dwTest[1]>>16) || 2==(dwTest[1]>>16)) && !(dwTest[1]-(dwTest[0]*0x20+4)<=0x10))//DXT格式TXP图片
				{
					fileNum=1;
					fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
					if (NULL==fi)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(fi, 0, fileNum*sizeof(FILEINFO));
					ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
					if (NULL==ti)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(ti, 0, fileNum*sizeof(TXPINFO));
					fi[0].Offset=0;
					sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
					sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
					sprintf_s(&fi[0].szType[0], 8, "TXP");
					fseek(fpIn, 0L, 2);
					fi[0].Size=ftell(fpIn);
				}
				else if (((DWORD)1<<(dwTest[1]>>24) >= (dwTest[0]>>16)) && !(dwTest[1]-(dwTest[0]*0x20+4)<=0x10))
				{
					fileNum=1;
					fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
					if (NULL==fi)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(fi, 0, fileNum*sizeof(FILEINFO));
					ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
					if (NULL==ti)
					{
						MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bFileChange=false;
						break;
					}
					memset(ti, 0, fileNum*sizeof(TXPINFO));
					fi[0].Offset=0;
					sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
					sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
					sprintf_s(&fi[0].szType[0], 8, "TX2");
					fseek(fpIn, 0L, 2);
					fi[0].Size=ftell(fpIn);
				}
				else
				{
					fseek(fpIn, 4L, 1);
					fread(&dwTest[0], 4, 1, fpIn);
					if (1<<(dwTest[1] & 0x000000ff)==dwTest[0]>>16)
					{
						fileNum=1;
						fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
						if (NULL==fi)
						{
							MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bFileChange=false;
							break;
						}
						memset(fi, 0, fileNum*sizeof(FILEINFO));
						ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
						if (NULL==ti)
						{
							MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bFileChange=false;
							break;
						}
						memset(ti, 0, fileNum*sizeof(TXPINFO));
						fi[0].Offset=0;
						sprintf_s(&fi[0].szOffset[0], 11, "0x%08X", fi[0].Offset);
						sprintf_s(&fi[0].szName[0], 28, "%s", szFileInName);
						sprintf_s(&fi[0].szType[0], 8, "TXP1");
						fseek(fpIn, 0L, 2);
						fi[0].Size=ftell(fpIn);
					}
					else
					{
						//默认为处理start.lzs解压后的文件
						rewind(fpIn);
						fread(&fileNum, 4, 1, fpIn);
						fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
						if (NULL==fi)
						{
							MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bFileChange=false;
							break;
						}
						ZeroMemory(fi, fileNum*sizeof(FILEINFO));
						ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
						if (NULL==ti)
						{
							MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bFileChange=false;
							break;
						}
						ZeroMemory(ti, fileNum*sizeof(TXPINFO));
						for(i=0;i<fileNum;i++)
						{
							fread(&fi[i].Offset, 4, 1, fpIn);
							sprintf_s(&fi[i].szOffset[0], 11, "0x%08X", fi[i].Offset);
							fread(&fi[i].szName, 28, 1, fpIn);
							strcpy_s(&fi[i].szType[0], 8, strchr(fi[i].szName, '.')+1);
							_strupr(fi[i].szType);
						}
						for(i=0;i<fileNum-1;i++)
						{
							fi[i].Size=fi[i+1].Offset-fi[i].Offset;
						}
						fseek(fpIn, 0L, 2);
						fi[fileNum-1].Size=ftell(fpIn)-fi[fileNum-1].Offset;
					}
				}
			}
			else//NISPACK文件
			{
				fseek(fpIn, 4L, 1);
				fread(&fileNum, 4, 1, fpIn);
				fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
				if (NULL==fi)
				{
					MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
					bFileChange=false;
					break;
				}
				ZeroMemory(fi, fileNum*sizeof(FILEINFO));
				ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
				if (NULL==ti)
				{
					MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
					bFileChange=false;
					break;
				}
				ZeroMemory(ti, fileNum*sizeof(TXPINFO));
				for(i=0;i<fileNum;i++)
				{
					fread(&fi[i].szName, 32, 1, fpIn);
					strcpy_s(&fi[i].szType[0], 8, strchr(fi[i].szName, '.')+1);
					_strupr(fi[i].szType);
					fread(&fi[i].Offset, 4, 1, fpIn);
					sprintf_s(&fi[i].szOffset[0], 11, "0x%08X", fi[i].Offset);
					fread(&fi[i].Size, 4, 1, fpIn);
					fread(&fi[i].Unknow, 4, 1, fpIn);
				}
			}
			layer=0;
			SendMessage (hWnd, WM_COMMAND, (WPARAM)(0x0000ffff&IDM_EXPLORER_VIEW), (LPARAM)0);
			bFileChange=false;
			break;
		case IDM_OPENHG2DATA:
			memset(szFileIn, 0, sizeof(szFileIn[MAX_PATH]));
			memset(&ofn, 0, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFileIn;
			ofn.nMaxFile = sizeof(szFileIn);
			ofn.lpstrFilter = "hg2 DATA File(data.dat)\0data.dat\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = szFileInName;
			ofn.nMaxFileTitle = sizeof(szFileInName);
			ofn.lpstrInitialDir = NULL;
			//ofn.lpstrDefExt = "vlg";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

			if (GetOpenFileName(&ofn)!=TRUE)
				break;
			bFileChange=true;
			if(NULL!=fpIn)
				fclose(fpIn);
			if (fopen_s(&fpIn,szFileIn,"rb+")!=NULL)
			{
				MessageBox(hWnd, "Can't open hg2 DATA file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
				bFileChange=false;
				break;
			}
			sprintf_s(szIndexFile, MAX_PATH, "%s//index.bin", szDefaultPath);
			if (fopen_s(&fpIndex, szIndexFile, "rb")!=NULL)
			{
				MessageBox(hWnd, "Can't open index.bin!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpIn);
				bFileChange=false;
				break;
			}
			bOpenHg2Data=true;
			sprintf_s(szWinTitle, MAX_PATH, "%s-%s", szTitle, szFileIn);
			SetWindowText(hWnd, szWinTitle);
			GetCurrentDirectory(MAX_PATH,(LPTSTR)szCurrentPath);
			strcpy_s(szCurrentFile, MAX_PATH, szFileIn);
			if(NULL!=fpIn)
			{
				free(fi);
				fi=NULL;
				free(ti);
				ti=NULL;
				free(di);
				di=NULL;
				free(si);
				si=NULL;
				free(spi);
				spi=NULL;
				free(sii);
				sii=NULL;
				free(sifi);
				sifi=NULL;
			}
			//读取索引信息
			fileNum=FILENUM;
			fi=(FILEINFO*)malloc(fileNum*sizeof(FILEINFO));
			if (NULL==fi)
			{
				MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				bFileChange=false;
				break;
			}
			memset(fi, 0, fileNum*sizeof(FILEINFO));
			ti=(TXPINFO*)malloc(fileNum*sizeof(TXPINFO));
			if (NULL==ti)
			{
				MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				bFileChange=false;
				break;
			}
			memset(ti, 0, fileNum*sizeof(TXPINFO));

			for(i=0; i<FILENUM; i++)
			{
				fread(&fi[i].Offset, 4, 1, fpIndex);
				fread(&fi[i].Size, 4, 1, fpIndex);
				sprintf_s(fi[i].szName, 8, "%03X.bin", i);
				fseek(fpIn, fi[i].Offset, 0);
				fread(&fi[i].szType, 8, 1, fpIn);
				iCharCount=ConfirmType(fi[i].szType);
				if(iCharCount>=3 && iCharCount<=4)
					fi[i].szType[iCharCount]='\0';
				else if(iCharCount>4 && iCharCount<7)
					fi[i].szType[4]='\0';
				else if(7==iCharCount)
					fi[i].szType[iCharCount]='\0';
				else
				{
					if(0x02<=i && 0x15>=i)
						sprintf_s(fi[i].szType, 8, "DAT");
					else if(0x16<=i && 0x1e>=i)
						sprintf_s(fi[i].szType, 8, "MTXP");
					else if((0x2a<=i && 0x33f>=i && 0x1b5!=i) || 0==i)
						sprintf_s(fi[i].szType, 8, "TXP");
					else if((0x340<=i && 0x408>=i) || 0x1b5==i)
						sprintf_s(fi[i].szType, 8, "TXP1");
					else
						sprintf_s(fi[i].szType, 8, "RAW");
				}
				sprintf_s(fi[i].szOffset, 11, "0x%08X", fi[i].Offset);
				//sprintf_s(fi[i].szSize, 11, "0x%08X", fi[i].Size);
			}
			fclose(fpIndex);
			layer=0;
			SendMessage (hWnd, WM_COMMAND, (WPARAM)(0x0000ffff&IDM_EXPLORER_VIEW), (LPARAM)0);
			bFileChange=false;
			break;
		case IDM_CLOSE:
			bFileChange=true;
			if(NULL!=fpIn)
			{
				fclose(fpIn);
				fpIn=NULL;
				free(fi);
				fi=NULL;
				free(ti);
				ti=NULL;
				free(di);
				di=NULL;
				free(si);
				si=NULL;
				free(spi);
				spi=NULL;
				free(sii);
				sii=NULL;
				free(sifi);
				sifi=NULL;
			}
			SetWindowText(hWnd, szTitle);
			strcpy_s(szCurrentFile, MAX_PATH, "");
			//初始化全局标志变量
			bHg2=false;
			bPrinny=true;
			bTRP1=false;
			bOpenHg2Data=false;

			if (!IsWindow(hWndExplorer) || bFileChange)
			{
				if(bFileChange && IsWindow(hWndExplorer))
				{
					DestroyWindow(hWndExplorer);
					hWndExplorer = NULL;
				}
				hWndExplorer = CreateDialog(hInst, MAKEINTRESOURCE(IDD_EXPLORER), hWnd, (DLGPROC) ExplorerProc);
				GetClientRect(hWnd, &rcClt);
				GetWindowRect(hWndExplorer, &rcDlg);
				pt.x=rcClt.right-(rcDlg.right-rcDlg.left);
				pt.y=0;
				ClientToScreen(hWnd, &pt);
				SetWindowPos(hWndExplorer, hWnd, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				ShowWindow(hWndExplorer, SW_SHOW);
			}
			bFileChange=false;
			break;
		case IDM_BACKUP:
			//备份文件
			if(NULL==fpIn)
				break;
			sprintf_s(szFileBak, MAX_PATH, "%s.bak", szFileIn);
			if (fopen_s(&fpBak,szFileBak,"wb")!=NULL)
			{
				MessageBox(hWnd, "Can't create backup file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
				break;
			}
			fseek(fpIn, 0L, 2);
			fpos=ftell(fpIn);
			rewind(fpIn);
			lpBakBuf=(BYTE*)malloc(fpos*sizeof(BYTE));
			if (NULL==lpBakBuf)
			{
				MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpBak);
				break;
			}
			memset(lpBakBuf, 0, fpos*sizeof(BYTE));
			fread(lpBakBuf, fpos, 1, fpIn);
			fwrite(lpBakBuf, fpos, 1, fpBak);
			free(lpBakBuf);
			lpBakBuf=NULL;
			fclose(fpBak);
			MessageBox(hWnd, "Backup file success!", szTitle, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
			break;
		case IDM_SAVE:
			//保存文件
			if(NULL==fpIn)
				break;
			if(0==layer)
			{
				if (bPrinny)
				{
					fseek(fpIn, fi[index].Offset+0x10+iImPalNo*ti[index].clrb*4, 0);
					//fpos=ftell(fpIn);
					if(bTRP1)
						fseek(fpIn, dwTRP1Offset, 1);
					fwrite(pal, 1, ti[index].clrb*4, fpIn);
					fseek(fpIn, fi[index].Offset+0x10+ti[index].clrb*ti[index].palNum*4L, 0);
					if(bTRP1)
						fseek(fpIn, dwTRP1Offset, 1);
				} 
				else if(bHg2)
				{
					fseek(fpIn, fi[index].Offset+0x10, 0);
				}
				if(0x10==ti[index].clrb || 0==ti[index].clrb)
					fwrite(pBufIm, 1, ti[index].cx*ti[index].cy/2, fpIn);
				else if(0x100==ti[index].clrb || 2==ti[index].clrb)
					fwrite(pBufIm, 1, ti[index].cx*ti[index].cy, fpIn);
			}
			else if(11==layer || 12==layer)
			{
				fseek(fpIn, di[index].Offset+0x10+iImPalNo*gii[index].clrb*4, 0);
				fpos=ftell(fpIn);
				fwrite(pal, 1, gii[index].clrb*4, fpIn);
				fseek(fpIn, di[index].Offset+0x10+gii[index].clrb*gii[index].palNum*4L, 0);
				if(0x10==gii[index].clrb)
					fwrite(pBufIm, 1, gii[index].cx*gii[index].cy/2, fpIn);
				else if(0x100==gii[index].clrb)
					fwrite(pBufIm, 1, gii[index].cx*gii[index].cy, fpIn);
			}
			else if(2==layer)
			{
				if(bPrinny)
					fseek(fpIn, spi[sii[index].palIndex].PalOffset+iImPalNo*0x40, 0);
				else if(bHg2)
					fseek(fpIn, spi[iImPalNo].PalOffset, 0);

				if(0x04==sii[index].bpp)
					fwrite(pal, 1, 0x40, fpIn);
				else if(0x08==sii[index].bpp)
					fwrite(pal, 1, 0x400, fpIn);
				fseek(fpIn, sii[index].ImageOffset, 0);
				fwrite(pBufIm, 1, sii[index].cx*sii[index].cy*sii[index].bpp/0x08, fpIn);
			}
			fclose(fpIn);
			free(pBufIm);
			pBufIm=NULL;
			free(pal);
			pal=NULL;
			if (fopen_s(&fpIn,szFileIn,"rb+")!=NULL)
			{
				MessageBox(hWnd, "Reopen file failed!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
				break;
			}
			MessageBox(hWnd, "Save file success!", szTitle, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
			break;
		case IDM_EXPLORER_VIEW:
			if (!IsWindow(hWndExplorer) || bFileChange)
			{
				if(bFileChange && IsWindow(hWndExplorer))
				{
					DestroyWindow(hWndExplorer);
					hWndExplorer = NULL;
				}
				hWndExplorer = CreateDialog(hInst, MAKEINTRESOURCE(IDD_EXPLORER), hWnd, (DLGPROC) ExplorerProc);
				GetClientRect(hWnd, &rcClt);
				GetWindowRect(hWndExplorer, &rcDlg);
				pt.x=rcClt.right-(rcDlg.right-rcDlg.left);
				pt.y=0;
				ClientToScreen(hWnd, &pt);
				SetWindowPos(hWndExplorer, hWnd, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				ShowWindow(hWndExplorer, SW_SHOW);
			}
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_MOVE:
		GetClientRect(hWnd, &rcClt);
		GetWindowRect(hWndExplorer, &rcDlg);
		pt.x=rcClt.right-(rcDlg.right-rcDlg.left);
		pt.y=0;
		ClientToScreen(hWnd, &pt);
		SetWindowPos(hWndExplorer, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		//ShowWindow(hWndExplorer, SW_SHOW);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_USER+1:
		if(0==layer)
		{
			strcpy_s(szSBText, MAX_PATH, szFileIn);
			strcat_s(szSBText, MAX_PATH, "->");
			strcat_s(szSBText, MAX_PATH, fi[index].szName);
		}
		else if(1==layer || 11==layer || 12==layer)
		{
			strcpy_s(szSBText, MAX_PATH, szFileIn);
			strcat_s(szSBText, MAX_PATH, "->");
			strcat_s(szSBText, MAX_PATH, di[index].szParentName);
			strcat_s(szSBText, MAX_PATH, "->");
			strcat_s(szSBText, MAX_PATH, di[index].szName);
		}
		else if(2==layer)
		{
			strcpy_s(szSBText, MAX_PATH, szFileIn);
			strcat_s(szSBText, MAX_PATH, "->");
			strcat_s(szSBText, MAX_PATH, sifi[index].szParentName);
			strcat_s(szSBText, MAX_PATH, "->");
			strcat_s(szSBText, MAX_PATH, sifi[index].szName);
		}
		SendMessage(hWndStatus, SB_SETTEXT, 1, (LPARAM)(LPSTR)szSBText);
		if(NULL!=lParam)
			SendMessage(hWndStatus, SB_SETTEXT, 2, (LPARAM)(LPSTR)szBpp);
		break;
	case WM_SIZE:
		cxClient = LOWORD (lParam);
		cyClient = HIWORD (lParam);

		if(NULL!=hWndStatus)
		{
			GetWindowRect(hWndStatus, &rcStatus);
			SetWindowPos(hWndChild, NULL, 0, 0, cxClient, cyClient-(rcStatus.bottom-rcStatus.top), SWP_NOZORDER);
			SetWindowPos(hWndStatus, NULL, 0, cyClient-(rcStatus.bottom-rcStatus.top), cxClient, cyClient, SWP_NOZORDER);
		}

		//设置状态栏的分栏大小
		GetWindowRect(hWndPB, &rcPB);
		GetClientRect(hWnd, &rcClt);
		nSBWidth[0] = 8+rcPB.right-rcPB.left;
		nSBWidth[1] = rcClt.right-60;
		nSBWidth[2] = rcClt.right-20;
 		SendMessage(hWndStatus, SB_SETPARTS, (WPARAM) 3, (LPARAM)&nSBWidth);
		break;
	case WM_DESTROY:
		if(NULL!=fpIn)
		{
			fclose(fpIn);
			free(fi);
			fi=NULL;
			free(ti);
			ti=NULL;
			free(di);
			di=NULL;
			free(si);
			si=NULL;
			free(spi);
			spi=NULL;
			free(sii);
			sii=NULL;
			free(sifi);
			sifi=NULL;
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rc;
	static HDC hMem;
	static SCROLLINFO si;
	short sWheelDelta;
	static int cxClient,cyClient, xMaxScroll, xMinScroll=0, yMaxScroll, yMinScroll=0, xCurrentScroll, yCurrentScroll;

	switch (message)
	{
	/*case WM_CREATE:
		break;*/
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if(hMem!=NULL)
		{
			if(0==layer)
				BitBlt(hdc, 0, 0, ti[index].cx, ti[index].cy, hMem, xCurrentScroll, yCurrentScroll, SRCCOPY);
			else if(11==layer || 12==layer)
				BitBlt(hdc, 0, 0, gii[index].cx, gii[index].cy, hMem, xCurrentScroll, yCurrentScroll, SRCCOPY);
			else if(2==layer)
				BitBlt(hdc, 0, 0, sii[index].cx, sii[index].cy, hMem, xCurrentScroll, yCurrentScroll, SRCCOPY);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_USER+2:
		if(0==layer)
		{
			if (bPrinny)
			{
				if(!TIM22TXP(hMem))
				{
					MessageBox(hWnd, "Can't support this file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
					break;
				}
			}
			else if(bHg2)
			{
				if(!TIM22DXTn(hMem))
				{
					MessageBox(hWnd, "Can't support this file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
					break;
				}
			}
		}
		else if(11==layer || 12==layer)
		{
			if(!TIM22GM3(hMem))
			{
				MessageBox(hWnd, "Can't support this file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
				break;
			}
		}
		else if(2==layer)
		{
			if(!TIM22DAT(hMem))
			{
				MessageBox(hWnd, "Can't support this file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
				break;
			}
		}
		InvalidateRect(hWnd, NULL, true);
		break;
	case WM_USER+1:
		if(hGraph!=NULL)
		{
			DeleteObject(hGraph);
			DeleteDC(hMem);
		}
		hdc=GetDC(hWnd);
		hMem=CreateCompatibleDC(hdc);
		if(0==layer)
		{
			hGraph=CreateCompatibleBitmap(hdc, ti[index].cx, ti[index].cy);
		}
		else if(2==layer)
		{
			hGraph=CreateCompatibleBitmap(hdc, sii[index].cx, sii[index].cy);
		}
		else if(11==layer || 12==layer)
		{
			hGraph=CreateCompatibleBitmap(hdc, gii[index].cx, gii[index].cy);
		}
		SelectObject(hMem,hGraph);
		ReleaseDC(hWnd,hdc);
		if (0==layer && (0==ti[index].clrb || 2==ti[index].clrb))
		{
			BYTE *pByImageData=(BYTE*)malloc(ti[index].cx*ti[index].cy*4);
			if (NULL==pByImageData)
			{
				MessageBox(hWnd, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				break;
			}
			memset(pByImageData, 0, ti[index].cx*ti[index].cy*4);
			if(!DecodeDXTn(hMem, pByImageData, NULL))
			{
				free(pByImageData);
				pByImageData=NULL;
				break;
			}
			free(pByImageData);
			pByImageData=NULL;
		} 
		else
		{
			if(!DrawImage(hMem, (int)lParam, (int)wParam))
				break;
		}
		InvalidateRect(hWnd, NULL, true);

		//初始化滚动条
		GetClientRect (hWnd, &rc);
		cxClient = rc.right-rc.left;
		cyClient = rc.bottom-rc.top;

		if(0==layer)
		{
			yMaxScroll = ti?ti[index].cy:0;
			xMaxScroll = ti?ti[index].cx:0;
		}
		else if(2==layer)
		{
			yMaxScroll = sii?sii[index].cy:0;
			xMaxScroll = sii?sii[index].cx:0;
		}
		else if(11==layer || 12==layer)
		{
			yMaxScroll = gii?gii[index].cy:0;
			xMaxScroll = gii?gii[index].cx:0;
		}
		yCurrentScroll = min(yCurrentScroll, yMaxScroll);
		xCurrentScroll = min(xCurrentScroll, xMaxScroll);

		si.cbSize = sizeof(si);
		si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
		si.nMin   = yMinScroll;
		si.nMax   = yMaxScroll;
		si.nPage  = cyClient;
		si.nPos   = yCurrentScroll;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		// Retrieve the position. Due to adjustments by Windows it may not be the same as the value set
		GetScrollInfo(hWnd, SB_VERT, &si);
		yCurrentScroll=si.nPos;

		si.cbSize = sizeof(si);
		si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
		si.nMin   = xMinScroll;
		si.nMax   = xMaxScroll;
		si.nPage  = cxClient;
		si.nPos   = xCurrentScroll;
		SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
		// Retrieve the position. Due to adjustments by Windows it may not be the same as the value set
		GetScrollInfo(hWnd, SB_HORZ, &si);
		xCurrentScroll=si.nPos;
		break;
	case WM_SIZE:
		cxClient = LOWORD (lParam);
		cyClient = HIWORD (lParam);

		if(0==layer)
		{
			yMaxScroll = ti?ti[index].cy:0;
			xMaxScroll = ti?ti[index].cx:0;
		}
		else if(2==layer)
		{
			yMaxScroll = sii?sii[index].cy:0;
			xMaxScroll = sii?sii[index].cx:0;
		}
		yCurrentScroll = min(yCurrentScroll, yMaxScroll);
		xCurrentScroll = min(xCurrentScroll, xMaxScroll);

		si.cbSize = sizeof(si);
		si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
		si.nMin   = yMinScroll;
		si.nMax   = yMaxScroll;
		si.nPage  = cyClient;
		si.nPos   = yCurrentScroll;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		// Retrieve the position. Due to adjustments by Windows it may not be the same as the value set
		GetScrollInfo(hWnd, SB_VERT, &si);
		yCurrentScroll=si.nPos;

		si.cbSize = sizeof(si);
		si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
		si.nMin   = xMinScroll;
		si.nMax   = xMaxScroll;
		si.nPage  = cxClient;
		si.nPos   = xCurrentScroll;
		SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
		// Retrieve the position. Due to adjustments by Windows it may not be the same as the value set
		GetScrollInfo(hWnd, SB_HORZ, &si);
		xCurrentScroll=si.nPos;
		break;
	case WM_VSCROLL:
		int yPrePos;

		// Get all the vertical scroll bar information
		si.cbSize 	= sizeof (si);
		si.fMask  	= SIF_ALL;
		GetScrollInfo (hWnd, SB_VERT, &si);
		// Save the position for comparison later on
		yPrePos = si.nPos;

		switch (LOWORD(wParam)) 
		{ 
			case SB_PAGEUP: 
				si.nPos -= cyClient;
				break; 
			case SB_PAGEDOWN: 
				si.nPos += cyClient;
				break; 
			case SB_LINEUP: 
				si.nPos -= 10;
				break; 
			case SB_LINEDOWN: 
				si.nPos += 10;
				break; 
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;
			case SB_TOP:
				si.nPos = si.nMin;
				break;   
			case SB_BOTTOM:
       			si.nPos = si.nMax;
 	 			break;
			default: 
				si.nPos = yPrePos;
		} 
		// Set the position and then retrieve it. Due to adjustments by Windows it may not be the same as the value set.
		si.fMask = SIF_POS;
		SetScrollInfo (hWnd, SB_VERT, &si, TRUE);
		GetScrollInfo (hWnd, SB_VERT, &si);

		// If the position has changed, scroll the window and update it
		if (si.nPos != yPrePos)
		{                    
			yCurrentScroll=si.nPos;
			ScrollWindow(hWnd, 0, yPrePos - si.nPos, NULL, NULL);
			UpdateWindow(hWnd);
		}
		break;
	case WM_HSCROLL:
		int xPrePos;

		// Get all the horizontal scroll bar information
		si.cbSize 	= sizeof (si);
		si.fMask  	= SIF_ALL;
		GetScrollInfo (hWnd, SB_HORZ, &si);
		// Save the position for comparison later on
		xPrePos = si.nPos;

		switch (LOWORD(wParam)) 
		{ 
			case SB_PAGEUP: 
				si.nPos -= cxClient;
				break; 
			case SB_PAGEDOWN: 
				si.nPos += cxClient;
				break; 
			case SB_LINEUP: 
				si.nPos -= 10;
				break; 
			case SB_LINEDOWN: 
				si.nPos += 10;
				break; 
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;
			case SB_TOP:
				si.nPos = si.nMin;
				break;   
			case SB_BOTTOM:
       			si.nPos = si.nMax;
 	 			break;
			default: 
				si.nPos = xPrePos;
		} 
		// Set the position and then retrieve it. Due to adjustments by Windows it may not be the same as the value set.
		si.fMask = SIF_POS;
		SetScrollInfo (hWnd, SB_HORZ, &si, TRUE);
		GetScrollInfo (hWnd, SB_HORZ, &si);

		// If the position has changed, scroll the window and update it
		if (si.nPos != xPrePos)
		{                    
			xCurrentScroll=si.nPos;
			ScrollWindow(hWnd, xPrePos - si.nPos, 0, NULL, NULL);
			UpdateWindow(hWnd);
		}
		break;
	case WM_MOUSEWHEEL:
		sWheelDelta = (short) HIWORD(wParam);
		if (sWheelDelta>0)
		{
			for (i=0;i<sWheelDelta/40;i++)
				SendMessage (hWnd, WM_VSCROLL, (WPARAM)SB_LINEUP, 0);
		}
		else
		{
			for (i=0;i<-sWheelDelta/40;i++)
				SendMessage (hWnd, WM_VSCROLL, (WPARAM)SB_LINEDOWN, 0);
		}
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
			case VK_UP:
				SendMessage (hWnd, WM_VSCROLL, (WPARAM)SB_LINEUP, 0);
				break;
			case VK_DOWN:
				SendMessage (hWnd, WM_VSCROLL, (WPARAM)SB_LINEDOWN, 0);
				break;
			case VK_PRIOR:
				SendMessage (hWnd, WM_VSCROLL, (WPARAM)SB_PAGEUP, 0);
				break;
			case VK_NEXT:
				SendMessage (hWnd, WM_VSCROLL, (WPARAM)SB_PAGEDOWN, 0);
				break;
			case VK_HOME:
				SendMessage (hWnd, WM_VSCROLL, SB_TOP, 0);
				break;
			case VK_END:
				SendMessage (hWnd, WM_VSCROLL, SB_BOTTOM, 0);
				break;
			case VK_LEFT:
				SendMessage (hWnd, WM_HSCROLL, (WPARAM)SB_LINELEFT, 0);
				break;
			case VK_RIGHT:
				SendMessage (hWnd, WM_HSCROLL, (WPARAM)SB_LINERIGHT, 0);
				break;
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		SetFocus(hWnd);
		break;
	case WM_DESTROY:
		if(hGraph!=NULL)
		{
			DeleteObject(hGraph);
			DeleteDC(hMem);
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ExplorerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	int wmId, wmEvent, gm3DatNum, gm3ImageNum, mtxpHeadSize, mtxpSubFileNum, lvIndex;
	static HWND hWndListView,hWndButton;
	RECT rcl, rcButton, rc;
	INITCOMMONCONTROLSEX icex;
	char szText[256], szOutFileName[256], szOutFile[MAX_PATH], szRawOutFile[MAX_PATH], szRawOutFileName[256], szRawInFile[MAX_PATH], szRawInFileName[256];
	static char szPalNo[4];
	static int iPalNo, iPalNoSum=0;
	static DWORD Temp[3]; 
    LVCOLUMN lvc;
	LVITEM lvI;
	NMLVDISPINFO *plvdi;
	LPNMITEMACTIVATE lpnmitem;
	LPNMLVKEYDOWN lpnkd;
	NMITEMACTIVATE nmitem;
	LPNMLISTVIEW lpnmv;
	char szAsc[4]={'\x20', '\xa1', '\xc4', '\0'};
	char szDes[4]={'\x20', '\xa1', '\xc5', '\0'};
	POINT pt;
	HMENU hPopupMenu;
	BYTE *lpRawBuf;
	OPENFILENAME ofn;

	switch (message)
	{
	case WM_INITDIALOG:
		// Ensure that the common control DLL is loaded.
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC  = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&icex);

		GetClientRect (hDlg, &rcl);

		hWndButton = GetDlgItem(hDlg, IDCANCEL);
		GetWindowRect(hWndButton, &rcButton);
		SetWindowPos(hWndButton, NULL, rcl.right-(rcButton.right-rcButton.left)-5, rcl.bottom-(rcButton.bottom-rcButton.top)-5, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		
		hWndButton = GetDlgItem(hDlg, ID_EX_IMPORT);
		GetWindowRect(hWndButton, &rcButton);
		SetWindowPos(hWndButton, NULL, rcl.right-2*(rcButton.right-rcButton.left)-10, rcl.bottom-(rcButton.bottom-rcButton.top)-5, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		// Create the list-view window in report view
		hWndListView = CreateWindowEx(WS_EX_STATICEDGE | WS_EX_TRANSPARENT, WC_LISTVIEW, "Explorer View", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS, 
									5, 5, rcl.right-rcl.left-10, rcl.bottom-rcl.top-13-(rcButton.bottom-rcButton.top),
									hDlg, (HMENU) IDC_LISTVIEW, hInst, NULL);
		ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_BORDERSELECT | LVS_EX_FULLROWSELECT);

		ZeroMemory(&lvc, sizeof(LVCOLUMN));
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		for (i = 0; i < 3; i++)
		{ 
			lvc.iSubItem = i;
			lvc.pszText = szText;
			lvc.cx = 100;     // width of column in pixels
			lvc.fmt = LVCFMT_LEFT;  // left-aligned column
			LoadString(hInst, IDS_COLUMN1 + i, szText, sizeof(szText)/sizeof(szText[0]));
			if (ListView_InsertColumn(hWndListView, i, &lvc) == -1) 
				break;
		}
		for(i=0;i<iGPalNum;i++)
		{
			sprintf_s(szPalNo, 4, "%03d", i);
			SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
		}
		SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETMINVISIBLE, (WPARAM)16, 0);
		SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);

		ZeroMemory(&sd, sizeof(SORTDATA));
		SetFocus(hWndListView);
		return (INT_PTR)TRUE;
	case WM_ACTIVATE:
		if(fpIn!=NULL && (ListView_GetItemCount(hWndListView)==0 || true==layerChange))
		{
			if(bSorted)
			{
				ZeroMemory(&lvc, sizeof(LVCOLUMNA));
				lvc.mask=LVCF_TEXT | LVCF_SUBITEM;
				for(i=0;i<3;i++)
				{
					LoadString(hInst, IDS_COLUMN1+i, szText, sizeof(szText)/sizeof(szText[0]));
					lvc.iSubItem=i;
					lvc.pszText=szText;
					lvc.cchTextMax=256;
					SendMessage(hWndListView, LVM_SETCOLUMN, (WPARAM)i, (LPARAM)&lvc);
				}
				bSorted=false;
			}

			if(0==layer)
			{
				ListView_DeleteAllItems(hWndListView);
				lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
				lvI.state = 0;
				lvI.stateMask = 0;
				for (i = 0; i < fileNum; i++)
				{
					lvI.iItem = i;
					lvI.iSubItem = 0;
					lvI.lParam = (LPARAM)i;// &fi[i];
					lvI.pszText = LPSTR_TEXTCALLBACK; // sends an LVN_GETDISP message.
					if(ListView_InsertItem(hWndListView, &lvI) == -1)
						break;
				}
				rc.left=LVIR_BOUNDS;
				SendMessage(hWndListView, LVM_GETITEMRECT, (WPARAM) (int)0, (LPARAM) (LPRECT)&rc);
				SendMessage(hWndListView, LVM_SCROLL, (WPARAM) (int)0, (LPARAM) (int)(preIndex0-index)*(rc.bottom-rc.top));
				lvI.state=lvI.stateMask=LVIS_FOCUSED | LVIS_SELECTED;
				SendMessage(hWndListView, LVM_SETITEMSTATE, (WPARAM) (int)preIndex0, (LPARAM) (LPLVITEM)&lvI);
			}
			else if(1==layer || 11==layer || 12==layer)
			{
				//设置dat或者gm3或者MTXP文件层次信息
				ListView_DeleteAllItems(hWndListView);
				lvI.mask = LVIF_TEXT| LVIF_PARAM | LVIF_STATE;
				lvI.state = 0;
				lvI.stateMask = 0;
				for (i = 0; i < datFileNum+1; i++)
				{
					lvI.iItem = i;
					lvI.iSubItem = 0;
					lvI.lParam = (LPARAM)i;// &di[i];
					lvI.pszText = LPSTR_TEXTCALLBACK; // sends an LVN_GETDISP message.
					if(ListView_InsertItem(hWndListView, &lvI) == -1)
						break;
				}
				if(1==layer)
				{
					rc.left=LVIR_BOUNDS;
					SendMessage(hWndListView, LVM_GETITEMRECT, (WPARAM) (int)0, (LPARAM) (LPRECT)&rc);
					SendMessage(hWndListView, LVM_SCROLL, (WPARAM) (int)0, (LPARAM) (int)(preIndex1-index)*(rc.bottom-rc.top));
					lvI.state=lvI.stateMask=LVIS_FOCUSED | LVIS_SELECTED;
					SendMessage(hWndListView, LVM_SETITEMSTATE, (WPARAM) (int)preIndex1, (LPARAM) (LPLVITEM)&lvI);
				}
			}
			else if(2==layer)
			{
				//设置子sub image文件层次信息
				ListView_DeleteAllItems(hWndListView);
				lvI.mask = LVIF_TEXT| LVIF_PARAM | LVIF_STATE;
				lvI.state = 0;
				lvI.stateMask = 0;
				for (i = 0; i < siImageNum+1; i++)
				{
					lvI.iItem = i;
					lvI.iSubItem = 0;
					lvI.lParam = (LPARAM)i;// &sifi[i];
					lvI.pszText = LPSTR_TEXTCALLBACK; // sends an LVN_GETDISP message.
					if(ListView_InsertItem(hWndListView, &lvI) == -1)
						break;
				}
			}
		}
		return (INT_PTR)TRUE;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
			case LVN_GETDISPINFO:
				plvdi=(NMLVDISPINFO*)lParam;
				if(0==layer)
				{
					switch (plvdi->item.iSubItem)
					{
					case 0:
						plvdi->item.pszText = fi[plvdi->item.lParam].szName;
						break;  
					case 1:
						plvdi->item.pszText = fi[plvdi->item.lParam].szOffset;
						break;
					case 2:
						plvdi->item.pszText = fi[plvdi->item.lParam].szType;
						break;
					}
				}
				else if(1==layer || 11==layer || 12==layer)
				{
					switch (plvdi->item.iSubItem)
					{
					case 0:
						plvdi->item.pszText = di[plvdi->item.lParam].szName;
						break;  
					case 1:
						plvdi->item.pszText = di[plvdi->item.lParam].szOffset;
						break;
					case 2:
						plvdi->item.pszText = di[plvdi->item.lParam].szType;
						break;
					}
				}
				else if(2==layer)
				{
					switch (plvdi->item.iSubItem)
					{
					case 0:
						plvdi->item.pszText = sifi[plvdi->item.lParam].szName;
						break;  
					case 1:
						plvdi->item.pszText = sifi[plvdi->item.lParam].szOffset;
						break;
					case 2:
						plvdi->item.pszText = sifi[plvdi->item.lParam].szType;
						break;
					}
				}
				break;
			case NM_DBLCLK:
				lpnmitem = (LPNMITEMACTIVATE) lParam;
				lvIndex=lpnmitem->iItem;
				if(lvIndex==-1)
					break;
				ZeroMemory(&lvI, sizeof(LVITEM));
				lvI.mask=LVIF_PARAM;
				lvI.iItem=lvIndex;
				if(SendMessage(hWndListView, LVM_GETITEM, 0, (LPARAM) (LPLVITEM)&lvI))
					index=lvI.lParam;
				else
					break;
				if(NULL!=pBufIm)
				{
					free(pBufIm);
					pBufIm=NULL;
					free(pal);
					pal=NULL;
				}
				EnableWindow(GetDlgItem(hDlg, IDC_COMBO1), TRUE);
				if(0==layer)
				{
					if(!strcmp("TXP", fi[index].szType) || !strcmp("TX2", fi[index].szType))
					{
						fseek(fpIn, fi[index].Offset, 0);
						fread(&ti[index].cx, 2, 1, fpIn);
						fread(&ti[index].cy, 2, 1, fpIn);
						fseek(fpIn, 4L, 1);
						fread(&ti[index].clrb, 2, 1, fpIn);
						fread(&ti[index].palNum, 2, 1, fpIn);
						fread(&ti[index].flag, 2, 1, fpIn);
						fseek(fpIn, 2L, 1);//需要fseek过整个文件头
						if (0==ti[index].palNum && 0==ti[index].flag)
						{//hg2 TXP
							bHg2=true;
							bPrinny=false;
							fseek(fpIn, fi[index].Offset, 0);
							//fseek(fpIn, 2L, 1);
							fread(&ti[index].cx, 2, 1, fpIn);
							fread(&ti[index].cy, 2, 1, fpIn);
							fseek(fpIn, 2L, 1);
							fread(&ti[index].clrb, 2, 1, fpIn);
							fseek(fpIn, 6L, 1);
							//fread(&ti[index].cx, 2, 1, fpIn);
							fseek(fpIn, 2L, 1);
							EnableWindow(GetDlgItem(hDlg, IDC_COMBO1), FALSE);
							if(0==ti[index].clrb)
								strcpy_s(szBpp, 5, "DXT1");
							else if(2==ti[index].clrb)
								strcpy_s(szBpp, 5, "DXT5");
						} 
						else
						{//prinny TXP
							bHg2=false;
							bPrinny=true;
							if(1==ti[index].palNum)
								SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
							GetDlgItemText(hDlg, IDC_COMBO1, szPalNo, 4);
							iPalNo=atoi(szPalNo);
							if(0x10==ti[index].clrb)
								strcpy_s(szBpp, 5, "4bpp");
							else if(0x100==ti[index].clrb)
								strcpy_s(szBpp, 5, "8bpp");
						}
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)szBpp);
						SendMessage(hWndChild, WM_USER+1, (WPARAM)index, (LPARAM)iPalNo);
						bEx_Import=true;
					}
					else if (!strcmp("FTX", fi[index].szType))
					{
						//借用ti结构描述ftx信息
						fseek(fpIn, fi[index].Offset, 0);
						fread(&ti[index].cx, 4, 1, fpIn);
						fread(&ti[index].cy, 4, 1, fpIn);
						fseek(fpIn, 2L, 1);
						fread(&ti[index].clrb, 2, 1, fpIn);
						fread(&ti[index].palNum, 2, 1, fpIn);
						ti[index].flag=0x00;
						fseek(fpIn, 2L, 1);//需要fseek过整个文件头
						if(1==ti[index].palNum)
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						GetDlgItemText(hDlg, IDC_COMBO1, szPalNo, 4);
						iPalNo=atoi(szPalNo);
						if(0x10==ti[index].clrb)
							strcpy_s(szBpp, 5, "4bpp");
						else if(0x100==ti[index].clrb)
							strcpy_s(szBpp, 5, "8bpp");
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)szBpp);
						SendMessage(hWndChild, WM_USER+1, (WPARAM)index, (LPARAM)iPalNo);
						bEx_Import=true;
					}
					else if (!strcmp("TRP1", fi[index].szType))
					{
						//借用ti结构描述TRP1信息
						fseek(fpIn, fi[index].Offset, 0);
						bTRP1=true;
						fseek(fpIn, 16L, 1);
						fread(&dwTRP1Offset, 4, 1, fpIn);
						fseek(fpIn, fi[index].Offset+dwTRP1Offset, 0);
						fseek(fpIn, 2L, 1);
						fread(&ti[index].cy, 2, 1, fpIn);
						fseek(fpIn, 2L, 1);
						fread(&ti[index].clrb, 2, 1, fpIn);
						fseek(fpIn, 2L, 1);
						fread(&ti[index].palNum, 2, 1, fpIn);
						ti[index].flag=fgetc(fpIn);
						fseek(fpIn, 1L, 1);
						fread(&ti[index].cx, 2, 1, fpIn);
						if(1==ti[index].palNum)
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						GetDlgItemText(hDlg, IDC_COMBO1, szPalNo, 4);
						iPalNo=atoi(szPalNo);
						if(0x10==ti[index].clrb)
							strcpy_s(szBpp, 5, "4bpp");
						else if(0x100==ti[index].clrb)
							strcpy_s(szBpp, 5, "8bpp");
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)szBpp);
						SendMessage(hWndChild, WM_USER+1, (WPARAM)index, (LPARAM)iPalNo);
						bEx_Import=true;
					}
					else if (!strcmp("TXP1", fi[index].szType))
					{
						//借用ti结构描述TXP1信息
						fseek(fpIn, fi[index].Offset, 0);
						fseek(fpIn, 2L, 1);
						fread(&ti[index].cy, 2, 1, fpIn);
						fseek(fpIn, 2L, 1);
						fread(&ti[index].clrb, 2, 1, fpIn);
						fseek(fpIn, 2L, 1);
						fread(&ti[index].palNum, 2, 1, fpIn);
						ti[index].flag=fgetc(fpIn);
						fseek(fpIn, 1L, 1);
						fread(&ti[index].cx, 2, 1, fpIn);
						if(1==ti[index].palNum)
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						GetDlgItemText(hDlg, IDC_COMBO1, szPalNo, 4);
						iPalNo=atoi(szPalNo);
						if(0x10==ti[index].clrb)
							strcpy_s(szBpp, 5, "4bpp");
						else if(0x100==ti[index].clrb)
							strcpy_s(szBpp, 5, "8bpp");
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)szBpp);
						SendMessage(hWndChild, WM_USER+1, (WPARAM)index, (LPARAM)iPalNo);
						bEx_Import=true;
					}
					else if(!strcmp("DAT", fi[index].szType))
					{
						fseek(fpIn, fi[index].Offset, 0);
						fread(&datFileNum, 4, 1, fpIn);
						fread(&Temp[0], 4, 1, fpIn);
						fread(&Temp[1], 4, 1, fpIn);
						fread(&Temp[2], 4, 1, fpIn);
						if(0!=Temp[0] || 0!=Temp[1] || 0!=Temp[2])
						{
							MessageBox(hDlg, "It isn't supported file type!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
							break;
						}

						di=(DATINFO*)malloc((datFileNum+1)*sizeof(DATINFO));
						if (NULL==di)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						ZeroMemory(di, (datFileNum+1)*sizeof(DATINFO));
						si=(SUBINFO*)malloc((datFileNum+1)*sizeof(SUBINFO));
						if (NULL==si)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						ZeroMemory(si, (datFileNum+1)*sizeof(SUBINFO));
						//设置第一项
						strcpy_s(di[0].szParentName, 28, fi[index].szName);
						strcpy_s(di[0].szName, 3, "..");
						strcpy_s(di[0].szOffset, 11, "Return");
						strcpy_s(di[0].szType, 8, "DIR");
						//设置其余项
						for(i=1;i<=datFileNum;i++)
						{
							strcpy_s(di[i].szParentName, 28, fi[index].szName);
							fread(&di[i].Size, 4, 1, fpIn);
							sprintf_s(di[i].szName, 3, "%02X", i);
							di[i].Offset=ftell(fpIn)-4;
							sprintf_s(di[i].szOffset, 11, "0x%08X", di[i].Offset);
							fseek(fpIn, di[i].Size-4, 1);
							strcpy_s(di[i].szType, 8, "RAW");
						}
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)NULL);
						layer=1;
						preIndex0=index;
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
					else if(!strcmp("GM3", fi[index].szType))
					{
						fseek(fpIn, fi[index].Offset, 0);
						fread(&gm3DatNum, 4, 1, fpIn);
						fread(&gm3ImageNum, 4, 1, fpIn);
						datFileNum=gm3ImageNum;
						fseek(fpIn, gm3DatNum*4*2, 1);
						fread(&Temp[0], 4, 1, fpIn);//获取图像数据开始地址
						fseek(fpIn, fi[index].Offset+Temp[0], 0);//定位到图像数据处

						//借用di结构描述gm3文件信息
						di=(DATINFO*)malloc((gm3ImageNum+1)*sizeof(DATINFO));
						if (NULL==di)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						ZeroMemory(di, (gm3ImageNum+1)*sizeof(DATINFO));
						gii=(GM3IMAGEINFO*)malloc((gm3ImageNum+1)*sizeof(GM3IMAGEINFO));
						if (NULL==gii)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						ZeroMemory(gii, (gm3ImageNum+1)*sizeof(GM3IMAGEINFO));
						//设置第一项
						strcpy_s(di[0].szParentName, 28, fi[index].szName);
						strcpy_s(di[0].szName, 3, "..");
						strcpy_s(di[0].szOffset, 11, "Return");
						strcpy_s(di[0].szType, 8, "DIR");
						//设置其余项
						for(i=1;i<=gm3ImageNum;i++)
						{
							strcpy_s(di[i].szParentName, 28, fi[index].szName);
							fread(&di[i].Size, 4, 1, fpIn);
							sprintf_s(di[i].szName, 3, "%02X", i);
							di[i].Offset=ftell(fpIn)-4;
							sprintf_s(di[i].szOffset, 11, "0x%08X", di[i].Offset);
							fseek(fpIn, di[i].Size-4, 1);
							strcpy_s(di[i].szType, 8, "RAW");
						}
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)NULL);
						layer=11;
						preIndex0=index;
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
					else if(!strcmp("MTXP", fi[index].szType))
					{
						fseek(fpIn, fi[index].Offset, 0);
						fread(&mtxpHeadSize, 4, 1, fpIn);
						fread(&mtxpSubFileNum, 4, 1, fpIn);
						datFileNum=mtxpSubFileNum;

						//借用di结构描述MTXP文件信息
						di=(DATINFO*)malloc((mtxpSubFileNum+1)*sizeof(DATINFO));
						if (NULL==di)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						memset(di, 0, (mtxpSubFileNum+1)*sizeof(DATINFO));
						//借用gii结构描述MTXP图像信息
						gii=(GM3IMAGEINFO*)malloc((mtxpSubFileNum+1)*sizeof(GM3IMAGEINFO));
						if (NULL==gii)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						memset(gii, 0, (mtxpSubFileNum+1)*sizeof(GM3IMAGEINFO));
						//设置第一项
						strcpy_s(di[0].szParentName, 28, fi[index].szName);
						strcpy_s(di[0].szName, 3, "..");
						strcpy_s(di[0].szOffset, 11, "Return");
						strcpy_s(di[0].szType, 8, "DIR");
						//设置其余项
						for(i=1;i<=mtxpSubFileNum;i++)
						{
							strcpy_s(di[i].szParentName, 28, fi[index].szName);
							sprintf_s(di[i].szName, 3, "%02X", i);
							fseek(fpIn, 4L, 1);
							fread(&di[i].Offset, 4, 1, fpIn);
							di[i].Offset+=fi[index].Offset;
							sprintf_s(di[i].szOffset, 11, "0x%08X", di[i].Offset);
							strcpy_s(di[i].szType, 8, "RAW");
						}
						for(i=1;i<mtxpSubFileNum;i++)
						{
							di[i].Size=di[i+1].Offset-di[i].Offset;
						}
						if (bOpenHg2Data)
							di[mtxpSubFileNum].Size=fi[index].Size-(di[mtxpSubFileNum].Offset-fi[index].Offset);
						else
						{
							fseek(fpIn, 0L, 2);
							di[mtxpSubFileNum].Size=ftell(fpIn)-di[mtxpSubFileNum].Offset;
						}

						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)NULL);
						layer=12;
						preIndex0=index;
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
					else
					{
						MessageBox(hDlg, "It isn't supported file type!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
						break;
					}
				}
				else if (12==layer)
				{
					if(0==index)
					{
						layer=0;
						if(di)
						{
							free(di);
							di=NULL;
							free(gii);
							gii=NULL;
						}
						iGPalNum=16;
						SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
						for(i=0;i<iGPalNum;i++)
						{
							sprintf_s(szPalNo, 4, "%03d", i);
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
						}
						SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
					else
					{
						fseek(fpIn, di[index].Offset, 0);
						gii[index].fileSize=di[index].Size;
						fseek(fpIn, 6L, 1);
						//gii[index].cy=fgetc(fpIn);
						//gii[index].cy=1<<gii[index].cy;
						//fread(&gii[index].cy, 2, 1, fpIn);
						//fseek(fpIn, 2L, 1);
						fread(&gii[index].clrb, 2, 1, fpIn);
						fseek(fpIn, 2L,1);
						fread(&gii[index].palNum, 2, 1, fpIn);
						gii[index].flag=fgetc(fpIn);
						fseek(fpIn, 1L, 1);
						fread(&gii[index].cx, 2, 1, fpIn);
						gii[index].cy=(gii[index].fileSize-0x10-gii[index].palNum*gii[index].clrb*4)/gii[index].cx;
						iGPalNum=gii[index].palNum;
						if(SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_GETCOUNT, 0, 0)!=gii[index].palNum)
						{
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
							for(i=0;i<gii[index].palNum;i++)
							{
								sprintf_s(szPalNo, 4, "%03d", i);
								SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
							}
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						}
						GetDlgItemText(hDlg, IDC_COMBO1, szPalNo, 4);
						iPalNo=atoi(szPalNo);
						if(0x10==gii[index].clrb)
							strcpy_s(szBpp, 5, "4bpp");
						else if(0x100==gii[index].clrb)
							strcpy_s(szBpp, 5, "8bpp");
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)szBpp);
						SendMessage(hWndChild, WM_USER+1, (WPARAM)gii[index].palNum, (LPARAM)iPalNo);
						bEx_Import=true;
					}
				}
				else if(11==layer)
				{
					if(0==index)
					{
						layer=0;
						if(di)
						{
							free(di);
							di=NULL;
							free(gii);
							gii=NULL;
						}
						iGPalNum=16;
						SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
						for(i=0;i<iGPalNum;i++)
						{
							sprintf_s(szPalNo, 4, "%03d", i);
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
						}
						SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
					else
					{
						fseek(fpIn, di[index].Offset, 0);
						fread(&gii[index].fileSize, 4, 1, fpIn);
						fread(&gii[index].cx, 2, 1, fpIn);
						fread(&gii[index].cy, 2, 1, fpIn);
						fread(&gii[index].clrb, 2, 1, fpIn);
						gii[index].flag=fgetc(fpIn);
						if(0x10==gii[index].clrb)
						{
							if(gii[index].flag<0x05)
								gii[index].flag=0x00;
							else
								gii[index].flag=0x01;
						}
						else if(0x100==gii[index].clrb)
						{
							if(gii[index].flag<0x04)
								gii[index].flag=0x00;
							else
								gii[index].flag=0x01;
						}
						fseek(fpIn, 1L, 1);
						fread(&gii[index].palNum, 4, 1, fpIn);
						iGPalNum=gii[index].palNum;
						if(SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_GETCOUNT, 0, 0)!=gii[index].palNum)
						{
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
							for(i=0;i<gii[index].palNum;i++)
							{
								sprintf_s(szPalNo, 4, "%03d", i);
								SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
							}
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						}
						GetDlgItemText(hDlg, IDC_COMBO1, szPalNo, 4);
						iPalNo=atoi(szPalNo);
						if(0x10==gii[index].clrb)
							strcpy_s(szBpp, 5, "4bpp");
						else if(0x100==gii[index].clrb)
							strcpy_s(szBpp, 5, "8bpp");
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)fi[index].szName, (LPARAM)szBpp);
						SendMessage(hWndChild, WM_USER+1, (WPARAM)gii[index].palNum, (LPARAM)iPalNo);
						bEx_Import=true;
					}
				}
				else if(1==layer)
				{
					if(0==index)
					{
						layer=0;
						if(di)
						{
							free(di);
							di=NULL;
							free(si);
							si=NULL;
						}
						iGPalNum=16;
						SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
						for(i=0;i<iGPalNum;i++)
						{
							sprintf_s(szPalNo, 4, "%03d", i);
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
						}
						SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
					else
					{
						fseek(fpIn, di[index].Offset, 0);
						fread(&si[index].fileSize, 4, 1, fpIn);
						fseek(fpIn, 8L, 1);
						fread(&si[index].palNum, 2, 1, fpIn);
						fread(&si[index].imageNum, 2, 1, fpIn);
						siImageNum=si[index].imageNum;
						fseek(fpIn, 0x14L, 1);
						fread(&si[index].PalInfoOffset, 4, 1, fpIn);
						si[index].PalInfoOffset+=di[index].Offset;
						fread(&si[index].ImageInfoOffset, 4, 1, fpIn);
						si[index].ImageInfoOffset+=di[index].Offset;
						fread(&Temp[2], 4, 1, fpIn);
						Temp[2]=(Temp[2]-(si[index].ImageInfoOffset-di[index].Offset));
						Temp[2]=Temp[2]/(4*si[index].imageNum);
						if(4==Temp[2])
						{
							bHg2=true;
							bPrinny=false;
						}
						
						spi=(SUBPALINFO*)malloc((si[index].palNum)*sizeof(SUBPALINFO));
						if (NULL==spi)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						ZeroMemory(spi, (si[index].palNum)*sizeof(SUBPALINFO));
						sii=(SUBIMAGEINFO*)malloc((si[index].imageNum+1)*sizeof(SUBIMAGEINFO));
						if (NULL==sii)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						ZeroMemory(sii, (si[index].imageNum+1)*sizeof(SUBIMAGEINFO));

						fseek(fpIn, si[index].PalInfoOffset, 0);
						for(i=0;i<si[index].palNum;i++)
						{
							fread(&spi[i].PalOffset, 4, 1, fpIn);
							spi[i].PalOffset+=di[index].Offset;
							fread(&spi[i].PalSize, 4, 1, fpIn);
							spi[i].perPalNum=spi[i].PalSize/0x10;
						}

						//判断是16色还是256色调色板并设置16色调色板总数
						iPalNoSum=0;
						for(i=0;i<si[index].palNum;i++)
						{
							if(0x100==spi[i].PalSize)
							{
								//判断开头两个16色调色板第一个颜色是否相等
								fseek(fpIn, spi[i].PalOffset, 0);
								fread(&Temp[0], 4, 1, fpIn);
								fseek(fpIn, 0x3CL, 1);
								fread(&Temp[1], 4, 1, fpIn);
								if(Temp[0]==Temp[1])
								{
									//相等即是16色调色板
									spi[i].bpp=0x04;
									iPalNoSum+=spi[i].perPalNum;
								}
								else
								{
									//判断结尾两个16色调色板第一个颜色是否相等
									fseek(fpIn, 0x33CL, 1);
									fread(&Temp[0], 4, 1, fpIn);
									fseek(fpIn, 0x3CL, 1);
									fread(&Temp[1], 4, 1, fpIn);
									if(Temp[0]==Temp[1])
									{
										//相等即是16色调色板
										spi[i].bpp=0x04;
										iPalNoSum+=spi[i].perPalNum;
									}
									else
										spi[i].bpp=0x08;
								}
							}
							else
							{
								spi[i].bpp=0x04;
								iPalNoSum+=spi[i].perPalNum;
							}
						}

						fseek(fpIn, si[index].ImageInfoOffset, 0);
						if (bHg2)
						{//hg2 dat文件图像信息数据
							for(i=1;i<si[index].imageNum+1;i++)
							{
								fread(&sii[i].ImageOffset, 4, 1, fpIn);
								sii[i].ImageOffset+=di[index].Offset;
								fseek(fpIn, 2L, 1);
								fread(&sii[i].cy, 2, 1, fpIn);
								fseek(fpIn, 2L, 1);
								fread(&sii[i].cx, 2, 1, fpIn);
								sii[i].bpp=fgetc(fpIn);
								fseek(fpIn, 2L, 1);
								sii[i].flag=fgetc(fpIn);
							}
						}
						else if(bPrinny)
						{//prinny dat文件图像信息数据
							for(i=1;i<si[index].imageNum+1;i++)
							{
								fread(&sii[i].ImageOffset, 4, 1, fpIn);
								sii[i].ImageOffset+=di[index].Offset;
								fread(&sii[i].cx, 2, 1, fpIn);
								fread(&sii[i].cy, 2, 1, fpIn);
								sii[i].bpp=fgetc(fpIn);
								fseek(fpIn, 2L, 1);
								sii[i].flag=fgetc(fpIn);
							}
						}
						
						if (bHg2)
						{//hg2 dat文件调色板信息数据
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
							for(i=0;i<si[index].palNum;i++)
							{
								sprintf_s(szPalNo, 4, "%03d", i);
								SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
							}
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
							iGPalNum=si[index].palNum;
						}
						else if(bPrinny)
						{//prinny dat文件调色板信息数据
							bHg2=false;
							bPrinny=true;
							//设置256色图像用哪个256色调色板
							i=1;
							for(j=0;j<si[index].palNum;j++)
							{
								if(0x08==spi[j].bpp)
								{
									while(i<=si[index].imageNum)
									{
										if(0x08==sii[i].bpp)
										{
											sii[i].palIndex=j;
											i++;
											break;
										}
										i++;
									}
								}
							}

							//把第一个16色调色板的索引值赋给16色图像结构中的palIndex字段
							for(j=0;j<si[index].palNum;j++)
							{
								if(0x04==spi[j].bpp)
								{
									for (i=1;i<=si[index].imageNum;i++)
									{
										if(0x04==sii[i].bpp)
											sii[i].palIndex=j;
									}
									break;
								}
							}

							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);
							for(i=0;i<iPalNoSum;i++)
							{
								sprintf_s(szPalNo, 4, "%03d", i);
								SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
							}
							if(0==iPalNoSum)
							{
								sprintf_s(szPalNo, 4, "%03d", iPalNoSum);
								SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)szPalNo);
							}
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
							iGPalNum=iPalNoSum;
						}

						sifi=(SUBIMAGEFILEINFO*)malloc((si[index].imageNum+1)*sizeof(SUBIMAGEFILEINFO));
						if (NULL==sifi)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							break;
						}
						ZeroMemory(sifi, (si[index].imageNum+1)*sizeof(SUBIMAGEFILEINFO));
						//设置第一项
						strcpy_s(sifi[0].szName, 6, "..");
						strcpy_s(sifi[0].szOffset, 11, "Return");
						strcpy_s(sifi[0].szType, 8, "DIR");
						//设置其余项
						for(i=1;i<=si[index].imageNum;i++)
						{
							strcpy_s(sifi[i].szParentName, 28, di[index].szParentName);
							sprintf_s(sifi[i].szName, 6, "%s_%02X", di[index].szName, i);
							sifi[i].Offset=sii[i].ImageOffset;
							sprintf_s(sifi[i].szOffset, 11, "0x%08X", sifi[i].Offset);
							strcpy_s(sifi[i].szType, 8, "RAW");
						}
						//针对anime91.dat->01做特殊处理
						if(!strcmp(di[index].szParentName, "anime91.dat") && !strcmp(di[index].szName, "01"))
						{
							sii=(SUBIMAGEINFO*)realloc(sii, (si[index].imageNum+4)*sizeof(SUBIMAGEINFO));
							if (NULL==sii)
							{
								MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
								break;
							}
							si[index].imageNum+=3;
							siImageNum+=3;
							memcpy(&sii[5], &sii[2], sizeof(SUBIMAGEINFO));
							//sii[2].cy=0x110;
							sii[5].ImageOffset+=sii[2].cx*0x110;
							sii[5].cy=0x70;
							sii[5].palIndex++;

							memcpy(&sii[6], &sii[5], sizeof(SUBIMAGEINFO));
							sii[6].ImageOffset+=sii[5].cx*sii[5].cy;
							sii[6].cy=0x80;
							sii[6].palIndex++;

							memcpy(&sii[7], &sii[6], sizeof(SUBIMAGEINFO));
							sii[7].palIndex++;

							sifi=(SUBIMAGEFILEINFO*)realloc(sifi, (si[index].imageNum+4)*sizeof(SUBIMAGEFILEINFO));
							if (NULL==sifi)
							{
								MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
								break;
							}
							memcpy(&sifi[5], &sifi[2], sizeof(SUBIMAGEFILEINFO));
							sprintf_s(sifi[5].szName, 6, "%s_21", di[index].szName);
							sifi[5].Offset=sii[5].ImageOffset;
							sprintf_s(sifi[5].szOffset, 11, "0x%08X", sifi[5].Offset);

							memcpy(&sifi[6], &sifi[5], sizeof(SUBIMAGEFILEINFO));
							sprintf_s(sifi[6].szName, 6, "%s_22", di[index].szName);
							sifi[6].Offset=sii[6].ImageOffset;
							sprintf_s(sifi[6].szOffset, 11, "0x%08X", sifi[6].Offset);

							memcpy(&sifi[7], &sifi[6], sizeof(SUBIMAGEFILEINFO));
							sprintf_s(sifi[7].szName, 6, "%s_23", di[index].szName);
							sifi[7].Offset=sii[7].ImageOffset;
							sprintf_s(sifi[7].szOffset, 11, "0x%08X", sifi[7].Offset);
						}
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)di[index].szName, (LPARAM)NULL);
						layer=2;
						preIndex1=index;
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
				}
				else if(2==layer)
				{
					if(0==index)
					{
						layer=1;
						if(sifi)
						{
							free(spi);
							spi=NULL;
							free(sii);
							sii=NULL;
							free(sifi);
							sifi=NULL;
						}
						layerChange=true;
						sd.bAsc=false;
						SendMessage(hDlg, WM_ACTIVATE, 0, 0);
						layerChange=false;
					}
					else
					{
						if(0x08==sii[index].bpp && bPrinny)//prinny256色图像
							SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0);
						GetDlgItemText(hDlg, IDC_COMBO1, szPalNo, 4);
						iPalNo=atoi(szPalNo);
						iPalNoSum=SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_GETCOUNT , 0, 0);
						sprintf_s(szBpp, 5, "%dbpp", sii[index].bpp);
						SendMessage(GetParent(hDlg), WM_USER+1, (WPARAM)sifi[index].szName, (LPARAM)szBpp);
						SendMessage(hWndChild, WM_USER+1, (WPARAM)iPalNoSum, (LPARAM)iPalNo);
						bEx_Import=true;
					}
				}
				break;
		case LVN_COLUMNCLICK:
			lpnmv = (LPNMLISTVIEW) lParam;

			bSorted=true;
			sd.col=lpnmv->iSubItem;
			sd.hWndListView=hWndListView;
			if(sd.preCol!=sd.col)//保证每次第一次点击同一个列头时都是升序排序
				sd.bAsc=false;
			sd.bAsc=!sd.bAsc;

			ZeroMemory(&lvc, sizeof(LVCOLUMNA));
			lvc.mask=LVCF_TEXT | LVCF_SUBITEM;
			for(i=0;i<3;i++)
			{
				LoadString(hInst, IDS_COLUMN1+i, szText, sizeof(szText)/sizeof(szText[0]));
				lvc.iSubItem=i;
				lvc.pszText=szText;
				lvc.cchTextMax=256;
				SendMessage(hWndListView, LVM_SETCOLUMN, (WPARAM)i, (LPARAM)&lvc);
			}
			lvc.iSubItem=sd.col;
			lvc.pszText=szText;
			lvc.cchTextMax=256;
			if(SendMessage(hWndListView, LVM_GETCOLUMN, (WPARAM)sd.col, (LPARAM)&lvc))
			{
				if(sd.bAsc)
					strcat_s(szText, 256, szAsc);
				else
					strcat_s(szText, 256, szDes);
				SendMessage(hWndListView, LVM_SETCOLUMN, (WPARAM)sd.col, (LPARAM)&lvc);
				sd.preCol=sd.col;
			}
			SendMessage(hWndListView, LVM_SORTITEMSEX, (WPARAM) (LPARAM)&sd, (LPARAM) (PFNLVCOMPARE)CompareFunc);
			break;
		case LVN_KEYDOWN:
			lpnkd = (LPNMLVKEYDOWN) lParam;
			if(VK_SPACE==lpnkd->wVKey)
			{
				nmitem.iItem=SendMessage(hWndListView, LVM_GETSELECTIONMARK, 0, 0);
				nmitem.hdr.code=NM_DBLCLK;
				SendMessage(hDlg, WM_NOTIFY, (WPARAM) (int)0, (LPARAM)&nmitem);
			}
			break;
		case NM_RCLICK:
			lpnmitem = (LPNMITEMACTIVATE) lParam;
			lvIndex=lpnmitem->iItem;
			if(lvIndex==-1)
				break;
			memset(&lvI, 0, sizeof(LVITEM));
			lvI.mask=LVIF_PARAM;
			lvI.iItem=lvIndex;
			if(SendMessage(hWndListView, LVM_GETITEM, 0, (LPARAM) (LPLVITEM)&lvI))
				index=lvI.lParam;
			else
				break;
			hPopupMenu = GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDC_POPUPMENU)),0);
			GetCursorPos(&pt);
			TrackPopupMenuEx(hPopupMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, pt.x, pt.y, hDlg, NULL);
			break;
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDOK:
			if(hWndListView==GetFocus())
			{
				nmitem.iItem=SendMessage(hWndListView, LVM_GETSELECTIONMARK, 0, 0);
				nmitem.hdr.code=NM_DBLCLK;
				SendMessage(hDlg, WM_NOTIFY, (WPARAM) (int)0, (LPARAM)&nmitem);
			}
			break;
		case IDM_POPUPEXPORT:
			if (0!=layer)
			{
				MessageBox(hDlg, "Only the file in first layer can direct export!", szTitle, MB_OK | MB_ICONERROR);
				break;
			}
			sprintf_s(szRawOutFile, MAX_PATH, "%s\\%s", szCurrentPath, fi[index].szName);
			//memset(szRawOutFile, 0, sizeof(szRawOutFile[MAX_PATH]));
			//memset(szRawOutFileName, 0, sizeof(szRawOutFileName[256]));
			strcpy_s(szRawOutFileName, 256, fi[index].szName);
			memset(&ofn, 0, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = szRawOutFile;
			ofn.nMaxFile = sizeof(szRawInFile);
			ofn.lpstrFilter = "All Files(*.*)\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = szRawOutFileName;
			ofn.nMaxFileTitle = sizeof(szRawOutFileName);
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrTitle = "Export Raw File(->)";
			ofn.lpstrDefExt = "bin";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
			if (GetSaveFileName(&ofn)!=TRUE)
				break;
			if (fopen_s(&fpRawEx, szRawOutFile, "wb")!=NULL)
			{
				MessageBox(hDlg, "Can't create export file!", szTitle, MB_OK | MB_ICONERROR);
				break;
			}
			lpRawBuf=(BYTE*)malloc(fi[index].Size*sizeof(BYTE));
			if (NULL==lpRawBuf)
			{
				MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpRawEx);
				break;
			}
			memset(lpRawBuf, 0, (fi[index].Size)*sizeof(BYTE));
			fseek(fpIn, fi[index].Offset, 0);
			fread(lpRawBuf, 1, fi[index].Size, fpIn);
			if(fi[index].Size!=fwrite(lpRawBuf, 1, fi[index].Size, fpRawEx))
			{
				MessageBox(hDlg, "Exporting file occur error!", szTitle, MB_OK | MB_ICONERROR);
				free(lpRawBuf);
				lpRawBuf=NULL;
				fclose(fpRawEx);
				break;
			}
			free(lpRawBuf);
			lpRawBuf=NULL;
			fclose(fpRawEx);
			MessageBox(hDlg, "Exporting file succeed!", szTitle, MB_OK | MB_ICONINFORMATION);
			break;
		case IDM_POPUPIMPORT:
			memset(szRawInFile, 0, sizeof(szRawInFile[MAX_PATH]));
			memset(szRawInFileName, 0, sizeof(szRawInFileName[256]));
			memset(&ofn, 0, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = szRawInFile;
			ofn.nMaxFile = sizeof(szRawInFile);
			ofn.lpstrFilter = "All Files(*.*)\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = szRawInFileName;
			ofn.nMaxFileTitle = sizeof(szRawInFileName);
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrTitle = "Import Raw File(<-)";
			//ofn.lpstrDefExt = "vlg";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			if (GetOpenFileName(&ofn)!=TRUE) 
				break;

			if (fopen_s(&fpRawIm, szRawInFile, "rb")!=NULL)
			{
				MessageBox(hDlg, "Can't open Import File!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpRawIm);
				break;
			}
			fseek(fpRawIm, 0L, 2);
			Temp[0]=ftell(fpRawIm);
			if (fi[index].Size!=Temp[0])
			{
				MessageBox(hDlg, "The file to import doesn't match the selected file!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpRawIm);
				break;
			}
			lpRawBuf=(BYTE*)malloc(fi[index].Size*sizeof(BYTE));
			if (NULL==lpRawBuf)
			{
				MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpRawIm);
				break;
			}
			memset(lpRawBuf, 0, (fi[index].Size)*sizeof(BYTE));
			rewind(fpRawIm);
			fread(lpRawBuf, 1, fi[index].Size, fpRawIm);
			fseek(fpIn, fi[index].Offset, 0);
			if(fi[index].Size!=fwrite(lpRawBuf, 1, fi[index].Size, fpIn))
			{
				MessageBox(hDlg, "Importing file occur error!", szTitle, MB_OK | MB_ICONERROR);
				free(lpRawBuf);
				lpRawBuf=NULL;
				fclose(fpIn);
				fclose(fpRawIm);
				break;
			}
			free(lpRawBuf);
			lpRawBuf=NULL;
			fclose(fpIn);
			fclose(fpRawIm);
			if (fopen_s(&fpIn, szCurrentFile,"rb+")!=NULL)
			{
				MessageBox(hDlg, "Reopen file failed!", szTitle, MB_OK | MB_ICONERROR);
				break;
			}
			MessageBox(hDlg, "Importing file succeed!", szTitle, MB_OK | MB_ICONINFORMATION);
			break;
		case ID_EX_IMPORT:
			if(!bEx_Import)
			{
				MessageBox(hDlg, "Please preview the file first!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
				break;
			}
			iImPalNo=DialogBox(hInst, MAKEINTRESOURCE(IDD_EXIM), hDlg, Ex_ImportDlgProc);
			if(bExCurFile)
			{
				if(0==layer)
					sprintf_s(szOutFileName, 256, "%s@%s.tm2", fi[index].szName, szPalNo);
				else if(11==layer || 12==layer)
					sprintf_s(szOutFileName, 256, "%s-%s@%s.tm2", di[index].szParentName, di[index].szName, szPalNo);
				else if(2==layer)
					sprintf_s(szOutFileName, 256, "%s-%s@%s.tm2", sifi[index].szParentName, sifi[index].szName, szPalNo);
				
				sprintf_s(szOutFile, MAX_PATH, "%s\\%s", szCurrentPath, szOutFileName);
				if (fopen_s(&fpOut,szOutFile,"wb")!=NULL)
				{
					MessageBox(hDlg,"Can't create file!",szTitle,MB_OK | MB_ICONERROR | MB_TOPMOST);
					break;
				}
				if(0==layer)
				{
					if(!TXP2TIM2(iPalNo, szOutFileName))
					{
						MessageBox(hDlg, "Fail to export TIM2 file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
						fclose(fpOut);
						break;
					}
				}
				else if(11==layer || 12==layer)
				{
					if(!GM32TIM2(iPalNo))
					{
						MessageBox(hDlg, "Fail to export TIM2 file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
						fclose(fpOut);
						break;
					}
				}
				else if(2==layer)
				{
					if(!DAT2TIM2(iPalNo))
					{
						MessageBox(hDlg, "Fail to export TIM2 file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
						fclose(fpOut);
						break;
					}
				}
				fclose(fpOut);
				MessageBox(hDlg, "Export TIM2 file success!", szTitle, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
				bExCurFile=false;
			}
			if(bExAllFile)
			{
				bExAllFile=false;
			}
			if(bImFile)
			{
				if(0==layer)
				{
					if(0x10==ti[index].clrb || 0==ti[index].clrb)
					{
						pBufIm=(BYTE*)malloc(ti[index].cx*ti[index].cy/2);
						if (NULL==pBufIm)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bEx_Import=false;
							break;
						}
						ZeroMemory(pBufIm, ti[index].cx*ti[index].cy/2);
					}
					else if(0x100==ti[index].clrb || 2==ti[index].clrb)
					{
						pBufIm=(BYTE*)malloc(ti[index].cx*ti[index].cy);
						if (NULL==pBufIm)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bEx_Import=false;
							break;
						}
						ZeroMemory(pBufIm, ti[index].cx*ti[index].cy);
					}
					if (0x10==ti[index].clrb || 0x100==ti[index].clrb)
					{
						pal=(PAL*)malloc(ti[index].clrb*4);
						if (NULL==pBufIm)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bEx_Import=false;
							break;
						}
						ZeroMemory(pal, ti[index].clrb*4);
					}
				}
				else if(11==layer || 12==layer)
				{
					if(0x10==gii[index].clrb)
					{
						pBufIm=(BYTE*)malloc(gii[index].cx*gii[index].cy/2);
						if (NULL==pBufIm)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bEx_Import=false;
							break;
						}
						ZeroMemory(pBufIm, gii[index].cx*gii[index].cy/2);
					}
					else if(0x100==gii[index].clrb)
					{
						pBufIm=(BYTE*)malloc(gii[index].cx*gii[index].cy);
						if (NULL==pBufIm)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bEx_Import=false;
							break;
						}
						ZeroMemory(pBufIm, gii[index].cx*gii[index].cy);
					}
					pal=(PAL*)malloc(gii[index].clrb*4);
					if (NULL==pal)
					{
						MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bEx_Import=false;
						break;
					}
					ZeroMemory(pal, gii[index].clrb*4);
				}
				else if(2==layer)
				{
					pBufIm=(BYTE*)malloc(sii[index].cx*sii[index].cy*sii[index].bpp/0x08);
					if (NULL==pBufIm)
					{
						MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
						bEx_Import=false;
						break;
					}
					ZeroMemory(pBufIm, sii[index].cx*sii[index].cy*sii[index].bpp/0x08);
					if(0x04==sii[index].bpp)
					{
						pal=(PAL*)malloc(0x40);
						if (NULL==pal)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bEx_Import=false;
							break;
						}
						ZeroMemory(pal, 0x40);
					}
					else if(0x08==sii[index].bpp)
					{
						pal=(PAL*)malloc(0x400);
						if (NULL==pal)
						{
							MessageBox(hDlg, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
							bEx_Import=false;
							break;
						}
						ZeroMemory(pal, 0x400);
					}
				}
				SendMessage(hWndChild, WM_USER+2, (WPARAM)iImPalNo, (LPARAM)pBufIm);
				SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, (WPARAM)iImPalNo, 0);
				bImFile=false;
			}
			bEx_Import=false;
			break;
		case IDCANCEL:
			DestroyWindow(hDlg);
			hWndExplorer = NULL;
			break;
		}
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int col, result=0;
	HWND hWndListView;
	char szCmp1[28], szCmp2[28];
	LVITEM item;
	SORTDATA *sdLoc;

	sdLoc=(SORTDATA *)lParamSort;
	col=sdLoc->col;
	hWndListView=sdLoc->hWndListView;

	if(0!=layer)
	{
		if(0==lParam1)
			return -1;
		if(0==lParam2)
			return 1;
	}
	item.iItem=lParam1;
	item.iSubItem=col;
	item.pszText=szCmp1;
	item.cchTextMax=27;
	item.mask=LVIF_TEXT;
	SendMessage(hWndListView, LVM_GETITEM, 0, (LPARAM) (LPLVITEM)&item);

	item.iItem=lParam2;
	item.pszText=szCmp2;
	SendMessage(hWndListView, LVM_GETITEM, 0, (LPARAM) (LPLVITEM)&item);

	if(sdLoc->bAsc)
		result=_stricmp(szCmp1, szCmp2);
	else
		result=-(_stricmp(szCmp1, szCmp2));
	return result;
}

INT_PTR CALLBACK Ex_ImportDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	int wmId, wmEvent;
	static int iPalNo;
	OPENFILENAME ofn;
	static char szFileIm[MAX_MULTIFILE],szFileImName[MAX_PATH];
	char *szImPalNo;
	int iImPalNo=0, i;

	switch (message)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDC_BUTTON1:
			ZeroMemory(szFileIm, sizeof(szFileIm[MAX_MULTIFILE]));
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = szFileIm;
			ofn.nMaxFile = sizeof(szFileIm);
			ofn.lpstrFilter = "TIM2 File(*.tm2)\0*.tm2\0All Files(*.*)\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = szFileImName;
			ofn.nMaxFileTitle = sizeof(szFileImName);
			ofn.lpstrInitialDir = NULL;
			//ofn.lpstrDefExt = "vlg";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

			if (GetOpenFileName(&ofn)!=TRUE)
				break;
			szFileIm[ofn.nFileOffset-1]='\\';
			for (i=ofn.nFileOffset; i<MAX_MULTIFILE; i++)
			{
				if('\0'==szFileIm[i] && '\0'!=szFileIm[i+1])
					szFileIm[i]=';';
				else if('\0'==szFileIm[i] && '\0'==szFileIm[i+1])
					break;
			}
			SetDlgItemText(hDlg, IDC_EDIT2, szFileIm);
			break;
		case IDC_RADIO1:
			if (BN_CLICKED==wmEvent)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
			}
			break;
		case IDC_RADIO3:
			if (BN_CLICKED==wmEvent)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
			}
			break;
		case IDOK:
			if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_RADIO1))
			{
				bExCurFile=true;
				bExAllFile=false;
				bImFile=false;
			}
			else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_RADIO2))
			{
				bExCurFile=false;
				bExAllFile=true;
				bImFile=false;
			}
			else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_RADIO3))
			{
				bExCurFile=false;
				bExAllFile=false;
				bImFile=true;

				GetDlgItemText(hDlg, IDC_EDIT2, szFileIm, MAX_MULTIFILE);
				szImPalNo=strrchr(szFileIm, '@')+1;
				if(1==(int)szImPalNo)
				{
					MessageBox(hDlg, "The import file name is wrong!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
					bImFile=false;
					break;
				}
				iImPalNo=atoi(szImPalNo);
				if (NULL!=strchr(szFileIm, ';'))
				{
					strcpy_s(szFontTIM2File, MAX_MULTIFILE, szFileIm);
					EndDialog(hDlg, iImPalNo);
					return (INT_PTR)TRUE;
				}
				if (fopen_s(&fpIm,szFileIm,"rb")!=NULL)
				{
					MessageBox(hDlg, "Can't open file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
					bImFile=false;
					break;
				}
			}
		case IDCANCEL:
			_strset(szFontTIM2File, '\0');
			EndDialog(hDlg, iImPalNo);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

COLORREF Alpha(BYTE r, BYTE g, BYTE b, BYTE a)
{
	BYTE AlphaR, AlphaG, AlphaB;
	DWORD color;

	color=GetSysColor(COLOR_GRAYTEXT);
	AlphaR=r*a/255+GetRValue(color)*(255-a)/255;
	AlphaG=g*a/255+GetGValue(color)*(255-a)/255;
	AlphaB=b*a/255+GetBValue(color)*(255-a)/255;

	//return RGB(AlphaR, AlphaG, AlphaB);
	return RGB(AlphaB, AlphaG, AlphaR);//SetDIBits的颜色值需要把R和B对调
}

int GetOnePixel(int clrb, BYTE *pBuf, int sn)
{
	BYTE t;

	if(0x10==clrb || 0x04==clrb)
	{
		if(0==sn%2)
		{
			t=pBuf[sn/2];
			return t&0x0f;
		}
		else
		{
			t=pBuf[sn/2];
			return t>>4;
		}
		
	}
	else if(0x100==clrb || 0x08==clrb)
		return pBuf[sn];

	return -1;
}

bool Draw(HDC hdc, TXPINFO *ti, SUBIMAGEINFO *sii, GM3IMAGEINFO *gii, int iPalNo, BYTE *pBuf)
{
	COLORREF color;
	int p, cx=0,cy=0,xTile=0,yTile=0;
	DWORD *pdwImageData;
	int iPos;

	int cxLoc;
	int cyLoc;
	int bppLoc;
	BYTE flagLoc;

	if(NULL==sii && NULL==gii)
	{
		cxLoc=ti->cx;
		cyLoc=ti->cy;
		bppLoc=ti->clrb;
		flagLoc=ti->flag;
	}
	else if(NULL==ti && NULL==gii)
	{
		cxLoc=sii->cx;
		cyLoc=sii->cy;
		bppLoc=sii->bpp;
		flagLoc=sii->flag;
	}
	else if(NULL==ti && NULL==sii)
	{
		cxLoc=gii->cx;
		cyLoc=gii->cy;
		bppLoc=gii->clrb;
		flagLoc=gii->flag;
	}
	else
		return false;

	pdwImageData=(DWORD*)malloc(cxLoc*cyLoc*sizeof(DWORD));
	if (NULL==pdwImageData)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		return false;
	}
	memset(pdwImageData, 0, cxLoc*cyLoc*sizeof(DWORD));

	//初始化进度条
	SendMessage(hWndPB, PBM_SETRANGE32, 0, (cxLoc*cyLoc)/1024);
    SendMessage(hWndPB, PBM_SETSTEP, (WPARAM) 1, 0);
	SendMessage(hWndPB, PBM_SETPOS, (WPARAM) 0, 0);

	if(0x10==bppLoc || 0x04==bppLoc)//16色tile结构
	{
		tileWidth=32;
		tileHeight=8;
	}
	else if(0x100==bppLoc || 0x08==bppLoc)//256色tile结构
	{
		tileWidth=16;
		tileHeight=8;
	}
	for(i=0;i<cxLoc*cyLoc;i++)
	{
		p=GetOnePixel(bppLoc, pBuf, i);//取颜色索引号
		if(-1==p)
			return false;
		if(bPrinny)//选择prinny图片调色板
			p+=iPalNo*0x10;

		if(0x00==flagLoc)//没有tile结构的图片
		{
			cx=i%cxLoc;
			cy=i/cxLoc;
			color=Alpha(pal[p].r, pal[p].g, pal[p].b, pal[p].a);
			//SetPixelV(hdc, cx, cy, color);
			iPos=cx+cy*cxLoc;
			pdwImageData[iPos]=color;
		}
		else if(0x01==flagLoc)//有tile结构的图片
		{
			cx=i%tileWidth;
			cy=(i/tileWidth)%tileHeight;
			xTile=(i/(tileWidth*tileHeight))%(cxLoc/tileWidth);
			yTile=(i/(tileWidth*tileHeight))/(cxLoc/tileWidth);
			color=Alpha(pal[p].r, pal[p].g, pal[p].b, pal[p].a);
			//SetPixelV(hdc, cx+xTile*tileWidth, cy+yTile*tileHeight, color);
			iPos=(cx+xTile*tileWidth)+(cy+yTile*tileHeight)*cxLoc;
			pdwImageData[iPos]=color;
		}
		if(1023==i%1024)
			SendMessage(hWndPB, PBM_STEPIT, 0, 0);
	}
	char image_info[sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)];
	BITMAPINFOHEADER *info_header = (BITMAPINFOHEADER*) image_info;
	info_header->biSize = sizeof(BITMAPINFOHEADER);
	info_header->biWidth = cxLoc;
	info_header->biHeight = -cyLoc;
	info_header->biPlanes = 1;
	info_header->biBitCount = 32;
	info_header->biCompression = BI_RGB;
	info_header->biSizeImage = 0;
	info_header->biXPelsPerMeter = 0;
	info_header->biYPelsPerMeter = 0;
	info_header->biClrUsed = 0;
	info_header->biClrImportant = 0;
	SetDIBits(hdc, hGraph, 0, cyLoc, pdwImageData, (BITMAPINFO*)info_header, DIB_RGB_COLORS);

	free(pdwImageData);
	pdwImageData=NULL;
	return true;
}

bool DrawImage(HDC hdc, int iPalNo, int iPalNoSum)
{
	BYTE *pBufDraw;
	int maxSize, imageSize;

	if(0==layer)
	{
		pal=(PAL*)malloc(ti[index].palNum*ti[index].clrb*sizeof(PAL));
		if (NULL==pal)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			return false;
		}
		ZeroMemory(pal, ti[index].palNum*ti[index].clrb*sizeof(PAL));

		maxSize=ti[index].palNum*ti[index].clrb;
	}
	else if(11==layer || 12==layer)
	{
		pal=(PAL*)malloc(gii[index].palNum*gii[index].clrb*sizeof(PAL));
		if (NULL==pal)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			return false;
		}
		ZeroMemory(pal, gii[index].palNum*gii[index].clrb*sizeof(PAL));

		maxSize=gii[index].palNum*gii[index].clrb;
	}
	else if(2==layer)
	{
		if(0x04==sii[index].bpp)
		{
			pal=(PAL*)malloc(iPalNoSum*0x10*sizeof(PAL));
			if (NULL==pal)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				return false;
			}
			ZeroMemory(pal, iPalNoSum*0x10*sizeof(PAL));

			maxSize=iPalNoSum*0x10;
		}
		else if(0x08==sii[index].bpp)
		{
			pal=(PAL*)malloc(0x100*sizeof(PAL));
			if (NULL==pal)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				return false;
			}
			ZeroMemory(pal, 0x100*sizeof(PAL));

			maxSize=0x100;
		}
		
		if(bPrinny)
			fseek(fpIn, spi[sii[index].palIndex].PalOffset, 0);
		else if(bHg2)
			fseek(fpIn, spi[iPalNo].PalOffset, 0);
	}
	else
		return false;

	for(i=0;i<maxSize;i++)
	{
		pal[i].r=fgetc(fpIn);
		pal[i].g=fgetc(fpIn);
		pal[i].b=fgetc(fpIn);
		pal[i].a=fgetc(fpIn);
	}

	if(0==layer)
	{
		imageSize=ti[index].cx*ti[index].cy;

		if(0x10==ti[index].clrb)
		{
			pBufDraw=(BYTE*)malloc(imageSize/2);
			if (NULL==pBufDraw)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				free(pal);
				pal=NULL;
				return false;
			}
			ZeroMemory(pBufDraw, imageSize/2);
			fread(pBufDraw, 1, imageSize/2, fpIn);
		}
		else if(0x100==ti[index].clrb)
		{
			pBufDraw=(BYTE*)malloc(imageSize);
			if (NULL==pBufDraw)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				free(pal);
				pal=NULL;
				return false;
			}
			ZeroMemory(pBufDraw, imageSize);
			fread(pBufDraw, 1, imageSize, fpIn);
		}

		if(!Draw(hdc, &ti[index], NULL, NULL, iPalNo, pBufDraw))
			return false;
	}
	else if(11==layer || 12==layer)
	{
		imageSize=gii[index].cx*gii[index].cy;

		if(0x10==gii[index].clrb)
		{
			pBufDraw=(BYTE*)malloc(imageSize/2);
			if (NULL==pBufDraw)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				free(pal);
				pal=NULL;
				return false;
			}
			ZeroMemory(pBufDraw, imageSize/2);
			fread(pBufDraw, 1, imageSize/2, fpIn);
		}
		else if(0x100==gii[index].clrb)
		{
			pBufDraw=(BYTE*)malloc(imageSize);
			if (NULL==pBufDraw)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				free(pal);
				pal=NULL;
				return false;
			}
			ZeroMemory(pBufDraw, imageSize);
			fread(pBufDraw, 1, imageSize, fpIn);
		}

		if(!Draw(hdc, NULL, NULL, &gii[index], iPalNo, pBufDraw))
			return false;
	}
	else if(2==layer)
	{
		imageSize=sii[index].cx*sii[index].cy;
		fseek(fpIn, sii[index].ImageOffset, 0);

		if(0x04==sii[index].bpp)
		{
			pBufDraw=(BYTE*)malloc(imageSize/2);
			if (NULL==pBufDraw)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				free(pal);
				pal=NULL;
				return false;
			}
			ZeroMemory(pBufDraw, imageSize/2);
			fread(pBufDraw, 1, imageSize/2, fpIn);
		}
		else if(0x08==sii[index].bpp)
		{
			pBufDraw=(BYTE*)malloc(imageSize);
			if (NULL==pBufDraw)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				free(pal);
				pal=NULL;
				return false;
			}
			ZeroMemory(pBufDraw, imageSize);
			fread(pBufDraw, 1, imageSize, fpIn);
		}

		if(!Draw(hdc, NULL, &sii[index], NULL, iPalNo, pBufDraw))
			return false;
	}

	free(pal);
	pal=NULL;
	free(pBufDraw);
	pBufDraw=NULL;
	return true;
}

bool Export(TXPINFO *ti, SUBIMAGEINFO *sii, GM3IMAGEINFO *gii, BYTE *pBuf)
{
	BYTE c;
	int pos,cx=0,cy=0,xTile=0,yTile=0;

	int cxLoc;
	int cyLoc;
	int bppLoc;
	BYTE flagLoc;

	if(NULL==sii && NULL==gii)
	{
		cxLoc=ti->cx;
		cyLoc=ti->cy;
		bppLoc=ti->clrb;
		flagLoc=ti->flag;
	}
	else if(NULL==ti && NULL==gii)
	{
		cxLoc=sii->cx;
		cyLoc=sii->cy;
		bppLoc=sii->bpp;
		flagLoc=sii->flag;
	}
	else if(NULL==ti && NULL==sii)
	{
		cxLoc=gii->cx;
		cyLoc=gii->cy;
		bppLoc=gii->clrb;
		flagLoc=gii->flag;
	}
	else
		return false;

	for(i=0;i<cxLoc*cyLoc;i++)
	{
		if(0x10==bppLoc || 0x04==bppLoc)//16色
		{
			if(0x00==flagLoc)//16色无tile结构的txp图片
			{
				if(0==i%2)
					c=fgetc(fpIn);
				else
					pBuf[i/2]=c;
			}
			else if(0x01==flagLoc)//16色有tile结构的txp图片
			{
				tileWidth=32;
				tileHeight=8;

				if(0==i%2)
					c=fgetc(fpIn);
				else
				{
					cx=i%tileWidth;
					cy=(i/tileWidth)%tileHeight;
					xTile=(i/(tileWidth*tileHeight))%(cxLoc/tileWidth);
					yTile=(i/(tileWidth*tileHeight))/(cxLoc/tileWidth);
					pos=(cy+yTile*tileHeight)*cxLoc+(cx+xTile*tileWidth);
					pBuf[pos/2]=c;
				}
			}
			else
				return false;
		}
		else if(0x100==bppLoc || 0x08==bppLoc)//256色
		{
			if(0x00==flagLoc)//256色无tile结构的txp图片
			{
				c=fgetc(fpIn);
				pBuf[i]=c;
			}
			else if(0x01==flagLoc)//256色有tile结构的txp图片
			{
				tileWidth=16;
				tileHeight=8;

				c=fgetc(fpIn);
				cx=i%tileWidth;
				cy=(i/tileWidth)%tileHeight;
				xTile=(i/(tileWidth*tileHeight))%(cxLoc/tileWidth);
				yTile=(i/(tileWidth*tileHeight))/(cxLoc/tileWidth);
				pos=(cy+yTile*tileHeight)*cxLoc+(cx+xTile*tileWidth);
				pBuf[pos]=c;
			}
			else
				return false;
		}
	}
	return true;
}

bool TXP2TIM2(int iPalNo, char *szOutFileName)
{
	DWORD imageSize,fileSize;
	short palSize;
	BYTE *pBufEx;

	if (ti[index].cy>MAX_HEIGHT)//如果字库图片高度大于能处理的最大值，则转入字库图片导出函数处理
	{
		if(!TXP2FontTIM2(iPalNo, szOutFileName))
		{
			return false;
		}
		return true;
	}
	//写tim2文件头
	fwrite("TIM2", 4, 1, fpOut);
	fputc(0x04, fpOut);
	fputc(0x00, fpOut);
	fputc(0x01, fpOut);
	fwrite("\0\0\0\0\0\0\0\0", 1, 9, fpOut);
	if (bPrinny)
	{
		if(0x10==ti[index].clrb)
			imageSize=ti[index].cx*ti[index].cy/2;
		else if(0x100==ti[index].clrb)
			imageSize=ti[index].cx*ti[index].cy;
		palSize=ti[index].clrb*4;
		fileSize=imageSize+palSize+0x40;
		fileSize-=16;
		fwrite(&fileSize, 4, 1, fpOut);
		fwrite(&palSize, 2, 1, fpOut);
		fwrite("\0", 1, 2, fpOut);
		fwrite(&imageSize, 4, 1, fpOut);
		fputc(0x30, fpOut);
		fputc(0x00, fpOut);
		fwrite(&ti[index].clrb, 2, 1, fpOut);
		fputc(0x00, fpOut);
		fputc(0x01, fpOut);
		fputc(0x83, fpOut);
		if(0x10==ti[index].clrb)
			fputc(0x04, fpOut);
		else if(0x100==ti[index].clrb)
			fputc(0x05, fpOut);
	} 
	else if(bHg2)
	{
		imageSize=ti[index].cx*ti[index].cy*4;
		palSize=0;
		fileSize=imageSize+palSize+0x40;
		fileSize-=16;
		fwrite(&fileSize, 4, 1, fpOut);
		fwrite(&palSize, 2, 1, fpOut);
		fwrite("\0", 1, 2, fpOut);
		fwrite(&imageSize, 4, 1, fpOut);
		fwrite("\x30\0\0\0\0\x01\0\x03", 1, 8, fpOut);
	}
	fwrite(&ti[index].cx, 2, 1, fpOut);
	fwrite(&ti[index].cy, 2, 1, fpOut);
	fwrite("\0\0\x40\x21\x02\0\0\0\x60\x02\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 24, fpOut);

	//写图像数据
	pBufEx=(BYTE*)malloc(imageSize);
	if (NULL==pBufEx)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		return false;
	}
	memset(pBufEx, 0, imageSize);
	if (bPrinny)
	{
		fseek(fpIn, fi[index].Offset, 0);
		if(bTRP1)
			fseek(fpIn, dwTRP1Offset, 1);
		fseek(fpIn, ti[index].clrb*4*ti[index].palNum+16, 1);
		if(!Export(&ti[index], NULL, NULL, pBufEx))
		{
			free(pBufEx);
			pBufEx=NULL;
			return false;
		}
	} 
	else if(bHg2)
	{
		fseek(fpIn, fi[index].Offset+16, 0);
		DecodeDXTn(NULL, pBufEx, NULL);
	}
	fwrite(pBufEx, 1, imageSize, fpOut);
	free(pBufEx);
	pBufEx=NULL;

	//写调色板数据
	if (bPrinny)
	{
		fseek(fpIn, fi[index].Offset+16+iPalNo*palSize, 0);
		if(bTRP1)
			fseek(fpIn, dwTRP1Offset, 1);
		pBufEx=(BYTE*)malloc(palSize);
		if (NULL==pBufEx)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			return false;
		}
		ZeroMemory(pBufEx, palSize);
		fread(pBufEx, 1, palSize, fpIn);
		fwrite(pBufEx, 1, palSize, fpOut);
		free(pBufEx);
		pBufEx=NULL;
	}
	return true;
}

bool DAT2TIM2(int iPalNo)
{
	DWORD imageSize,fileSize;
	short palSize, clrb;
	BYTE *pBufEx;

	//写tim2文件头
	fwrite("TIM2", 4, 1, fpOut);
	fputc(0x04, fpOut);
	fputc(0x00, fpOut);
	fputc(0x01, fpOut);
	fwrite("\0\0\0\0\0\0\0\0", 1, 9, fpOut);
	imageSize=sii[index].cx*sii[index].cy*sii[index].bpp/0x08;
	if(0x04==sii[index].bpp)
		palSize=0x40;
	else if(0x08==sii[index].bpp)
		palSize=0x400;
	fileSize=imageSize+palSize+0x40;
	fileSize-=16;
	fwrite(&fileSize, 4, 1, fpOut);
	fwrite(&palSize, 2, 1, fpOut);
	fwrite("\0", 1, 2, fpOut);
	fwrite(&imageSize, 4, 1, fpOut);
	fputc(0x30, fpOut);
	fputc(0x00, fpOut);
	clrb=palSize/4;
	fwrite(&clrb, 2, 1, fpOut);
	fputc(0x00, fpOut);
	fputc(0x01, fpOut);
	fputc(0x83, fpOut);
	if(0x04==sii[index].bpp)
		fputc(0x04, fpOut);
	else if(0x08==sii[index].bpp)
		fputc(0x05, fpOut);
	fwrite(&sii[index].cx, 2, 1, fpOut);
	fwrite(&sii[index].cy, 2, 1, fpOut);
	fwrite("\0\0\x40\x21\x02\0\0\0\x60\x02\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 24, fpOut);

	//写图像数据
	pBufEx=(BYTE*)malloc(imageSize);
	if (NULL==pBufEx)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		return false;
	}
	ZeroMemory(pBufEx, imageSize);
	fseek(fpIn, sii[index].ImageOffset, 0);
	if(!Export(NULL, &sii[index], NULL, pBufEx))
	{
		free(pBufEx);
		pBufEx=NULL;
		return false;
	}
	fwrite(pBufEx, 1, imageSize, fpOut);
	free(pBufEx);
	pBufEx=NULL;

	//写调色板数据
	if(bPrinny)
		fseek(fpIn, spi[sii[index].palIndex].PalOffset+iPalNo*0x40, 0);
	else if(bHg2)
		fseek(fpIn, spi[iPalNo].PalOffset, 0);

	pBufEx=(BYTE*)malloc(palSize);
	if (NULL==pBufEx)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		return false;
	}
	ZeroMemory(pBufEx, palSize);
	fread(pBufEx, 1, palSize, fpIn);
	fwrite(pBufEx, 1, palSize, fpOut);
	free(pBufEx);
	pBufEx=NULL;
	return true;
}

bool GM32TIM2(int iPalNo)
{
	DWORD imageSize,fileSize;
	short palSize;
	BYTE *pBufEx;

	//写tim2文件头
	fwrite("TIM2", 4, 1, fpOut);
	fputc(0x04, fpOut);
	fputc(0x00, fpOut);
	fputc(0x01, fpOut);
	fwrite("\0\0\0\0\0\0\0\0", 1, 9, fpOut);
	if(0x10==gii[index].clrb)
		imageSize=gii[index].cx*gii[index].cy/2;
	else if(0x100==gii[index].clrb)
		imageSize=gii[index].cx*gii[index].cy;
	palSize=gii[index].clrb*4;
	fileSize=imageSize+palSize+0x40;
	fileSize-=16;
	fwrite(&fileSize, 4, 1, fpOut);
	fwrite(&palSize, 2, 1, fpOut);
	fwrite("\0", 1, 2, fpOut);
	fwrite(&imageSize, 4, 1, fpOut);
	fputc(0x30, fpOut);
	fputc(0x00, fpOut);
	fwrite(&gii[index].clrb, 2, 1, fpOut);
	fputc(0x00, fpOut);
	fputc(0x01, fpOut);
	fputc(0x83, fpOut);
	if(0x10==gii[index].clrb)
		fputc(0x04, fpOut);
	else if(0x100==gii[index].clrb)
		fputc(0x05, fpOut);
	fwrite(&gii[index].cx, 2, 1, fpOut);
	fwrite(&gii[index].cy, 2, 1, fpOut);
	fwrite("\0\0\x40\x21\x02\0\0\0\x60\x02\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 24, fpOut);

	//写图像数据
	pBufEx=(BYTE*)malloc(imageSize);
	if (NULL==pBufEx)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		return false;
	}
	ZeroMemory(pBufEx, imageSize);
	fseek(fpIn, di[index].Offset, 0);
	fseek(fpIn, gii[index].clrb*4*gii[index].palNum+16, 1);
	if(!Export(NULL, NULL, &gii[index], pBufEx))
	{
		free(pBufEx);
		pBufEx=NULL;
		return false;
	}
	fwrite(pBufEx, 1, imageSize, fpOut);
	free(pBufEx);
	pBufEx=NULL;

	//写调色板数据
	fseek(fpIn, di[index].Offset+16+iPalNo*palSize, 0);
	pBufEx=(BYTE*)malloc(palSize);
	if (NULL==pBufEx)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		return false;
	}
	ZeroMemory(pBufEx, palSize);
	fread(pBufEx, 1, palSize, fpIn);
	fwrite(pBufEx, 1, palSize, fpOut);
	free(pBufEx);
	pBufEx=NULL;
	return true;
}

bool Import(TXPINFO *ti, SUBIMAGEINFO *sii, GM3IMAGEINFO *gii, BYTE *pBuf)
{
	BYTE p;
	int pos,cx=0,cy=0,xTile=0,yTile=0;

	int cxLoc;
	int cyLoc;
	int bppLoc;
	BYTE flagLoc;

	if(NULL==sii && NULL==gii)
	{
		cxLoc=ti->cx;
		cyLoc=ti->cy;
		bppLoc=ti->clrb;
		flagLoc=ti->flag;
	}
	else if(NULL==ti && NULL==gii)
	{
		cxLoc=sii->cx;
		cyLoc=sii->cy;
		bppLoc=sii->bpp;
		flagLoc=sii->flag;
	}
	else if(NULL==ti && NULL==sii)
	{
		cxLoc=gii->cx;
		cyLoc=gii->cy;
		bppLoc=gii->clrb;
		flagLoc=gii->flag;
	}
	else
		return false;

	for(i=0;i<cxLoc*cyLoc;i++)
	{
		if(0x10==bppLoc || 0x04==bppLoc)//16色
		{
			if(0x00==flagLoc)//16色无tile结构的txp图片
			{
				if(0==i%2)
					p=pBuf[i/2];
				else
					pBufIm[i/2]=p;
			}
			else if(0x01==flagLoc)//16色有tile结构的txp图片
			{
				tileWidth=32;
				tileHeight=8;

				if(0==i%2)
				{
					cx=i%tileWidth;
					cy=(i/tileWidth)%tileHeight;
					xTile=(i/(tileWidth*tileHeight))%(cxLoc/tileWidth);
					yTile=(i/(tileWidth*tileHeight))/(cxLoc/tileWidth);
					pos=(cy+yTile*tileHeight)*cxLoc+(cx+xTile*tileWidth);
					pBufIm[i/2]=pBuf[pos/2];
				}
			}
			else
				return false;
		}
		else if(0x100==bppLoc || 0x08==bppLoc)//256色
		{
			if(0x00==flagLoc)//256色无tile结构的txp图片
			{
				p=pBuf[i];
				pBufIm[i]=p;
			}
			else if(0x01==flagLoc)//256色有tile结构的txp图片
			{
				tileWidth=16;
				tileHeight=8;

				cx=i%tileWidth;
				cy=(i/tileWidth)%tileHeight;
				xTile=(i/(tileWidth*tileHeight))%(cxLoc/tileWidth);
				yTile=(i/(tileWidth*tileHeight))/(cxLoc/tileWidth);
				pos=(cy+yTile*tileHeight)*cxLoc+(cx+xTile*tileWidth);
				pBufIm[i]=pBuf[pos];
			}
			else
				return false;
		}
	}
	return true;
}

bool TIM22TXP(HDC hdc)
{
	BYTE p, *pBufOri;
	char szFlag[5];

	if (NULL!=szFontTIM2File[0])
	{
		if(!FontTIM22TXP(hdc))
		{
			return false;
		}
		return true;
	}
	tim2=(TIM2*)malloc(sizeof(TIM2));
	if (NULL==tim2)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		return false;
	}
	ZeroMemory(tim2, sizeof(TIM2));
	fread(szFlag, 4, 1, fpIm);
	szFlag[4]='\0';
	if(strcmp("TIM2", szFlag))
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fseek(fpIm, 0x10L, 0);
	fread(&tim2->fileSize, 4, 1, fpIm);
	tim2->fileSize+=16;
	fread(&tim2->palSize, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->imageSize, 4, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->clrb, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	p=fgetc(fpIm);
	if(0x83!=p && 0x03!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	p=fgetc(fpIm);
	if(0x04!=p && 0x05!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fread(&tim2->cx, 2, 1, fpIm);
	fread(&tim2->cy, 2, 1, fpIm);
	//加载tim2的图片信息至ti[index]结构中
	ti[index].cx=tim2->cx;
	ti[index].cy=tim2->cy;
	ti[index].clrb=tim2->clrb;

	fseek(fpIm, 0x40L+tim2->imageSize, 0);
	fpos=ftell(fpIm);
	for(i=0;i<tim2->palSize/4;i++)
	{
		pal[i].r=fgetc(fpIm);
		pal[i].g=fgetc(fpIm);
		pal[i].b=fgetc(fpIm);
		pal[i].a=fgetc(fpIm);
	}

	fseek(fpIm, 0x40L, 0);
	pBufOri=(BYTE*)malloc(tim2->imageSize);
	if (NULL==pBufOri)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		free(tim2);
		tim2=NULL;
		return false;
	}
	ZeroMemory(pBufOri, tim2->imageSize);

	//读入TIM2图像数据到pBufOri缓冲区中
	fread(pBufOri, 1, tim2->imageSize, fpIm);

	//从pBufOri缓冲区中导入TIM2图像数据到pBufIm缓冲区中
	if(!Import(&ti[index], NULL, NULL, pBufOri))
	{
		fclose(fpIm);
		free(pBufOri);
		pBufOri=NULL;
		free(tim2);
		tim2=NULL;
		return false;
	}

	//预览TIM2图像
	Draw(hdc, &ti[index], NULL, NULL, 0, pBufIm);

	fclose(fpIm);
	free(pBufOri);
	pBufOri=NULL;
	free(tim2);
	tim2=NULL;
	return true;
}

bool TIM22DAT(HDC hdc)
{
	BYTE p, *pBufOri;
	char szFlag[5];

	tim2=(TIM2*)malloc(sizeof(TIM2));
	if (NULL==tim2)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		return false;
	}
	ZeroMemory(tim2, sizeof(TIM2));
	fread(szFlag, 4, 1, fpIm);
	szFlag[4]='\0';
	if(strcmp("TIM2", szFlag))
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fseek(fpIm, 0x10L, 0);
	fread(&tim2->fileSize, 4, 1, fpIm);
	tim2->fileSize+=16;
	fread(&tim2->palSize, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->imageSize, 4, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->clrb, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	p=fgetc(fpIm);
	if(0x83!=p && 0x03!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	p=fgetc(fpIm);
	if(0x04!=p && 0x05!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fread(&tim2->cx, 2, 1, fpIm);
	fread(&tim2->cy, 2, 1, fpIm);
	fseek(fpIm, 0x40L+tim2->imageSize, 0);
	fpos=ftell(fpIm);

	for(i=0;i<tim2->palSize/4;i++)
	{
		pal[i].r=fgetc(fpIm);
		pal[i].g=fgetc(fpIm);
		pal[i].b=fgetc(fpIm);
		pal[i].a=fgetc(fpIm);
	}

	fseek(fpIm, 0x40L, 0);
	pBufOri=(BYTE*)malloc(tim2->imageSize);
	if (NULL==pBufOri)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		free(tim2);
		tim2=NULL;
		return false;
	}
	ZeroMemory(pBufOri, tim2->imageSize);

	//读入TIM2图像数据到pBufOri缓冲区中
	fread(pBufOri, 1, tim2->imageSize, fpIm);

	//从pBufOri缓冲区中导入TIM2图像数据到pBufIm缓冲区中
	if(!Import(NULL, &sii[index], NULL, pBufOri))
	{
		fclose(fpIm);
		free(pBufOri);
		pBufOri=NULL;
		free(tim2);
		tim2=NULL;
		return false;
	}

	//预览TIM2图像
	Draw(hdc, NULL, &sii[index], NULL, 0, pBufIm);

	fclose(fpIm);
	free(pBufOri);
	pBufOri=NULL;
	free(tim2);
	tim2=NULL;
	return true;
}

bool TIM22GM3(HDC hdc)
{
	BYTE p, *pBufOri;
	char szFlag[5];

	tim2=(TIM2*)malloc(sizeof(TIM2));
	if (NULL==tim2)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		return false;
	}
	ZeroMemory(tim2, sizeof(TIM2));
	fread(szFlag, 4, 1, fpIm);
	szFlag[4]='\0';
	if(strcmp("TIM2", szFlag))
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fseek(fpIm, 0x10L, 0);
	fread(&tim2->fileSize, 4, 1, fpIm);
	tim2->fileSize+=16;
	fread(&tim2->palSize, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->imageSize, 4, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->clrb, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	p=fgetc(fpIm);
	if(0x83!=p && 0x03!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	p=fgetc(fpIm);
	if(0x04!=p && 0x05!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fread(&tim2->cx, 2, 1, fpIm);
	fread(&tim2->cy, 2, 1, fpIm);
	fseek(fpIm, 0x40L+tim2->imageSize, 0);
	fpos=ftell(fpIm);

	for(i=0;i<tim2->palSize/4;i++)
	{
		pal[i].r=fgetc(fpIm);
		pal[i].g=fgetc(fpIm);
		pal[i].b=fgetc(fpIm);
		pal[i].a=fgetc(fpIm);
	}

	fseek(fpIm, 0x40L, 0);
	pBufOri=(BYTE*)malloc(tim2->imageSize);
	if (NULL==pBufOri)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		free(tim2);
		tim2=NULL;
		fclose(fpIm);
		return false;
	}
	ZeroMemory(pBufOri, tim2->imageSize);

	//读入TIM2图像数据到pBufOri缓冲区中
	fread(pBufOri, 1, tim2->imageSize, fpIm);

	//从pBufOri缓冲区中导入TIM2图像数据到pBufIm缓冲区中
	if(!Import(NULL, NULL, &gii[index], pBufOri))
	{
		fclose(fpIm);
		free(pBufOri);
		pBufOri=NULL;
		free(tim2);
		tim2=NULL;
		return false;
	}

	//预览TIM2图像
	Draw(hdc, NULL, NULL, &gii[index], 0, pBufIm);

	fclose(fpIm);
	free(pBufOri);
	pBufOri=NULL;
	free(tim2);
	tim2=NULL;
	return true;
}

bool DecodeDXTn(HDC hdc, BYTE *pByBufOut, BYTE *pByBufIm)
{
	BYTE *pByBufIn;
	int i, iTexelNum, tx, ty;

	if (0==ti[index].clrb)
	{//DXT1
		if (NULL==pByBufIm)
		{
			pByBufIn=(BYTE*)malloc(ti[index].cx*ti[index].cy*sizeof(BYTE)/2);
			if (NULL==pByBufIn)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				return false;
			}
			memset(pByBufIn, 0, ti[index].cx*ti[index].cy*sizeof(BYTE)/2);
			fread(pByBufIn, ti[index].cx*ti[index].cy*sizeof(BYTE)/2, 1, fpIn);
		}
		else
		{
			pByBufIn=pByBufIm;
		}

		iTexelNum=ti[index].cx*ti[index].cy/16;
		//初始化进度条
		SendMessage(hWndPB, PBM_SETRANGE32, 0, iTexelNum);
		SendMessage(hWndPB, PBM_SETSTEP, (WPARAM) 1, 0);
		SendMessage(hWndPB, PBM_SETPOS, (WPARAM) 0, 0);
		for (i=0; i<iTexelNum; i++)
		{
			tx=(i%(ti[index].cx/4))*4;
			ty=(i/(ti[index].cx/4))*4;
			DecodeOneTexel(hdc, tx, ty, pByBufIn+i*8, pByBufOut, 0);
			SendMessage(hWndPB, PBM_STEPIT, 0, 0);
		}
	} 
	else
	{//DXT3 and DXT5
		if (NULL==pByBufIm)
		{
			pByBufIn=(BYTE*)malloc(ti[index].cx*ti[index].cy*sizeof(BYTE));
			if (NULL==pByBufIn)
			{
				MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
				return false;
			}
			memset(pByBufIn, 0, ti[index].cx*ti[index].cy*sizeof(BYTE));
			fread(pByBufIn, ti[index].cx*ti[index].cy*sizeof(BYTE), 1, fpIn);
		}
		else
		{
			pByBufIn=pByBufIm;
		}


		iTexelNum=ti[index].cx*ti[index].cy/16;
		//初始化进度条
		SendMessage(hWndPB, PBM_SETRANGE32, 0, iTexelNum);
		SendMessage(hWndPB, PBM_SETSTEP, (WPARAM) 1, 0);
		SendMessage(hWndPB, PBM_SETPOS, (WPARAM) 0, 0);
		for (i=0; i<iTexelNum; i++)
		{
			tx=(i%(ti[index].cx/4))*4;
			ty=(i/(ti[index].cx/4))*4;
			DecodeOneTexel(hdc, tx, ty, pByBufIn+i*16, pByBufOut, 2);
			SendMessage(hWndPB, PBM_STEPIT, 0, 0);
		}
	}
	char image_info[sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)];
	BITMAPINFOHEADER *info_header = (BITMAPINFOHEADER*) image_info;
	info_header->biSize = sizeof(BITMAPINFOHEADER);
	info_header->biWidth = ti[index].cx;
	info_header->biHeight = -ti[index].cy;
	info_header->biPlanes = 1;
	info_header->biBitCount = 32;
	info_header->biCompression = BI_RGB;
	info_header->biSizeImage = 0;
	info_header->biXPelsPerMeter = 0;
	info_header->biYPelsPerMeter = 0;
	info_header->biClrUsed = 0;
	info_header->biClrImportant = 0;
	SetDIBits(hdc, hGraph, 0, ti[index].cy, pByBufOut, (BITMAPINFO*)info_header, DIB_RGB_COLORS);

	if (NULL==pByBufIm)
	{
		free(pByBufIn);
		pByBufIn=NULL;
	}
	return true;
}

bool DecodeOneTexel(HDC hdc, int x, int y, BYTE *pBufIn, BYTE *pBufOut, int flag)
{
	DWORD dwBuf;
	BYTE cIndex[4][4], aIndex[4][4], r, g, b, alpha8[8];
	USHORT pal16[2], wBuf[3];
	int iCounter=0, j, k;
	COLORREF palClr[4], color;

	memcpy((void *)&dwBuf, (void*)pBufIn, 4);
	//dwBuf=DwReverse(dwBuf);
	for (j=3; j>=0; j--)
	{
		for (k=3; k>=0; k--)
		{
			cIndex[j][k]=(BYTE)(dwBuf>>(30-iCounter*2));
			cIndex[j][k]=cIndex[j][k] & 0x03;
			iCounter++;
		}
	}
	memcpy((void *)&dwBuf, (void *)(pBufIn+4), 4);
	pal16[1]=(USHORT)(dwBuf>>16);
	pal16[0]=(USHORT)(dwBuf & 0x0000ffff);
	if (1==flag)//DXT3 Alpha data
	{
		memcpy((void *)&dwBuf, (void *)(pBufIn+8), 4);
		dwBuf=DwReverse(dwBuf);
		iCounter=0;
		for (j=3; j>=2; j--)
		{
			for (k=3; k>=0; k--)
			{
				aIndex[j][k]=(BYTE)(dwBuf>>(28-iCounter*4));
				aIndex[j][k]=(aIndex[j][k] & 0x0f)<<4;
				iCounter++;
			}
		}
		memcpy((void *)&dwBuf, (void *)(pBufIn+12), 4);
		dwBuf=DwReverse(dwBuf);
		iCounter=0;
		for (j=1; j>=0; j--)
		{
			for (k=3; k>=0; k--)
			{
				aIndex[j][k]=(BYTE)(dwBuf>>(28-iCounter*4));
				aIndex[j][k]=(aIndex[j][k] & 0x0f)<<4;
				iCounter++;
			}
		}
	}
	else if (2==flag)//DXT5 Alpha data
	{
		memcpy((void *)&wBuf[0], (void *)(pBufIn+8), 2);
		memcpy((void *)&wBuf[1], (void *)(pBufIn+10), 2);
		memcpy((void *)&wBuf[2], (void *)(pBufIn+12), 2);
		dwBuf=((DWORD)wBuf[2]<<16) | ((DWORD)wBuf[1] & 0x0000ffff);
		iCounter=0;
		for (j=3; j>=2; j--)
		{
			for (k=3; k>=0; k--)
			{
				aIndex[j][k]=(BYTE)(dwBuf>>(29-iCounter*3));
				aIndex[j][k]=aIndex[j][k] & 0x07;
				iCounter++;
			}
		}
		dwBuf=((DWORD)wBuf[1]<<16) | ((DWORD)wBuf[0] & 0x0000ffff);
		dwBuf=dwBuf<<8;
		iCounter=0;
		for (j=1; j>=0; j--)
		{
			for (k=3; k>=0; k--)
			{
				aIndex[j][k]=(BYTE)(dwBuf>>(29-iCounter*3));
				aIndex[j][k]=aIndex[j][k] & 0x07;
				iCounter++;
			}
		}
		memcpy((void *)&alpha8[0], (void *)(pBufIn+14), 1);
		memcpy((void *)&alpha8[1], (void *)(pBufIn+15), 1);
		if (alpha8[0]>alpha8[1])
		{
			alpha8[2]=(alpha8[0]*6+alpha8[1]+3)/7;
			alpha8[3]=(alpha8[0]*5+alpha8[1]*2+3)/7;
			alpha8[4]=(alpha8[0]*4+alpha8[1]*3+3)/7;
			alpha8[5]=(alpha8[0]*3+alpha8[1]*4+3)/7;
			alpha8[6]=(alpha8[0]*2+alpha8[1]*5+3)/7;
			alpha8[7]=(alpha8[0]+alpha8[1]*6+3)/7;
		} 
		else
		{
			alpha8[2]=(alpha8[0]*4+alpha8[1]+2)/5;
			alpha8[3]=(alpha8[0]*3+alpha8[1]*2+2)/5;
			alpha8[4]=(alpha8[0]*2+alpha8[1]*3+2)/5;
			alpha8[5]=(alpha8[0]+alpha8[1]*4+2)/5;
			alpha8[6]=0;
			alpha8[7]=255;
		}
	}
	palClr[0]=RGB(R565(pal16[0]), G565(pal16[0]), B565(pal16[0]));
	palClr[1]=RGB(R565(pal16[1]), G565(pal16[1]), B565(pal16[1]));
	if (pal16[0]>pal16[1])
	{
		r=(R565(pal16[0])*2+R565(pal16[1])+1)/3;
		g=(G565(pal16[0])*2+G565(pal16[1])+1)/3;
		b=(B565(pal16[0])*2+B565(pal16[1])+1)/3;
		palClr[2]=RGB(r, g, b);
		r=(R565(pal16[0])+R565(pal16[1])*2+1)/3;
		g=(G565(pal16[0])+G565(pal16[1])*2+1)/3;
		b=(B565(pal16[0])+B565(pal16[1])*2+1)/3;
		palClr[3]=RGB(r, g, b);
	} 
	else
	{
		r=(R565(pal16[0])+R565(pal16[1]))/2;
		g=(G565(pal16[0])+G565(pal16[1]))/2;
		b=(B565(pal16[0])+B565(pal16[1]))/2;
		palClr[2]=RGB(r, g, b);
		palClr[3]=0;//GetSysColor(COLOR_GRAYTEXT);
	}
	if (0==flag)//DXT1
	{
		for (j=0; j<4; j++)
		{
			for (k=0; k<4; k++)
			{
				if (bExCurFile)
				{
					palClr[cIndex[j][k]]|=0xff000000;
					memcpy(pBufOut+((x+k)+(y+j)*ti[index].cx)*4, &palClr[cIndex[j][k]], 4);
				}
				else
				{
					//SetPixelV(hdc, x+k, y+j, palClr[cIndex[j][k]]);
					color=palClr[cIndex[j][k]];
					DWORD t=color & 0x00ff0000;
					color=((color & 0x000000ff)<<16) | (color & 0xff00ffff);
					color=((t>>16) & 0x000000ff) | (color & 0xffffff00);
					memcpy(pBufOut+((x+k)+(y+j)*ti[index].cx)*4, &color, 4);
				}
			}
		}
	} 
	else//DXT3,DXT5
	{
		for (j=0; j<4; j++)
		{
			for (k=0; k<4; k++)
			{
				if (bExCurFile)
				{
					palClr[cIndex[j][k]]&=0x00ffffff;
					palClr[cIndex[j][k]]|=(DWORD)alpha8[aIndex[j][k]]<<24;
					memcpy(pBufOut+((x+k)+(y+j)*ti[index].cx)*4, &palClr[cIndex[j][k]], 4);
				}
				else
				{
					color=Alpha(GetRValue(palClr[cIndex[j][k]]),
								GetGValue(palClr[cIndex[j][k]]),
								GetBValue(palClr[cIndex[j][k]]),
								alpha8[aIndex[j][k]]);
					//SetPixelV(hdc, x+k, y+j, color);
					memcpy(pBufOut+((x+k)+(y+j)*ti[index].cx)*4, &color, 4);
				}
			}
		}
	}
	return true;
}

inline DWORD DwReverse(DWORD Input)
{
	DWORD Output;

	Output=Input>>16 | Input<<16;
	Output=(((Output & 0xffff0000)>>8 | (Output & 0xffff0000)<<8) & 0xffff0000) | (((Output & 0x0000ffff)>>8 | (Output & 0x0000ffff)<<8) & 0x0000ffff);
	return Output;
}

//  获得 R5G6B5 红色分量 
inline BYTE R565( USHORT clr )
{
	//return ((clr & 0xF800 )>>11)*255/31;
	return ((clr & 0xF800 )>>8);
} 

//  获得 R5G6B5 绿色分量 
inline BYTE G565( USHORT clr )
{
	//return ((clr & 0x07E0 )>>5)*255/63;
	return ((clr & 0x07E0 )>>3);
} 

//  获得 R5G6B5 蓝色分量 
inline BYTE B565( USHORT clr )
{
	//return (clr & 0x001F)*255/31;
	return (clr & 0x001F)<<3;
} 

bool TIM22DXTn(HDC hdc)
{
	BYTE p, *pBufOri;
	char szFlag[5];
	int iTexelNum;

	tim2=(TIM2*)malloc(sizeof(TIM2));
	if (NULL==tim2)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		return false;
	}
	ZeroMemory(tim2, sizeof(TIM2));
	fread(szFlag, 4, 1, fpIm);
	szFlag[4]='\0';
	if(strcmp("TIM2", szFlag))
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fseek(fpIm, 0x10L, 0);
	fread(&tim2->fileSize, 4, 1, fpIm);
	tim2->fileSize+=16;
	fread(&tim2->palSize, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->imageSize, 4, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->clrb, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	p=fgetc(fpIm);
	if(0x00!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	p=fgetc(fpIm);
	if(0x03!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fread(&tim2->cx, 2, 1, fpIm);
	fread(&tim2->cy, 2, 1, fpIm);

	fseek(fpIm, 0x40L, 0);
	pBufOri=(BYTE*)malloc(tim2->imageSize);
	if (NULL==pBufOri)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		free(tim2);
		tim2=NULL;
		return false;
	}
	memset(pBufOri, 0, tim2->imageSize);

	//读入TIM2图像数据到pBufOri缓冲区中
	fread(pBufOri, 1, tim2->imageSize, fpIm);

	//从pBufOri缓冲区中编码DXT数据输出到pBufIm中
	iTexelNum=tim2->cx*tim2->cy/16;
	if(0==ti[index].clrb)
		squish::CompressImage(pBufOri, tim2->cx, tim2->cy, pBufIm, squish::kDxt1/* | squish::kWeightColourByAlpha | squish::kColourMetricUniform*/);
	else if(2==ti[index].clrb)
		squish::CompressImage(pBufOri, tim2->cx, tim2->cy, pBufIm, squish::kDxt5/* | squish::kWeightColourByAlpha | squish::kColourMetricUniform*/);
	FormatPSPDXTn(pBufIm, ti[index].clrb, iTexelNum);

	//预览TIM2图像
	BYTE *pByImageData=(BYTE*)malloc(ti[index].cx*ti[index].cy*4);
	if (NULL==pByImageData)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		free(pBufOri);
		pBufOri=NULL;
		free(tim2);
		tim2=NULL;
		return false;
	}
	memset(pByImageData, 0, ti[index].cx*ti[index].cy*4);
	DecodeDXTn(hdc, pByImageData, pBufIm);
	free(pByImageData);
	pByImageData=NULL;

	fclose(fpIm);
	free(pBufOri);
	pBufOri=NULL;
	free(tim2);
	tim2=NULL;
	return true;
}

void FormatPSPDXTn(BYTE *pBuf, int flag, int iTexelNum)
{
	DWORD dwBuf[4];
	USHORT unsBuf;
	int i;

	for (i=0; i<iTexelNum; i++)
	{
		if (0==flag)
		{
			dwBuf[0]=*(DWORD*)(pBuf+i*8);
			*(DWORD*)(pBuf+i*8)=*(DWORD*)(pBuf+i*8+4);
			*(DWORD*)(pBuf+i*8+4)=dwBuf[0];
		} 
		else if(2==flag)
		{
			dwBuf[0]=*(DWORD*)(pBuf+i*16);
			dwBuf[1]=*(DWORD*)(pBuf+i*16+4);
			dwBuf[2]=*(DWORD*)(pBuf+i*16+8);
			dwBuf[3]=*(DWORD*)(pBuf+i*16+12);
			unsBuf=(USHORT)(dwBuf[0]>>16);
			dwBuf[0]=(dwBuf[0]<<16) | (dwBuf[1]>>16 & 0x0000ffff);
			dwBuf[1]=(dwBuf[1]<<16) | ((DWORD)unsBuf & 0x0000ffff);
			*(DWORD*)(pBuf+i*16)=dwBuf[3];
			*(DWORD*)(pBuf+i*16+4)=dwBuf[2];
			*(DWORD*)(pBuf+i*16+8)=dwBuf[1];
			*(DWORD*)(pBuf+i*16+12)=dwBuf[0];
		}
	}
}
int ConfirmType(LPCSTR szType)
{
	int i, count=0;
	char szBuf[8];

	strcpy_s(szBuf, 8, szType);
	for (i=0; i<7; i++)
	{
		if ((szBuf[i]<'0'||szBuf[i]>'9')&&(szBuf[i]<'A'||szBuf[i]>'Z')&&(szBuf[i]<'a'||szBuf[i]>'z'))
			break;
		count++;
	}
	return count;
}
bool TXP2FontTIM2(int iPalNo, char *szOutFileName)
{
	DWORD imageSize,fileSize;
	short palSize;
	BYTE *pBufEx, bySubNum;
	char szOldOutFile[MAX_PATH], szNewOutFile[MAX_PATH];
	int i, cyOri;

	bySubNum=ti[index].cy/MAX_HEIGHT;
	fclose(fpOut);
	sprintf_s(szOldOutFile, MAX_PATH, "%s\\%s", szCurrentPath, szOutFileName);
	if(!DeleteFile(szOldOutFile))
	{
		return false;
	}
	*strrchr(szOutFileName, '.')='\0';
	cyOri=ti[index].cy;//保存原始图像高度
	ti[index].cy=MAX_HEIGHT;//把图像高度设为能处理的最大值

	for (i=0; i<(int)bySubNum; i++)
	{
		sprintf_s(szNewOutFile, MAX_PATH, "%s\\%s_%d.tm2", szCurrentPath, szOutFileName, i);
		if (fopen_s(&fpOut,szNewOutFile,"wb")!=NULL)
		{
			return false;
		}

		//写tim2文件头
		fwrite("TIM2", 4, 1, fpOut);
		fputc(0x04, fpOut);
		fputc(0x00, fpOut);
		fputc(0x01, fpOut);
		fwrite("\0\0\0\0\0\0\0\0", 1, 9, fpOut);
		if(0x10==ti[index].clrb)
			imageSize=ti[index].cx*ti[index].cy/2;
		else if(0x100==ti[index].clrb)
			imageSize=ti[index].cx*ti[index].cy;
		palSize=ti[index].clrb*4;
		fileSize=imageSize+palSize+0x40;
		fileSize-=16;
		fwrite(&fileSize, 4, 1, fpOut);
		fwrite(&palSize, 2, 1, fpOut);
		fwrite("\0", 1, 2, fpOut);
		fwrite(&imageSize, 4, 1, fpOut);
		fputc(0x30, fpOut);
		fputc(0x00, fpOut);
		fwrite(&ti[index].clrb, 2, 1, fpOut);
		fputc(0x00, fpOut);
		fputc(0x01, fpOut);
		fputc(0x83, fpOut);
		if(0x10==ti[index].clrb)
			fputc(0x04, fpOut);
		else if(0x100==ti[index].clrb)
			fputc(0x05, fpOut);
		fwrite(&ti[index].cx, 2, 1, fpOut);
		fwrite(&ti[index].cy, 2, 1, fpOut);
		fwrite("\0\0\x40\x21\x02\0\0\0\x60\x02\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 24, fpOut);

		//写图像数据
		pBufEx=(BYTE*)malloc(imageSize);
		if (NULL==pBufEx)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			return false;
		}
		memset(pBufEx, 0, imageSize);
		fseek(fpIn, fi[index].Offset, 0);
		fseek(fpIn, ti[index].clrb*4*ti[index].palNum+16, 1);
		if (0x10==ti[index].clrb)
			fseek(fpIn, ti[index].cx*MAX_HEIGHT*i/2, 1);
		else if (0x100==ti[index].clrb)
			fseek(fpIn, ti[index].cx*MAX_HEIGHT*i, 1);
		if(!Export(&ti[index], NULL, NULL, pBufEx))
		{
			free(pBufEx);
			pBufEx=NULL;
			return false;
		}
		fwrite(pBufEx, 1, imageSize, fpOut);
		free(pBufEx);
		pBufEx=NULL;

		//写调色板数据
		fseek(fpIn, fi[index].Offset+16+iPalNo*palSize, 0);
		pBufEx=(BYTE*)malloc(palSize);
		if (NULL==pBufEx)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			return false;
		}
		memset(pBufEx, 0, palSize);
		fread(pBufEx, 1, palSize, fpIn);
		fwrite(pBufEx, 1, palSize, fpOut);
		free(pBufEx);
		pBufEx=NULL;

		fclose(fpOut);
	}

	//写最后一个子字库tim2文件
	if (cyOri%MAX_HEIGHT>0)
	{
		sprintf_s(szNewOutFile, MAX_PATH, "%s\\%s_%d.tm2", szCurrentPath, szOutFileName, bySubNum);
		if (fopen_s(&fpOut,szNewOutFile,"wb")!=NULL)
		{
			return false;
		}

		ti[index].cy=cyOri%MAX_HEIGHT;
		//写tim2文件头
		fwrite("TIM2", 4, 1, fpOut);
		fputc(0x04, fpOut);
		fputc(0x00, fpOut);
		fputc(0x01, fpOut);
		fwrite("\0\0\0\0\0\0\0\0", 1, 9, fpOut);
		if(0x10==ti[index].clrb)
			imageSize=ti[index].cx*ti[index].cy/2;
		else if(0x100==ti[index].clrb)
			imageSize=ti[index].cx*ti[index].cy;
		palSize=ti[index].clrb*4;
		fileSize=imageSize+palSize+0x40;
		fileSize-=16;
		fwrite(&fileSize, 4, 1, fpOut);
		fwrite(&palSize, 2, 1, fpOut);
		fwrite("\0", 1, 2, fpOut);
		fwrite(&imageSize, 4, 1, fpOut);
		fputc(0x30, fpOut);
		fputc(0x00, fpOut);
		fwrite(&ti[index].clrb, 2, 1, fpOut);
		fputc(0x00, fpOut);
		fputc(0x01, fpOut);
		fputc(0x83, fpOut);
		if(0x10==ti[index].clrb)
			fputc(0x04, fpOut);
		else if(0x100==ti[index].clrb)
			fputc(0x05, fpOut);
		fwrite(&ti[index].cx, 2, 1, fpOut);
		fwrite(&ti[index].cy, 2, 1, fpOut);
		fwrite("\0\0\x40\x21\x02\0\0\0\x60\x02\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 24, fpOut);

		//写图像数据
		pBufEx=(BYTE*)malloc(imageSize);
		if (NULL==pBufEx)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			return false;
		}
		memset(pBufEx, 0, imageSize);
		fseek(fpIn, fi[index].Offset, 0);
		fseek(fpIn, ti[index].clrb*4*ti[index].palNum+16, 1);
		if (0x10==ti[index].clrb)
			fseek(fpIn, ti[index].cx*MAX_HEIGHT*(bySubNum)/2, 1);
		else if (0x100==ti[index].clrb)
			fseek(fpIn, ti[index].cx*MAX_HEIGHT*(bySubNum), 1);
		if(!Export(&ti[index], NULL, NULL, pBufEx))
		{
			free(pBufEx);
			pBufEx=NULL;
			return false;
		}
		fwrite(pBufEx, 1, imageSize, fpOut);
		free(pBufEx);
		pBufEx=NULL;

		//写调色板数据
		fseek(fpIn, fi[index].Offset+16+iPalNo*palSize, 0);
		pBufEx=(BYTE*)malloc(palSize);
		if (NULL==pBufEx)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			return false;
		}
		memset(pBufEx, 0, palSize);
		fread(pBufEx, 1, palSize, fpIn);
		fwrite(pBufEx, 1, palSize, fpOut);
		free(pBufEx);
		pBufEx=NULL;

		fclose(fpOut);
	}
	return true;
}
bool FontTIM22TXP(HDC hdc)
{
	BYTE p, *pBufOri;
	char szFlag[5], szFileImPath[MAX_PATH], szFileIm[MAX_PATH], szFileImName[MAX_PATH], *szImNo;
	int iPos=0, iPosSum=0, iImNo;
	DWORD imageSize;

	GetFirstFile(szFontTIM2File, szFileIm);
	//获取路径
	iPos=strrchr(szFileIm, '\\')-szFileIm;
	szFileIm[iPos]='\0';
	strcpy_s(szFileImPath, MAX_PATH, szFileIm);
	szFileIm[iPos]='\\';
	//获取子字库文件号
	szImNo=strrchr(szFileIm, '_')+1;
	if(1==(int)szImNo)
	{
		MessageBox(NULL, "The import file name is wrong!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
		bImFile=false;
		return false;
	}
	iImNo=atoi(szImNo);
	//打开第一个子字库文件
	if (fopen_s(&fpIm, szFileIm, "rb")!=NULL)
	{
		MessageBox(NULL, "Can't open file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
		bImFile=false;
		return false;
	}
	//处理第一个子字库文件
	tim2=(TIM2*)malloc(sizeof(TIM2));
	if (NULL==tim2)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		return false;
	}
	memset(tim2, 0, sizeof(TIM2));
	fread(szFlag, 4, 1, fpIm);
	szFlag[4]='\0';
	if(strcmp("TIM2", szFlag))
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fseek(fpIm, 0x10L, 0);
	fread(&tim2->fileSize, 4, 1, fpIm);
	tim2->fileSize+=16;
	fread(&tim2->palSize, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->imageSize, 4, 1, fpIm);
	fseek(fpIm, 2L, 1);
	fread(&tim2->clrb, 2, 1, fpIm);
	fseek(fpIm, 2L, 1);
	p=fgetc(fpIm);
	if(0x83!=p && 0x03!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	p=fgetc(fpIm);
	if(0x04!=p && 0x05!=p)
	{
		free(tim2);
		tim2=NULL;
		return false;
	}
	fread(&tim2->cx, 2, 1, fpIm);
	fread(&tim2->cy, 2, 1, fpIm);
	fseek(fpIm, 0x40L+tim2->imageSize, 0);
	fpos=ftell(fpIm);

	for(i=0;i<tim2->palSize/4;i++)
	{
		pal[i].r=fgetc(fpIm);
		pal[i].g=fgetc(fpIm);
		pal[i].b=fgetc(fpIm);
		pal[i].a=fgetc(fpIm);
	}
	if(0x10==ti[index].clrb)
		imageSize=tim2->cx*MAX_HEIGHT/2;
	else if(0x100==ti[index].clrb)
		imageSize=tim2->cx*MAX_HEIGHT;

	fseek(fpIm, 0x40L, 0);
	pBufOri=(BYTE*)malloc(tim2->imageSize);
	if (NULL==pBufOri)
	{
		MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
		fclose(fpIm);
		free(tim2);
		tim2=NULL;
		return false;
	}
	memset(pBufOri, 0, tim2->imageSize);
	//读入TIM2图像数据到pBufOri缓冲区中
	fread(pBufOri, 1, tim2->imageSize, fpIm);
	//从pBufOri缓冲区中导入TIM2图像数据到pBufIm缓冲区中
	if(!ImportFontTIM2(tim2, pBufOri, iImNo*imageSize))
	{
		fclose(fpIm);
		free(pBufOri);
		pBufOri=NULL;
		free(tim2);
		tim2=NULL;
		return false;
	}
	fclose(fpIm);
	free(pBufOri);
	pBufOri=NULL;

	//处理接下来的子字库文件
	iPos=0;
	while(iPos>=0)
	{
		iPos=GetNextFileName(szFontTIM2File+iPosSum, szFileImName);
		iPosSum+=iPos;
		sprintf_s(szFileIm, MAX_PATH, "%s\\%s", szFileImPath, szFileImName);
		//获取子字库文件号
		szImNo=strrchr(szFileIm, '_')+1;
		if(1==(int)szImNo)
		{
			MessageBox(NULL, "The import file name is wrong!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
			bImFile=false;
			break;
		}
		iImNo=atoi(szImNo);

		if (fopen_s(&fpIm, szFileIm, "rb")!=NULL)
		{
			MessageBox(NULL, "Can't open file!", szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
			bImFile=false;
			return false;
		}
		memset(tim2, 0, sizeof(TIM2));
		fread(szFlag, 4, 1, fpIm);
		szFlag[4]='\0';
		if(strcmp("TIM2", szFlag))
		{
			free(tim2);
			tim2=NULL;
			return false;
		}
		fseek(fpIm, 0x10L, 0);
		fread(&tim2->fileSize, 4, 1, fpIm);
		tim2->fileSize+=16;
		fread(&tim2->palSize, 2, 1, fpIm);
		fseek(fpIm, 2L, 1);
		fread(&tim2->imageSize, 4, 1, fpIm);
		fseek(fpIm, 2L, 1);
		fread(&tim2->clrb, 2, 1, fpIm);
		fseek(fpIm, 2L, 1);
		p=fgetc(fpIm);
		if(0x83!=p && 0x03!=p)
		{
			free(tim2);
			tim2=NULL;
			return false;
		}
		p=fgetc(fpIm);
		if(0x04!=p && 0x05!=p)
		{
			free(tim2);
			tim2=NULL;
			return false;
		}
		fread(&tim2->cx, 2, 1, fpIm);
		fread(&tim2->cy, 2, 1, fpIm);

		fseek(fpIm, 0x40L, 0);
		pBufOri=(BYTE*)malloc(tim2->imageSize);
		if (NULL==pBufOri)
		{
			MessageBox(NULL, "There is insufficient memory available!", szTitle, MB_OK | MB_ICONERROR);
			fclose(fpIm);
			free(tim2);
			tim2=NULL;
			return false;
		}
		memset(pBufOri, 0, tim2->imageSize);
		//读入TIM2图像数据到pBufOri缓冲区中
		fread(pBufOri, 1, tim2->imageSize, fpIm);
		//从pBufOri缓冲区中导入TIM2图像数据到pBufIm缓冲区中
		if(!ImportFontTIM2(tim2, pBufOri, iImNo*imageSize))
		{
			fclose(fpIm);
			free(pBufOri);
			pBufOri=NULL;
			free(tim2);
			tim2=NULL;
			return false;
		}
		fclose(fpIm);
		free(pBufOri);
		pBufOri=NULL;
	}

	free(tim2);
	tim2=NULL;
	//预览TIM2图像
	Draw(hdc, &ti[index], NULL, NULL, 0, pBufIm);

	return true;
}

bool GetFirstFile(char *szFile, char *szFirstFile)
{
	char *c;

	c=strchr(szFile, ';');
	if(NULL==c)
		return false;
	*c='\0';
	//*strchr(szFile, ';')='\0';
	strcpy_s(szFirstFile, MAX_PATH, szFile);
	return true;
}

int GetNextFileName(char *szFile, char *szNextFileName)
{
	int i=0;
	char *c;

	while(*(szFile+i))
		i++;
	i++;
	c=strchr(szFile+i, ';');
	if(NULL==c)
	{
		strcpy_s(szNextFileName, MAX_PATH, szFile+i);
		return -1;
	}
	*c='\0';
	//*strchr(szFile+i, ';')='\0';
	strcpy_s(szNextFileName, MAX_PATH, szFile+i);
	return i;
}

bool ImportFontTIM2(TIM2 *tm2, BYTE *pBuf, int iStart)
{
	BYTE p;
	int pos,cx=0,cy=0,xTile=0,yTile=0;

	int cxLoc;
	int cyLoc;
	int bppLoc;
	BYTE flagLoc;

	cxLoc=tm2->cx;
	cyLoc=tm2->cy;
	bppLoc=tm2->clrb;
	flagLoc=ti[index].flag;

	for(i=0;i<cxLoc*cyLoc;i++)
	{
		if(0x10==bppLoc || 0x04==bppLoc)//16色
		{
			if(0x00==flagLoc)//16色无tile结构的txp图片
			{
				if(0==i%2)
					p=pBuf[i/2];
				else
					pBufIm[i/2+iStart]=p;
			}
			else if(0x01==flagLoc)//16色有tile结构的txp图片
			{
				tileWidth=32;
				tileHeight=8;

				if(0==i%2)
				{
					cx=i%tileWidth;
					cy=(i/tileWidth)%tileHeight;
					xTile=(i/(tileWidth*tileHeight))%(cxLoc/tileWidth);
					yTile=(i/(tileWidth*tileHeight))/(cxLoc/tileWidth);
					pos=(cy+yTile*tileHeight)*cxLoc+(cx+xTile*tileWidth);
					pBufIm[i/2+iStart]=pBuf[pos/2];
				}
			}
			else
				return false;
		}
		else if(0x100==bppLoc || 0x08==bppLoc)//256色
		{
			if(0x00==flagLoc)//256色无tile结构的txp图片
			{
				p=pBuf[i];
				pBufIm[i+iStart]=p;
			}
			else if(0x01==flagLoc)//256色有tile结构的txp图片
			{
				tileWidth=16;
				tileHeight=8;

				cx=i%tileWidth;
				cy=(i/tileWidth)%tileHeight;
				xTile=(i/(tileWidth*tileHeight))%(cxLoc/tileWidth);
				yTile=(i/(tileWidth*tileHeight))/(cxLoc/tileWidth);
				pos=(cy+yTile*tileHeight)*cxLoc+(cx+xTile*tileWidth);
				pBufIm[i+iStart]=pBuf[pos];
			}
			else
				return false;
		}
	}
	return true;
}
