/*
 * $Id: LogWindow.cpp,v 1.1.1.1 2008/10/10 11:05:48 scott Exp $
 *
 */

#include "LogWindow.h"
#include "utility.h"
#include "resource.h"

char LogWindow::WinClass[32] = "LogWindowClass";
bool LogWindow::Registered = false;


/*
  =======================================================================================
  =======================================================================================
*/
LogWindow::LogWindow(HWND hWndParent)
{
	_hWnd = 0;
	_hWndParent = hWndParent;
	_hFont = 0;
	_hWndEdit = 0;
	_listCount = 0;
	memset(_saveFile, 0, sizeof(_saveFile));

	if (!LogWindow::Registered) {
		registerWinClass(GetModuleHandle(NULL));
	}

	_text = NULL;
	_textBufferSize = 0;
}

/*
  =======================================================================================
  =======================================================================================
*/
LogWindow::~LogWindow()
{
	closeWindow();

	if (_text) {
		delete [] _text;
		_text = 0;
	}

	_textBufferSize = 0;
}

/*
  =======================================================================================
  =======================================================================================
*/
HWND LogWindow::getWindowHandle()
{
	return _hWnd;
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::showWindow()
{
	RECT rect, rectDefault;

	if (_hWnd) {
		SetWindowPos(_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else {
		rectDefault.left = 50;
		rectDefault.top = 50;
		rectDefault.right = rectDefault.left + 400;
		rectDefault.bottom = rectDefault.top + 100; 

		if (!loadWindowRect("LogWindow", &rect, &rectDefault)) {
			rect = rectDefault;
		}

		_hWnd = CreateWindow(LogWindow::WinClass,
							"Imalg Log",	    
							WS_OVERLAPPEDWINDOW,						
							rect.left,	  
							rect.top,	
							rect.right - rect.left,
							rect.bottom - rect.top,
							_hWndParent,   
							NULL,
							GetModuleHandle(NULL),	  
							this);  

		if (_hWnd) {
			ShowWindow(_hWnd, SW_SHOW);
			UpdateWindow(_hWnd);						
		}
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::closeWindow()
{
	if (_hWnd) {
		DestroyWindow(_hWnd);
		_hWnd = 0;
	}

	if (_hFont) {
		DeleteObject(_hFont);
		_hFont = 0;
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::addEvent(const char *op, const char *s)
{
	char buff[512];
	int n;

	if (s && *s) {
		_listCount++;

		if (_listCount > 100) {
			_listCount = 1;

			if (_text) {
				memset(_text, 0, _textBufferSize);
			}
		}

		sprintf_s(buff, sizeof(buff), "[%d] ", _listCount);

		if (op && *op) {
			strncat_s(buff, sizeof(buff), op, _TRUNCATE);
			strncat_s(buff, sizeof(buff), " : ", _TRUNCATE);
		}
		else {
			memset(buff, 0, sizeof(buff));
		}

		strncat_s(buff, sizeof(buff), s, _TRUNCATE);
		strncat_s(buff, sizeof(buff), "\r\n", _TRUNCATE);
		
		if (_text) {
			n = strlen(_text);
		}
		else {
			n = 0;
		}

		n += 8 + strlen(buff);

		if (n > _textBufferSize) {
			if (!growTextBuff(n)) {
				return;
			}
		}

		strncat_s(_text, _textBufferSize, buff, _TRUNCATE);
		SetWindowText(_hWndEdit, _text);
		InvalidateRect(_hWnd, NULL, TRUE);
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
bool LogWindow::growTextBuff(int size_required)
{
	char *new_text;

	size_required += 0x0fff;
	size_required &= 0x1000;

	new_text = new char[size_required];

	if (!new_text) {
		return false;
	}

	memset(new_text, 0, size_required);

	if (_text) {
		if (strlen(_text) > 0) {
			strncpy_s(new_text, size_required, _text, _TRUNCATE);
		}

		delete [] _text;
	}			

	_text = new_text;
	_textBufferSize = size_required;

	return true;
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::clear()
{
	_listCount = 0;

	if (_text) {
		memset(_text, 0, _textBufferSize);
	}

	SetWindowText(_hWndEdit, _text);

	InvalidateRect(_hWnd, NULL, TRUE);
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::wmSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	SetWindowPos(_hWndEdit, 0, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOMOVE);
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::wmCreate(HWND hWnd)
{
	RECT r;

	GetClientRect(hWnd, &r);
	
	_hFont = CreateFont(-11, 0, 0, 0, 400,	
					   0, 0, 0, 0, OUT_TT_PRECIS, 0, CLEARTYPE_NATURAL_QUALITY, 0, "Tahoma"); 	


	_hWndEdit = CreateWindow("EDIT", 0, 
							WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 
							0, 0, r.right - r.left, r.bottom - r.top,
							hWnd, 0, GetModuleHandle(NULL), 0);

	if (_hWndEdit) {
		SendMessage(_hWndEdit, WM_SETFONT, (WPARAM) _hFont, 0);
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::wmDestroy()
{
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::wmPaint(HWND hWnd)
{
	PAINTSTRUCT ps;

	HDC hdc = BeginPaint(hWnd, &ps);

	EndPaint(hWnd, &ps);
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::wmCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case ID_LOG_SAVE:
		save(false);
		break;

	case ID_LOG_SAVE_AS:
		save(true);
		break;

	case ID_LOG_CLOSE:
		closeWindow();
		break;
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void LogWindow::save(bool prompt)
{
	OPENFILENAME ofn;
	HANDLE fh;
	int n;
	DWORD written;

	if (!_text) {
		return;
	}

	n = strlen(_text);

	if (n == 0) {
		MessageBox(_hWnd, "Nothing to save.", "Log Save", MB_ICONINFORMATION | MB_OK);
		return;
	}

	if (prompt || strlen(_saveFile) == 0) {
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = _hWnd;
		ofn.hInstance = GetModuleHandle(NULL);
		ofn.lpstrFilter = "Text files (*.txt)\0*.txt\0";
		ofn.lpstrFile = _saveFile;
		ofn.nMaxFile = sizeof(_saveFile) - 1;
		ofn.lpstrTitle = "Imalg Log";
		ofn.lpstrDefExt = "txt";

		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		if (!GetSaveFileName(&ofn)) {
			return;
		}
	}

	if (strlen(_saveFile) < 0) {
		return;
	}

	fh = CreateFile(_saveFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (!fh) {
		return;
	}

	WriteFile(fh, _text, n, &written, NULL);

	CloseHandle(fh);
}

/*
  =======================================================================================
  =======================================================================================
*/
LRESULT CALLBACK 
LogWindow::WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LogWindow *win;
	LPCREATESTRUCT cs;

	switch (msg) 
	{
	case WM_PAINT:
		win = (LogWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmPaint(hWnd);
		}

		break;
	
	case WM_SIZE:
		win = (LogWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmSize(hWnd, wParam, lParam);
		}

		break;

	case WM_COMMAND:
		win = (LogWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmCommand(wParam, lParam);
		}

		break;			  

	case WM_CLOSE:		
		win = (LogWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->closeWindow();
		}

		break;

	case WM_CREATE:
		win = (LogWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmCreate(hWnd);
		}

		break;

	case WM_DESTROY:
		saveWindowRect("LogWindow", hWnd); 

		win = (LogWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {			
			SetWindowLong(hWnd, GWL_USERDATA, 0);
		}

		break;

	case WM_NCCREATE:
		cs = (LPCREATESTRUCT) lParam;
		SetWindowLong(hWnd, GWL_USERDATA, (long) cs->lpCreateParams);
		// deliberate fall through

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}						   

	return 0;

}

/*
  =======================================================================================
  static function
  =======================================================================================
*/
bool LogWindow::registerWinClass(HINSTANCE hInstance)
{
	WNDCLASS wc;
	
	memset(&wc, 0, sizeof(wc));

	wc.lpfnWndProc = LogWindow::WinProc;																
	wc.cbWndExtra = sizeof(LogWindow *);							
	wc.hInstance = hInstance; 		
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMALG)); 
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);   			
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = "LOG_WINDOW_MENU";	
	wc.lpszClassName = LogWindow::WinClass;	
	
	if (!RegisterClass(&wc)) {
		return false;
	}

	LogWindow::Registered = true;
	
	return LogWindow::Registered;
}
