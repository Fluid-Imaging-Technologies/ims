/*
 * $Id: utility.cpp,v 1.2 2009/04/06 20:09:41 scott Exp $
 *
 */

#include "utility.h"
#include <shlobj.h>

char gIniFilename[MAX_PATH];

/*
  =======================================================================================  
  =======================================================================================
*/
double computeCircularity(double area, double perimeter)
{
	if (perimeter != 0.0) {
		return (4.0 * PI * area) / (perimeter * perimeter);
	}

	return 0.0;
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool loadWindowRect(const char *windowName, RECT *rect, RECT *rectDefault)
{
	RECT desktop;
	int oldw, oldh;

	if (!loadIniFilename()) {
		return false;
	}
	
	if (!windowName || !*windowName || !rect || !rectDefault) {
		return false;
	}

	rect->left = GetPrivateProfileInt(windowName, "Left", rectDefault->left, gIniFilename);
	rect->top = GetPrivateProfileInt(windowName, "Top", rectDefault->top, gIniFilename);
	rect->right = GetPrivateProfileInt(windowName, "Right", rectDefault->right, gIniFilename);
	rect->bottom = GetPrivateProfileInt(windowName, "Bottom", rectDefault->bottom, gIniFilename);

	oldw = rect->right - rect->left;
	oldh = rect->bottom - rect->top;

	// make sure the window shows up on the current display
	memset(&desktop, 0, sizeof(RECT));

	if (!GetClientRect(GetDesktopWindow(), &desktop)) {
		desktop.right = GetSystemMetrics(SM_CXMAXIMIZED);
		desktop.bottom = GetSystemMetrics(SM_CYMAXIMIZED);
	}

	if (desktop.right < 640) {
		desktop.right = 640;
	}

	if (desktop.bottom < 480) {
		desktop.bottom = 480;
	}

	if ( rect->left + 20 > desktop.right) {
		rect->left = desktop.right / 2;
		rect->right = rect->left + oldw;
	}

	if (rect->top + 20 > desktop.bottom) {
		rect->top = desktop.bottom / 2;
		rect->bottom = rect->top + oldh;
	}
	
	if (rect->right - rect->left < 40) {
		rect->right = rect->left + 40;
	}

	if (rect->bottom - rect->top < 40) {
		rect->bottom = rect->top + 40;
	}

	return true;
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool saveWindowRect(const char *windowName, HWND hWnd)
{
	char temp[32];
	RECT rect;

	if (IsIconic(hWnd) || IsZoomed(hWnd)) {
		return false;
	}

	if (!loadIniFilename()) {
		return false;
	}
	
	if (!windowName || !*windowName) {
		return false;
	}

	memset(&rect, 0, sizeof(rect));
	memset(temp, 0, sizeof(temp));

	if (!GetWindowRect(hWnd, &rect)) {
		return false;
	}

	sprintf_s(temp, sizeof(temp), "%d", rect.left);
	WritePrivateProfileString(windowName, "Left", temp, gIniFilename);
	
	sprintf_s(temp, sizeof(temp), "%d", rect.top);
	WritePrivateProfileString(windowName, "Top", temp, gIniFilename);
	
	sprintf_s(temp, sizeof(temp), "%d", rect.right);
	WritePrivateProfileString(windowName, "Right", temp, gIniFilename);
	
	sprintf_s(temp, sizeof(temp), "%d", rect.bottom);
	WritePrivateProfileString(windowName, "Bottom", temp, gIniFilename);

	return true;
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool loadIniFilename()
{
	char myDocsDir[MAX_PATH];

	if (strlen(gIniFilename) > 0) {
		return true;
	}

	memset(myDocsDir, 0, sizeof(myDocsDir));

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_DEFAULT, myDocsDir))) {
		strncat_s(myDocsDir, sizeof(myDocsDir), "\\ims\\", _TRUNCATE);

		if (!directory_exists(myDocsDir)) {
			// create it
			if (!CreateDirectory(myDocsDir, NULL)) {
				return false;
			}
		}
	}

	strncpy_s(gIniFilename, sizeof(gIniFilename), myDocsDir, _TRUNCATE);
	strncat_s(gIniFilename, sizeof(gIniFilename), "ims.ini", _TRUNCATE);

	return true;
}

/*
  =======================================================================================
  =======================================================================================
*/
bool directory_exists(const char *dirname)
{
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;

	if (dirname && *dirname) {
		if (!GetFileAttributesEx(dirname, GetFileExInfoStandard, &fileInfo)) {
			return false;
		}

		if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			return true;
		}
	}

	return false;
}

/*
  =======================================================================================
  =======================================================================================
*/
bool file_exists(const char *filename)
{
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;

	if (filename && *filename) {
		if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &fileInfo)) {

//#if defined (_DEBUG)
//			ShowLastError("file_exists: GetFileAttributesEx");
//#endif

			return false;
		}

		return true;
	}

	return false;
}

/*
  =======================================================================================
  Like the standard C runtime strdup except we use new() instead of malloc()
  =======================================================================================
*/
char *newStrDup(const char *src)
{
	if (!src) {
		return NULL;
	}

	int len = 1 + strlen(src);

	char *dst = new char[len];

	if (dst) {
		strncpy_s(dst, len, src, _TRUNCATE);
	}

	return dst;
}

/*
  =======================================================================================
  =======================================================================================
*/
void showLastError(const char *lpszFunction)
{
	LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    DWORD dw = GetLastError(); 

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dw,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,
					0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
					(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    
	wsprintf((LPTSTR)lpDisplayBuf, 
				TEXT("%s failed with error %d: %s"), 
				lpszFunction, dw, lpMsgBuf); 
    
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
