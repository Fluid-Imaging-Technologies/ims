/*
 * $Id: LogWindow.h,v 1.1.1.1 2008/10/10 11:05:48 scott Exp $
 *
 */

#ifndef LOG_WINDOW_H
#define LOG_WINDOW_H

#include "version.h"

class LogWindow
{
public:
	LogWindow(HWND hWndParent);
	~LogWindow();

	static bool registerWinClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static char WinClass[32];
	static bool Registered;

	HWND getWindowHandle();
	void showWindow();
	void closeWindow();

	void addEvent(const char *op, const char *s);
	void clear();

protected:
	void wmCreate(HWND hWnd);
	void wmDestroy();
	void wmPaint(HWND hWnd);
	void wmSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void wmCommand(WPARAM wParam, LPARAM lParam);
	
	bool growTextBuff(int size_required);
	void save(bool prompt = true);
		
private:
	HWND _hWnd;
	HWND _hWndParent;
	HWND _hWndEdit;
	HFONT _hFont;
	int _listCount;
	char _saveFile[MAX_PATH];	
	char *_text;
	int _textBufferSize;
};


#endif // ifndef LOG_WINDOW_H
