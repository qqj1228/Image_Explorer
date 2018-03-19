#pragma once

#include "resource.h"
#include "commctrl.h"
#include "Commdlg.h"
#include "stdio.h"

//加入squish库的支持
#include "squish.h"
#pragma comment(lib, "squish.lib")

#define IDC_LISTVIEW	40004
#define ID_STATUSBAR	50000
#define MAX_LOADSTRING	100
#define FILENUM			0x602
#define MAX_HEIGHT		0x1760
#define MAX_MULTIFILE	2500
 
typedef struct tagFILEINFO
{
	char szName[28];
	DWORD Offset;
	char szOffset[11];
	char szType[8];
	DWORD Size;
	DWORD Unknow;
}FILEINFO;

typedef struct tagTXPINFO//兼做FTX文件信息用
{
	int cx;
	int cy;
	int clrb;
	int palNum;
	int flag;
}TXPINFO;

typedef struct tagPAL
{
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
}PAL;

typedef struct tagTIM2
{
	int cx;
	int cy;
	int clrb;
	short palSize;
	DWORD fileSize;
	DWORD imageSize;
}TIM2;

typedef struct tagDATINFO//兼做GM3/MTXP文件信息结构用
{
	char szParentName[28];
	char szName[3];
	DWORD Offset;
	char szOffset[11];
	char szType[8];
	DWORD Size;
}DATINFO;

typedef struct tagSUBINFO
{
	DWORD fileSize;
	int palNum;
	int imageNum;
	DWORD PalInfoOffset;
	DWORD ImageInfoOffset;
}SUBINFO;

typedef struct tagSUBPALINFO
{
	DWORD PalOffset;
	DWORD PalSize;
	int perPalNum;
	BYTE bpp;
}SUBPALINFO;

typedef struct tagSUBIMAGEINFO
{
	DWORD ImageOffset;
	int cx;
	int cy;
	BYTE bpp;
	BYTE flag;
	int palIndex;
}SUBIMAGEINFO;

typedef struct tagSUBIMAGEFILEINFO
{
	char szParentName[28];
	char szName[6];
	DWORD Offset;
	char szOffset[11];
	char szType[8];
}SUBIMAGEFILEINFO;

typedef struct tagGM3IMAGEINFO//兼做MTXP图像信息结构用
{
	DWORD fileSize;
	int cx;
	int cy;
	int clrb;
	int palNum;
	int flag;
}GM3IMAGEINFO;

typedef struct tagSORTDATA
{
	int col;
	int preCol;
	HWND hWndListView;
	bool bAsc;
}SORTDATA;

// Forward declarations of functions included in this code module:
void				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK		CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ExplorerProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Ex_ImportDlgProc(HWND, UINT, WPARAM, LPARAM);
COLORREF			Alpha(BYTE r, BYTE g, BYTE b, BYTE a);
int					GetOnePixel(int clrb, BYTE *pBuf, int sn);
bool				Draw(HDC hdc, TXPINFO *ti, SUBIMAGEINFO *sii, GM3IMAGEINFO *gii, int iPalNo, BYTE *pBuf);
bool				DrawImage(HDC hdc, int iPalNo, int iPalNoSum);
bool				Export(TXPINFO *ti, SUBIMAGEINFO *sii, GM3IMAGEINFO *gii, BYTE *pBuf);
bool				TXP2TIM2(int iPalNo, char *szOutFileName);
bool				DAT2TIM2(int iPalNo);
bool				GM32TIM2(int iPalNo);
bool				Import(TXPINFO *ti, SUBIMAGEINFO *sii, GM3IMAGEINFO *gii, BYTE *pBuf);
bool				TIM22TXP(HDC hdc);
bool				TIM22DAT(HDC hdc);
bool				TIM22GM3(HDC hdc);
bool				DecodeDXTn(HDC hdc, BYTE *pByBufOut, BYTE *pByBufIm);
bool				DecodeOneTexel(HDC hdc, int x, int y, BYTE *pBufIn, BYTE *pBufOut, int flag);
inline DWORD		DwReverse(DWORD Input);
inline BYTE			R565( USHORT clr );//  获得 R5G6B5 红色分量
inline BYTE			G565( USHORT clr );//  获得 R5G6B5 绿色分量
inline BYTE			B565( USHORT clr );//  获得 R5G6B5 蓝色分量
bool				TIM22DXTn(HDC hdc);
void				FormatPSPDXTn(BYTE *pBuf, int flag, int iTexelNum);
int					ConfirmType(LPCSTR szType);
bool				TXP2FontTIM2(int iPalNo, char *szOutFileName);
bool				FontTIM22TXP(HDC hdc);
bool				GetFirstFile(char *szFile, char *szFirstFile);
int					GetNextFileName(char *szFile, char *szNextFileName);
bool				ImportFontTIM2(TIM2 *tm2, BYTE *pBuf, int iStart);