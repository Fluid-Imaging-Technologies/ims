/*
 * $Id: OverlayWindow.cpp,v 1.5 2009/04/30 19:01:05 scott Exp $
 *
 */

#include "OverlayWindow.h"
#include "utility.h"
#include "resource.h"


#define TOOL_BAR_HEIGHT 33

#define TB_PIXELS 0
#define TB_EDGES 1
#define TB_RECTS 2
#define TB_IMAGE 3
#define TB_ZOOM_IN 4
#define TB_ZOOM_OUT 5
#define NUM_TOOLBAR_BUTTONS 6

#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 768

#define ZOOM_FACTOR_100_PERCENT 1
#define ZOOM_FACTOR_80_PERCENT 2
#define ZOOM_FACTOR_50_PERCENT 3

char OverlayWindow::WinClass[32] = "OverlayWindowClass";
char OverlayWindow::MilWinClass[32] = "MilWindowClass";

bool OverlayWindow::Registered = false;


/*
  =======================================================================================
  =======================================================================================
*/
OverlayWindow::OverlayWindow(HWND hWndParent)
{
	_hWnd = 0;
	_hWndParent = hWndParent;
	//_hFont = 0;
	_hWndMil = 0;

	_hWndTBButtons = 0;
	_grey = GetSysColor(COLOR_BTNFACE);	

	_zoomFactor = ZOOM_FACTOR_50_PERCENT;

	_skipPaintMessage = false;
	_showBoundingRects = false;
	_showOverlayPixels = true;
	_showEdges = false;
	_showImages = true;

	_overlayColor = RGB(240, 0, 0);
	_boundingRectColor = RGB(0, 240, 0);
	_edgeColor = RGB(255, 255, 255);
	_penWidth = 1;

	initializeMilVars();

	setPixels();

	_selectedBlob = -1;
	_maxBlobs = 0;
	_numBlobs = 0;
	_blobLeft = NULL;
	_blobRight = NULL;
	_blobTop = NULL;
	_blobBottom = NULL;
	_blobNumChainedPixels = NULL;
	_blobLabels = NULL;
	_pixelChains = NULL;
	_blobFeretMaxAngle = NULL;
	_blobFeretMaxDiameter = NULL;
	_blobPerimeter = NULL;
	_blobConvexPerimeter = NULL;
	_blobCompactness = NULL;
	_blobArea = NULL;

	if (!OverlayWindow::Registered) {
		registerWinClass(GetModuleHandle(NULL));
	}

	loadSettings();
}

/*
  =======================================================================================
  =======================================================================================
*/
OverlayWindow::~OverlayWindow()
{
	closeWindow();

	if (_hWndTBButtons) {
		delete [] _hWndTBButtons;
		_hWndTBButtons = NULL;
	}

	if (_milBlobResult) {
		MblobFree(_milBlobResult);
		_milBlobResult = 0;
	}

	if (_milFeatureList) {
		MblobFree(_milFeatureList);
		_milFeatureList = 0;
	}

	if (_milDisplay) {
		MdispFree(_milDisplay);
		_milDisplay = 0;
	}

	if (_binImage) {
		MbufFree(_binImage);
		_binImage = 0;
	}

	if (_srcImage) {
		MbufFree(_srcImage);
		_srcImage = 0;
	}

	freeBlobBuffers();
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::initializeMilVars()
{
	_srcImage = MbufAllocColor(M_DEFAULT_HOST, 3, 1024, 768, 8 + M_UNSIGNED,
								M_IMAGE | M_DISP | M_PROC | M_RGB24, M_NULL);

	MbufClear(_srcImage, 0.0);

	_binImage = MbufAlloc2d(M_DEFAULT_HOST, 1024, 768, 8 + M_UNSIGNED,
								M_IMAGE | M_DISP | M_PROC, M_NULL);

	MbufClear(_binImage, 0.0);

	_milDisplay = MdispAlloc(M_DEFAULT_HOST, M_DEFAULT, M_DEF_DISPLAY_FORMAT, M_DEFAULT, M_NULL);

	_milFeatureList = MblobAllocFeatureList(M_DEFAULT_HOST, M_NULL);

	MblobSelectFeature(_milFeatureList, M_BOX_X_MIN);
	MblobSelectFeature(_milFeatureList, M_BOX_Y_MIN);
	MblobSelectFeature(_milFeatureList, M_BOX_X_MAX);
	MblobSelectFeature(_milFeatureList, M_BOX_Y_MAX);

	MblobSelectFeature(_milFeatureList, M_NUMBER_OF_CHAINED_PIXELS);
	MblobSelectFeature(_milFeatureList, M_CHAIN_INDEX);
	MblobSelectFeature(_milFeatureList, M_CHAIN_X);
	MblobSelectFeature(_milFeatureList, M_CHAIN_Y);
	MblobSelectFeature(_milFeatureList, M_FERET_MAX_ANGLE);
	MblobSelectFeature(_milFeatureList, M_FERET_MAX_DIAMETER);
	MblobSelectFeature(_milFeatureList, M_PERIMETER);
	MblobSelectFeature(_milFeatureList, M_CONVEX_PERIMETER);
	MblobSelectFeature(_milFeatureList, M_COMPACTNESS);
	MblobSelectFeature(_milFeatureList, M_AREA);

	_milBlobResult = MblobAllocResult(M_DEFAULT_HOST, M_NULL);

	//MblobControl(_milBlobResult, M_IDENTIFIER_TYPE, M_GRAYSCALE);
	MblobControl(_milBlobResult, M_IDENTIFIER_TYPE, M_BINARY);

	MblobControl(_milBlobResult, M_LATTICE, M_8_CONNECTED);
	//MblobControl(_milBlobResult, M_LATTICE, M_4_CONNECTED);

	MblobControl(_milBlobResult, M_NUMBER_OF_FERETS, 36);

	MblobControl(_milBlobResult, M_SAVE_RUNS, M_ENABLE); // ???

}

/*
  =======================================================================================
  =======================================================================================
*/
bool OverlayWindow::growBlobBuffers(int blobs_required)
{
	freeBlobBuffers();

	if (blobs_required <= 0) {
		blobs_required = 256;
	}
	else {
		blobs_required += 255;
		blobs_required &= 0xff00;
	}

	_blobLeft = new long[blobs_required];

	if (!_blobLeft) {
		return false;
	}

	_blobRight = new long[blobs_required];

	if (!_blobRight) {
		return false;
	}

	_blobTop = new long[blobs_required];

	if (!_blobTop) {
		return false;
	}

	_blobBottom = new long[blobs_required];

	if (!_blobBottom) {
		return false;
	}

	_blobNumChainedPixels = new long[blobs_required];

	if (!_blobNumChainedPixels) {
		return false;
	}

	_blobLabels = new long[blobs_required];

	if (!_blobLabels) {
		return false;
	}

	_pixelChains = new PixelChain[blobs_required];

	if (!_pixelChains) {
		return false;
	}

	_blobFeretMaxAngle = new double[blobs_required];

	if (!_blobFeretMaxAngle) {
		return false;
	}

	_blobFeretMaxDiameter = new double[blobs_required];

	if (!_blobFeretMaxDiameter) {
		return false;
	}

	_blobPerimeter = new double[blobs_required];

	if (!_blobPerimeter) {
		return false;
	}

	_blobConvexPerimeter = new double[blobs_required];

	if (!_blobConvexPerimeter) {
		return false;
	}

	_blobCompactness = new double[blobs_required];

	if (!_blobCompactness) {
		return false;
	}

	_blobArea = new double[blobs_required];

	if (!_blobArea) {
		return false;
	}

	_maxBlobs = blobs_required;

	return true;
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::freeBlobBuffers()
{
	if (_blobLeft) {
		delete [] _blobLeft;
		_blobLeft = NULL;
	}

	if (_blobRight) {
		delete [] _blobRight;
		_blobRight = NULL;
	}

	if (_blobTop) {
		delete [] _blobTop;
		_blobTop = NULL;
	}

	if (_blobBottom) {
		delete [] _blobBottom;
		_blobBottom = NULL;
	}

	if (_blobNumChainedPixels) {
		delete [] _blobNumChainedPixels;
		_blobNumChainedPixels = NULL;
	}

	if (_blobLabels) {
		delete [] _blobLabels;
		_blobLabels = NULL;
	}

	if (_pixelChains) {
		delete [] _pixelChains;
		_pixelChains = NULL;
	}

	if (_blobFeretMaxAngle) {
		delete [] _blobFeretMaxAngle;
		_blobFeretMaxAngle = NULL;
	}

	if (_blobFeretMaxDiameter) {
		delete [] _blobFeretMaxDiameter;
		_blobFeretMaxDiameter = NULL;
	}

	if (_blobPerimeter) {
		delete [] _blobPerimeter;
		_blobPerimeter = NULL;
	}

	if (_blobConvexPerimeter) {
		delete [] _blobConvexPerimeter;
		_blobConvexPerimeter = NULL;
	}

	if (_blobCompactness) {
		delete [] _blobCompactness;
		_blobCompactness = NULL;
	}

	if (_blobArea) {
		delete [] _blobArea;
		_blobArea = NULL;
	}

	_maxBlobs = 0;	
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::findBlobs(bool redraw /*= false*/)
{
	int max, i;
	long *chain_index = NULL;
	long *chain_x = NULL;
	long *chain_y = NULL;
	
	MblobCalculate(_binImage, M_NULL, _milFeatureList, _milBlobResult);

	_numBlobs = MblobGetNumber(_milBlobResult, M_NULL);

	if (_numBlobs < 1) {
		return;
	}

	if (_numBlobs > _maxBlobs) {
		if (!growBlobBuffers(_numBlobs)) {
			_numBlobs = 0;
			return;
		}
	}

	MblobGetResult(_milBlobResult, M_BOX_X_MIN + M_TYPE_LONG, _blobLeft);
	MblobGetResult(_milBlobResult, M_BOX_X_MAX + M_TYPE_LONG, _blobRight);
	MblobGetResult(_milBlobResult, M_BOX_Y_MIN + M_TYPE_LONG, _blobTop);
	MblobGetResult(_milBlobResult, M_BOX_Y_MAX + M_TYPE_LONG, _blobBottom);
	MblobGetResult(_milBlobResult, M_LABEL_VALUE + M_TYPE_LONG, _blobLabels);
	MblobGetResult(_milBlobResult, M_FERET_MAX_ANGLE + M_TYPE_DOUBLE, _blobFeretMaxAngle);
	MblobGetResult(_milBlobResult, M_FERET_MAX_DIAMETER + M_TYPE_DOUBLE, _blobFeretMaxDiameter);
	
	MblobGetResult(_milBlobResult, M_PERIMETER + M_TYPE_DOUBLE, _blobPerimeter);
	MblobGetResult(_milBlobResult, M_CONVEX_PERIMETER + M_TYPE_DOUBLE, _blobConvexPerimeter);
	MblobGetResult(_milBlobResult, M_COMPACTNESS + M_TYPE_DOUBLE, _blobCompactness);
	MblobGetResult(_milBlobResult, M_AREA + M_TYPE_DOUBLE, _blobArea);

	MblobGetResult(_milBlobResult, M_NUMBER_OF_CHAINED_PIXELS + M_TYPE_LONG, _blobNumChainedPixels);

	max = 0;
	for (i = 0; i < _numBlobs; i++) {
		if (_blobNumChainedPixels[i] > max) {
			max = _blobNumChainedPixels[i];
		}
	}

	if (max == 0) {
		return;
	}

	chain_index = new long[max];
	chain_x = new long[max];
	chain_y = new long[max];

	if (chain_index && chain_x && chain_y) {
		for (i = 0; i < _numBlobs; i++) {			
			MblobGetResultSingle(_milBlobResult, _blobLabels[i], M_CHAIN_INDEX + M_TYPE_LONG, chain_index);
			MblobGetResultSingle(_milBlobResult, _blobLabels[i], M_CHAIN_X + M_TYPE_LONG, chain_x);
			MblobGetResultSingle(_milBlobResult, _blobLabels[i], M_CHAIN_Y + M_TYPE_LONG, chain_y);
	
			_pixelChains[i].load(_blobFeretMaxAngle[i], _blobFeretMaxDiameter[i],
								_blobLeft[i], _blobRight[i], _blobTop[i], _blobBottom[i],
								_blobNumChainedPixels[i], chain_index, chain_x, chain_y);
		}
	}

	if (chain_index) {
		delete [] chain_index;
	}

	if (chain_x) {
		delete [] chain_x;
	}

	if (chain_y) {
		delete [] chain_y;
	}

	if (redraw) {
		InvalidateRect(_hWndMil, NULL, TRUE);
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::clearBlobs(bool redraw /*= false*/)
{
	_numBlobs = 0;	

	if (redraw) {
		InvalidateRect(_hWndMil, NULL, TRUE);
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
int OverlayWindow::findBlobFromPt(int x, int y)
{
	for (int i = 0; i < _numBlobs; i++) {
		if (x >= _blobLeft[i] && x <= _blobRight[i]) {
			if (y >= _blobTop[i] && y <= _blobBottom[i]) {
				return i;
			}
		}
	}

	return -1;
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int new_selected_blob = findBlobFromPt(LOWORD(lParam), HIWORD(lParam));

	if (new_selected_blob != _selectedBlob) {
		_selectedBlob = new_selected_blob;
		InvalidateRect(_hWndMil, NULL, TRUE);
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
HWND OverlayWindow::getWindowHandle()
{
	return _hWnd;
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::updateWindow(MIL_ID srcImage, MIL_ID binImage)
{
	_skipPaintMessage = true;

	_numBlobs = 0;
	_selectedBlob = -1;

	if (_srcImage) {
		MbufCopyColor(srcImage, _srcImage, M_ALL_BANDS);
	}

	if (_binImage) {
		MbufCopy(binImage, _binImage);
		setPixels();
		findBlobs();
	}

	if (!_hWnd) {
		showWindow();
	}
	else {
		SetWindowPos(_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	_skipPaintMessage = false;

	if (_hWnd) {
		InvalidateRect(_hWnd, NULL, TRUE);
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::showWindow()
{
	RECT rect, rectDefault;

	rectDefault.left = 50;
	rectDefault.top = 50;
	
	_minWindowSize.cx = (2 * GetSystemMetrics(SM_CXBORDER)) + (IMAGE_WIDTH / 2);

	_minWindowSize.cy = TOOL_BAR_HEIGHT
						+ (2 * GetSystemMetrics(SM_CYBORDER))
						+ GetSystemMetrics(SM_CYCAPTION)
						+ GetSystemMetrics(SM_CYMENU) 
						+ (IMAGE_HEIGHT / 2);
	

	if (GetSystemMetrics(SM_CYFULLSCREEN) < 800) {
		_maxWindowSize.cx = _minWindowSize.cx + ((IMAGE_WIDTH * 3) / 10);
		_maxWindowSize.cy = _minWindowSize.cy + ((IMAGE_HEIGHT * 3) / 10);
	}
	else {
		_maxWindowSize.cx = _minWindowSize.cx + (IMAGE_WIDTH / 2);
		_maxWindowSize.cy = _minWindowSize.cy + (IMAGE_HEIGHT / 2);
	}

	if (_zoomFactor == ZOOM_FACTOR_50_PERCENT) {
		rectDefault.right = rectDefault.left + _minWindowSize.cx;
		rectDefault.bottom = rectDefault.top + _minWindowSize.cy; 
	}
	else {
		rectDefault.right = rectDefault.left + _maxWindowSize.cx;
		rectDefault.bottom = rectDefault.top + _maxWindowSize.cy; 
	}

	if (!loadWindowRect("OverlayWindow", &rect, &rectDefault)) {
		rect = rectDefault;
	}

	if (_zoomFactor == ZOOM_FACTOR_50_PERCENT) {
		rect.right = rect.left + _minWindowSize.cx;
		rect.bottom = rect.top + _minWindowSize.cy;
	}
	else {
		rect.right = rect.left + _maxWindowSize.cx;
		rect.bottom = rect.top + _maxWindowSize.cy;
	}

	_hWnd = CreateWindow(OverlayWindow::WinClass,
						"Overlay Results",	    
						WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,						
						rect.left,	  
						rect.top,	
						rect.right - rect.left,
						rect.bottom - rect.top,
						NULL,   
						NULL,
						GetModuleHandle(NULL),	  
						this);  

	if (_hWnd) {
		ShowWindow(_hWnd, SW_SHOWNORMAL);

		if (_zoomFactor == ZOOM_FACTOR_50_PERCENT) {
			EnableWindow(_hWndTBButtons[TB_ZOOM_IN], TRUE);
			EnableWindow(_hWndTBButtons[TB_ZOOM_OUT], FALSE);
		}
		else {
			EnableWindow(_hWndTBButtons[TB_ZOOM_IN], FALSE);
			EnableWindow(_hWndTBButtons[TB_ZOOM_OUT], TRUE);
		}

		SetWindowText(_hWndTBButtons[TB_PIXELS], _showOverlayPixels ? "Hide Pixels" : "Show Pixels");
		SetWindowText(_hWndTBButtons[TB_RECTS], _showBoundingRects ? "Hide Rects" : "Show Rects");
		SetWindowText(_hWndTBButtons[TB_EDGES], _showEdges ? "Hide Edges" : "Show Edges");
		SetWindowText(_hWndTBButtons[TB_IMAGE], _showImages ? "Hide Image" : "Show Images");

		UpdateWindow(_hWnd);
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::zoomIn()
{
	double zoom;
	int maxY;

	if (ZOOM_FACTOR_50_PERCENT != _zoomFactor) {
		return;
	}

	maxY = GetSystemMetrics(SM_CYFULLSCREEN);

	if (maxY < 800) {
		_zoomFactor = ZOOM_FACTOR_80_PERCENT;
		zoom = -1.25;
		// (pad + 1/2) + 3/10 = pad + 8/10
		_maxWindowSize.cx = _minWindowSize.cx + ((IMAGE_WIDTH * 3) / 10);
		_maxWindowSize.cy = _minWindowSize.cy + ((IMAGE_HEIGHT * 3) / 10);	
	}
	else {
		_zoomFactor = ZOOM_FACTOR_100_PERCENT;
		zoom = 1.0;
		// (pad + 1/2) + 1/2 = pad + 1
		_maxWindowSize.cx = _minWindowSize.cx + (IMAGE_WIDTH / 2);
		_maxWindowSize.cy = _minWindowSize.cy + (IMAGE_HEIGHT / 2);	
	}

	SetWindowPos(_hWnd, HWND_TOP, 0, 0, _maxWindowSize.cx, _maxWindowSize.cy, SWP_NOMOVE);		
	MdispZoom(_milDisplay, zoom, zoom);
	
	EnableWindow(_hWndTBButtons[TB_ZOOM_IN], FALSE);
	EnableWindow(_hWndTBButtons[TB_ZOOM_OUT], TRUE);

	wmExitSizeMove();
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::zoomOut()
{
	if (ZOOM_FACTOR_50_PERCENT == _zoomFactor) {
		return;
	}

	_maxWindowSize.cx = _minWindowSize.cx;
	_maxWindowSize.cy = _minWindowSize.cy;					
	_zoomFactor = ZOOM_FACTOR_50_PERCENT;

	SetWindowPos(_hWnd, HWND_TOP, 0, 0, _maxWindowSize.cx, _maxWindowSize.cy, SWP_NOMOVE);		
	MdispZoom(_milDisplay, -2.0, -2.0);

	EnableWindow(_hWndTBButtons[TB_ZOOM_IN], TRUE);
	EnableWindow(_hWndTBButtons[TB_ZOOM_OUT], FALSE);

	wmExitSizeMove();
}

/*
  =======================================================================================
  _overlayColor = RGB(240, 0, 0);
	_boundingRectColor = RGB(0, 240, 0);
	_edgeColor = RGB(255, 255, 255);
  =======================================================================================
*/
void OverlayWindow::saveSettings()
{
	char zero[2], one[2];
	char buff[32], section[32];

	zero[0] = '0';
	zero[1] = 0;

	one[0] = '1';
	one[1] = 0;

	if (!loadIniFilename()) {
		return;
	}

	strncpy_s(section, sizeof(section), "OverlayWindow", _TRUNCATE);

	WritePrivateProfileString(section, "ShowBoundingRects", 
		_showBoundingRects ? one : zero, gIniFilename);
	
	WritePrivateProfileString(section, "ShowOverlayPixels", 
		_showOverlayPixels ? one : zero, gIniFilename);

	WritePrivateProfileString(section, "ShowEdges", 
		_showEdges ? one : zero, gIniFilename);

	WritePrivateProfileString(section, "ShowImages", 
		_showImages ? one : zero, gIniFilename);

	sprintf_s(buff, sizeof(buff), "%u", _overlayColor);
	WritePrivateProfileString(section, "OverlayColor", buff, gIniFilename);

	sprintf_s(buff, sizeof(buff), "%u", _boundingRectColor);
	WritePrivateProfileString(section, "BoundingRectColor", buff, gIniFilename);

	sprintf_s(buff, sizeof(buff), "%u", _edgeColor);
	WritePrivateProfileString(section, "EdgeColor", buff, gIniFilename);

	sprintf_s(buff, sizeof(buff), "%d", _penWidth);
	WritePrivateProfileString(section, "PenWidth", buff, gIniFilename);

	sprintf_s(buff, sizeof(buff), "%d", _zoomFactor);
	WritePrivateProfileString(section, "ZoomFactor", buff, gIniFilename);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::loadSettings()
{
	char section[32];

	if (!loadIniFilename()) {
		return;
	}

	strncpy_s(section, sizeof(section), "OverlayWindow", _TRUNCATE);

	_showBoundingRects = (1 == GetPrivateProfileInt(section, "ShowBoundingRects", 0, gIniFilename));
	_showOverlayPixels = (1 == GetPrivateProfileInt(section, "ShowOverlayPixels", 1, gIniFilename));
	_showEdges = (1 == GetPrivateProfileInt(section, "ShowEdges", 0, gIniFilename));
	_showImages = (1 == GetPrivateProfileInt(section, "ShowImages", 1, gIniFilename));

	_overlayColor = GetPrivateProfileInt(section, "OverlayColor", _overlayColor, gIniFilename);
	_boundingRectColor = GetPrivateProfileInt(section, "BoundingRectColor", _boundingRectColor, gIniFilename);
	_edgeColor = GetPrivateProfileInt(section, "EdgeColor", _edgeColor, gIniFilename);
	_penWidth = GetPrivateProfileInt(section, "PenWidth", _penWidth, gIniFilename);

	_zoomFactor = GetPrivateProfileInt(section, "ZoomFactor", _zoomFactor, gIniFilename);

	if (_zoomFactor != ZOOM_FACTOR_80_PERCENT && _zoomFactor != ZOOM_FACTOR_100_PERCENT) {
		_zoomFactor = ZOOM_FACTOR_50_PERCENT;
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::closeWindow()
{
	saveSettings();

	if (_hWnd) {
		if (_milDisplay) {
			MdispSelect(_milDisplay, M_NULL);
		}

		DestroyWindow(_hWnd);
		_hWnd = 0;
	}

	_hWndMil = 0;

	//if (_hFont) {
	//	DeleteObject(_hFont);
	//	_hFont = 0;
	//}
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmCreate(HWND hWnd)
{
	int h, w;
	double zoom;
	//_hFont = CreateFont(-11, 0, 0, 0, 400,	
	//				   0, 0, 0, 0, OUT_TT_PRECIS, 0, CLEARTYPE_NATURAL_QUALITY, 0, "Tahoma"); 	

	if (!_hWndTBButtons) {	
		createToolbar(hWnd);
	}

	if (_zoomFactor == ZOOM_FACTOR_50_PERCENT) {
		w = IMAGE_WIDTH / 2;
		h = IMAGE_HEIGHT / 2;
		zoom = -2.0;
	}
	else if (_zoomFactor == ZOOM_FACTOR_80_PERCENT) {
		w = (IMAGE_WIDTH * 4) / 5;
		h = (IMAGE_HEIGHT * 4) / 5;
		zoom = -1.25;
	}
	else {
		w = IMAGE_WIDTH;
		h = IMAGE_HEIGHT;
		zoom = 1.0;
	}
	
	_hWndMil = CreateWindow(OverlayWindow::MilWinClass,
							"",
							WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 
							0,
							TOOL_BAR_HEIGHT,
							w,
							h,
							hWnd,
							NULL,
							GetModuleHandle(NULL),
							this);	

	if (_hWndMil) {
		MdispSelectWindow(_milDisplay, _srcImage, _hWndMil);
		MdispZoom(_milDisplay, zoom, zoom);
	}

	_popupMenu = LoadMenu(GetModuleHandle(NULL), "OVERLAY_WIN_POPUP_MENU");
	_popupMenu = GetSubMenu(_popupMenu, 0);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::createToolbar(HWND hWnd)
{	
	int x, y, w, h;
	HFONT hFont;
	HINSTANCE hInstance;

	_hWndTBButtons = new HWND[NUM_TOOLBAR_BUTTONS];

	if (!_hWndTBButtons) {
		return;
	}

	hInstance = GetModuleHandle(NULL);

	for (int i = 0; i < NUM_TOOLBAR_BUTTONS; i++) {
		_hWndTBButtons[i] = 0;
	}

	x = 8;
	y = 5;
	w = 80;
	h = 24;

	_hWndTBButtons[TB_PIXELS] = CreateWindow("button", "Pixels", 
												WS_CHILD | WS_VISIBLE, 
												x, y, w, h,
												hWnd, (HMENU) TB_PIXELS, hInstance, NULL);

	x += w + 4;

	_hWndTBButtons[TB_EDGES] = CreateWindow("button", "Edges", 
												WS_CHILD | WS_VISIBLE, 
												x, y, w, h,
												hWnd, (HMENU) TB_EDGES, hInstance, NULL);

	x += w + 4;

	_hWndTBButtons[TB_RECTS] = CreateWindow("button", "Rects", 
												WS_CHILD | WS_VISIBLE, 
												x, y, w, h,
												hWnd, (HMENU) TB_RECTS, hInstance, NULL);

	x += w + 4;

	_hWndTBButtons[TB_IMAGE] = CreateWindow("button", "Image", 
												WS_CHILD | WS_VISIBLE, 
												x, y, w, h,
												hWnd, (HMENU) TB_IMAGE, hInstance, NULL);

	x += w + 8;
	w = 26;

	_hWndTBButtons[TB_ZOOM_IN] = CreateWindow("button", "Z+", 
												WS_CHILD | WS_VISIBLE, 
												x, y, w, h,
												hWnd, (HMENU) TB_ZOOM_IN, hInstance, NULL);

	x += w + 4;

	_hWndTBButtons[TB_ZOOM_OUT] = CreateWindow("button", "Z-", 
												WS_CHILD | WS_VISIBLE, 
												x, y, w, h,
												hWnd, (HMENU) TB_ZOOM_OUT, hInstance, NULL);

	hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

	for (int i = 0; i < NUM_TOOLBAR_BUTTONS; i++) {
		if (_hWndTBButtons[i]) {
			SendMessage(_hWndTBButtons[i], WM_SETFONT, (WPARAM) hFont, FALSE);
		}
	}		
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmDestroy()
{
	PostMessage(_hWndParent, WM_OVERLAY_WINDOW_DESTROYED, 0, 0);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc, hMemDC;
	HBITMAP bmp, oldbmp;
	HPEN oldPen;
	HBRUSH oldBrush;
	RECT r;
	
	hdc = BeginPaint(_hWnd, &ps);

	GetClientRect(_hWnd, &r);

	hMemDC = CreateCompatibleDC(hdc);
	bmp = CreateCompatibleBitmap(hdc, r.right - r.left, r.bottom - r.top);
	oldbmp = (HBITMAP) SelectObject(hMemDC, bmp);
	oldPen = (HPEN) SelectObject(hMemDC, GetStockObject(WHITE_PEN));
	oldBrush = (HBRUSH) SelectObject(hMemDC, GetStockObject(WHITE_BRUSH));
	Rectangle(hMemDC, r.left, r.top, r.right, r.bottom);
	SelectObject(hMemDC, oldPen);
	SelectObject(hMemDC, oldBrush);

	drawToolbar(hMemDC);

	BitBlt(hdc, 0, 0, r.right - r.left, r.bottom - r.top, hMemDC, 0, 0, SRCCOPY); 
	
	SelectObject(hMemDC, oldbmp);
	DeleteObject(bmp);
	DeleteDC(hMemDC);

	EndPaint(_hWnd, &ps);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::drawToolbar(HDC hdc)
{
	HPEN darkGreyPen, oldPen;
	HBRUSH greyBrush, oldBrush;
	int left, right, top, bottom;
	RECT r;

	GetClientRect(_hWnd, &r);

	left = 0;
	top = 0;
	right = (r.right - r.left); // - 8;
	bottom = TOOL_BAR_HEIGHT;

	greyBrush = CreateSolidBrush(_grey);
	darkGreyPen = CreatePen(PS_SOLID, 1, _grey); //GetSysColor(COLOR_BTNSHADOW));

	oldBrush = (HBRUSH) SelectObject(hdc, greyBrush);
	oldPen = (HPEN) SelectObject(hdc, darkGreyPen);
	Rectangle(hdc, left, top, right, bottom);	

	/*
	MoveToEx(hdc, left, top, NULL);
	LineTo(hdc, right, top);
	MoveToEx(hdc, left, bottom - 1, NULL);
	LineTo(hdc, right, bottom - 1);

	SelectObject(hdc, GetStockObject(WHITE_PEN));
	MoveToEx(hdc, left, top + 1, NULL);
	LineTo(hdc, right, top + 1);
	*/
	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);	
	DeleteObject(greyBrush);
	DeleteObject(darkGreyPen);

	r.bottom = r.top + TOOL_BAR_HEIGHT;

	DrawEdge(hdc, &r, EDGE_RAISED, BF_RECT);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmGetMinMaxInfo(MINMAXINFO *pMinMax)
{
	pMinMax->ptMinTrackSize.x = _minWindowSize.cx;
	pMinMax->ptMinTrackSize.y = _minWindowSize.cy;
	pMinMax->ptMaxTrackSize.x = _maxWindowSize.cx;
	pMinMax->ptMaxTrackSize.y = _maxWindowSize.cy;
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmExitSizeMove()
{
	RECT r;
	int h, w;

	GetClientRect(_hWnd, &r);

	h = (r.bottom - r.top) - (TOOL_BAR_HEIGHT + 1);
	w = r.right - r.left;

	if (_hWndMil) {
		SetWindowPos(_hWndMil, NULL, 
						0, 
						TOOL_BAR_HEIGHT + 1, 
						w, 
						h, 
						SWP_NOZORDER);
	}

	InvalidateRect(_hWnd, NULL, TRUE);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case TB_PIXELS:
		_showOverlayPixels = !_showOverlayPixels;
		CheckMenuItem(GetMenu(_hWnd), ID_TOOLS_SHOWOVERLAYPIXELS, _showOverlayPixels ? MF_CHECKED : MF_UNCHECKED);
		SetWindowText(_hWndTBButtons[TB_PIXELS], _showOverlayPixels ? "Hide Pixels" : "Show Pixels");
		InvalidateRect(_hWndMil, NULL, TRUE);
		break;

	case TB_RECTS:
		_showBoundingRects = !_showBoundingRects;
		CheckMenuItem(GetMenu(_hWnd), ID_TOOLS_SHOWBOUNDINGRECTS, _showBoundingRects ? MF_CHECKED : MF_UNCHECKED);
		SetWindowText(_hWndTBButtons[TB_RECTS], _showBoundingRects ? "Hide Rects" : "Show Rects");
		InvalidateRect(_hWndMil, NULL, TRUE);
		break;

	case TB_EDGES:
		_showEdges = !_showEdges;
		CheckMenuItem(GetMenu(_hWnd), ID_TOOLS_SHOWEDGES, _showEdges ? MF_CHECKED : MF_UNCHECKED);
		SetWindowText(_hWndTBButtons[TB_EDGES], _showEdges ? "Hide Edges" : "Show Edges");
		InvalidateRect(_hWndMil, NULL, TRUE);
		break;

	case TB_IMAGE:
		_showImages = !_showImages;
		CheckMenuItem(GetMenu(_hWnd), ID_TOOLS_SHOWIMAGES, _showImages ? MF_CHECKED : MF_UNCHECKED);
		SetWindowText(_hWndTBButtons[TB_IMAGE], _showImages ? "Hide Image" : "Show Images");
		InvalidateRect(_hWndMil, NULL, TRUE);
		break;

	case TB_ZOOM_IN:
		zoomIn();
		break;

	case TB_ZOOM_OUT:
		zoomOut();
		break;

	case ID_TOOLS_COLORS:
		doSettingsDlg();
		break;

	case ID_SAVE_IMAGE:
		//saveImage();
		break;

	case ID_CLOSE:
		closeWindow();
		break;
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
LRESULT CALLBACK 
OverlayWindow::WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	OverlayWindow *win;
	LPCREATESTRUCT cs;

	switch (msg) 
	{
	case WM_PAINT:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmPaint(hWnd);
		}

		break;
	
	/*
	case WM_SIZE:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmSize(hWnd, wParam, lParam);
		}

		break;
	*/

	case WM_GETMINMAXINFO:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmGetMinMaxInfo((MINMAXINFO *) lParam);
		}
	
		break;

	case WM_EXITSIZEMOVE:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmExitSizeMove();
		}

		break;

	case WM_COMMAND:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmCommand(wParam, lParam);
		}

		break;			  

	case WM_CLOSE:		
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->closeWindow();
		}

		break;

	case WM_CREATE:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmCreate(hWnd);
		}

		break;

	case WM_DESTROY:
		saveWindowRect("OverlayWindow", hWnd); 

		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

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
  =======================================================================================
*/
void OverlayWindow::setPixels()
{
	unsigned char *p;
	int pitch_byte, size_byte, rows, cols;

	if (!_binImage) {
		memset(_pixels, 0, sizeof(_pixels));
		return;
	}

	p = 0;

	MbufInquire(_binImage, M_HOST_ADDRESS, &p);

	if (!p) {
		return;
	}

	pitch_byte = MbufInquire(_binImage, M_PITCH_BYTE, M_NULL);
	size_byte = MbufInquire(_binImage, M_SIZE_BYTE, M_NULL);

	cols = pitch_byte;

	if (pitch_byte != 0) {
		rows = size_byte / pitch_byte;
	}
	else {
		rows = 0;
	}

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			_pixels[y][x] = *p;
			p++;
		}
	}
}


/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::wmMilPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;

	hdc = BeginPaint(hWnd, &ps);

	if (!_showImages) {
		clearBackground(hWnd, hdc);
	}

	if (_showOverlayPixels) {
		drawOverlayPixels(hdc);
	}

	if (_numBlobs > 0) {
		if (_showBoundingRects) {
			drawBoundingRects(hdc);
		}

		if (_showEdges) {
			drawBlobEdges(hdc);
		}

		if (_selectedBlob >= 0 && _selectedBlob < _numBlobs) {
			drawSelectedBlob(hdc);
		}
	}

	EndPaint(hWnd, &ps);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::clearBackground(HWND hWnd, HDC hdc)
{
	RECT r;
	HPEN oldPen;
	HBRUSH oldBrush;

	GetClientRect(hWnd, &r);

	oldPen = (HPEN) SelectObject(hdc, GetStockObject(BLACK_PEN));
	oldBrush = (HBRUSH) SelectObject(hdc, GetStockObject(BLACK_BRUSH));

	Rectangle(hdc, r.left, r.top, r.right, r.bottom);

	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::drawOverlayPixels(HDC hdc)
{
	int x, y;

	for (int i = 0; i < IMAGE_HEIGHT; i++) {
		for (int j = 0; j < IMAGE_WIDTH; j++) {
			if (_pixels[i][j]) {
				x = j;
				y = i;

				if (_zoomFactor == ZOOM_FACTOR_50_PERCENT) {
					x /= 2;
					y /= 2;
				}
				else if (_zoomFactor == ZOOM_FACTOR_80_PERCENT) {
					x = (x * 4) / 5;
					y = (y * 4) / 5;
				}

				SetPixel(hdc, x, y, _overlayColor);
			}
		}
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::drawBoundingRects(HDC hdc)
{
	HPEN oldPen, pen;
	RECT r;

	pen = CreatePen(PS_SOLID, _penWidth, _boundingRectColor);
	oldPen = (HPEN) SelectObject(hdc, pen);

	for (int i = 0; i < _numBlobs; i++) {
		r.left = _blobLeft[i];
		r.right = _blobRight[i];
		r.top = _blobTop[i];
		r.bottom = _blobBottom[i];

		if (_zoomFactor == ZOOM_FACTOR_50_PERCENT) {
			r.left /= 2;
			r.right /= 2;
			r.top /= 2;
			r.bottom /= 2;
		}
		else if (_zoomFactor == ZOOM_FACTOR_80_PERCENT) {
			r.left = (r.left * 4) / 5;
			r.right = (r.right * 4) / 5;
			r.top = (r.top * 4) / 5;
			r.bottom = (r.bottom * 4) / 5;
		}

		MoveToEx(hdc, r.left, r.top, NULL);
		LineTo(hdc, r.right, r.top);
		LineTo(hdc, r.right, r.bottom);
		LineTo(hdc, r.left, r.bottom);
		LineTo(hdc, r.left, r.top);
	}

	SelectObject(hdc, oldPen);
	DeleteObject(pen);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::drawSelectedBlob(HDC hdc)
{
	HPEN oldPen, pen;
	int i = _selectedBlob;
	
	pen = CreatePen(PS_SOLID, 2, _boundingRectColor);
	oldPen = (HPEN) SelectObject(hdc, pen);

	MoveToEx(hdc, _blobLeft[i] - 1, _blobTop[i] - 1, NULL);
	LineTo(hdc, _blobRight[i] + 1, _blobTop[i] - 1);
	LineTo(hdc, _blobRight[i] + 1, _blobBottom[i] + 1);
	LineTo(hdc, _blobLeft[i] - 1, _blobBottom[i] + 1);
	LineTo(hdc, _blobLeft[i] - 1, _blobTop[i] - 1);

	SelectObject(hdc, oldPen);
	DeleteObject(pen);
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::drawBlobEdges(HDC hdc)
{
	PixelChain *pc;
	int x, y;
	HPEN oldPen, pen;
	
	pen = CreatePen(PS_SOLID, _penWidth, _edgeColor);
	oldPen = (HPEN) SelectObject(hdc, pen);

	for (int i = 0; i < _numBlobs; i++) {
		pc = &_pixelChains[i];

		for (int j = 0; j < pc->_numPoints; j++) {
			x = pc->_pt[j].x;
			y = pc->_pt[j].y;

			if (_zoomFactor == ZOOM_FACTOR_50_PERCENT) {
				x /= 2;
				y /= 2;
			}
			else if (_zoomFactor == ZOOM_FACTOR_80_PERCENT) {
				x = (x * 4) / 5;
				y = (y * 4) / 5;
			}

			if (j) {
				LineTo(hdc, x, y);
			}
			else {
				MoveToEx(hdc, x, y, NULL);
			}
			/*
			SetPixel(hdc, x, y, _edgeColor);

			if (_penWidth > 1) {
				if (x > 0) {
					x--;
				}
				else {
					x++;
				}

				SetPixel(hdc, x, y, _edgeColor);
			}
			*/
		}
	}

	SelectObject(hdc, oldPen);
	DeleteObject(pen);
}

/*
  =======================================================================================
  =======================================================================================
*/
LRESULT CALLBACK 
OverlayWindow::MilWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPCREATESTRUCT cs;
	OverlayWindow *win;

	switch (msg) 
	{
	case WM_PAINT:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			if (!win->_skipPaintMessage) {
				win->wmMilPaint(hWnd);
			}
		}

		break;

	case WM_LBUTTONDOWN:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmLButtonDown(hWnd, wParam, lParam);
		}

		break;

	/*
	case WM_RBUTTONUP:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmRButtonUp(hWnd, wParam, lParam);
		}

		break;

	case WM_SIZE:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			// update ImageManager with new size here
		}

		break;

	case WM_MOUSEMOVE:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmMouseMove(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
		}

		break;

	case WM_LBUTTONUP:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmLButtonUp(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
		}

		break;
	*/

	case WM_COMMAND:
		win = (OverlayWindow *) GetWindowLong(hWnd, GWL_USERDATA);

		if (win) {
			win->wmCommand(wParam, lParam);
		}

		break;

	case WM_NCCREATE:
		cs = (LPCREATESTRUCT) lParam;
		SetWindowLong(hWnd, GWL_USERDATA, (long) cs->lpCreateParams);
		// deliberate fall through
		
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0;	
}

/*
  =======================================================================================
  =======================================================================================
*/
void OverlayWindow::doSettingsDlg()
{
	DialogBoxParam(GetModuleHandle(NULL), 
		MAKEINTRESOURCE(IDD_OVERLAY_SETTINGS), 
		_hWnd, 
		OverlayWindow::SettingsDlgProc, 
		(LPARAM) this);

	InvalidateRect(_hWndMil, NULL, TRUE);
}

/*
  =======================================================================================
  static function
  =======================================================================================
*/
BOOL CALLBACK 
OverlayWindow::SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CHOOSECOLOR cc;
	HBRUSH oldBrush, brush;
	RECT *pRect;
	LPDRAWITEMSTRUCT pdis;
	OverlayWindow *win;

	switch (msg) 
	{
	case WM_INITDIALOG:
		if (lParam) {
			SetWindowLong(hDlg, GWL_USERDATA, lParam);
			win = (OverlayWindow *) lParam;
			SetDlgItemInt(hDlg, IDC_PEN_WIDTH, win->_penWidth, FALSE);			
		}

		break;

	case WM_DRAWITEM:
		win = (OverlayWindow *) GetWindowLong(hDlg, GWL_USERDATA);

		if (!win) {
			return FALSE;
		}

		pdis = (LPDRAWITEMSTRUCT) lParam;

		switch (pdis->CtlID)
		{
		case IDC_OVERLAY_COLOR:
			brush = CreateSolidBrush(win->_overlayColor);
			break;

		case IDC_BOUNDING_RECT_COLOR:
			brush = CreateSolidBrush(win->_boundingRectColor);	
			break;

		case IDC_EDGE_COLOR:
			brush = CreateSolidBrush(win->_edgeColor);
			break;

		default:
			return FALSE;
		}
		
		oldBrush = (HBRUSH) SelectObject(pdis->hDC, brush);
		pRect = &pdis->rcItem;

		Rectangle(pdis->hDC, pRect->left, pRect->top, pRect->right, pRect->bottom);
		
		SelectObject(pdis->hDC, oldBrush);
		DeleteObject(brush);

		return TRUE;

	case WM_COMMAND:
		win = (OverlayWindow *) GetWindowLong(hDlg, GWL_USERDATA);

		if (!win) {
			return FALSE;
		}

		switch (LOWORD(wParam))
		{
		case IDC_OVERLAY_COLOR:
			memset(&cc, 0, sizeof(CHOOSECOLOR));
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.hwndOwner = hDlg;
			cc.lpCustColors = win->_customColors;
			cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
			cc.rgbResult = win->_overlayColor;

			if (ChooseColor(&cc)) {
				win->_overlayColor = cc.rgbResult;
				InvalidateRect(hDlg, NULL, TRUE);
			}

			break;

		case IDC_BOUNDING_RECT_COLOR:
			memset(&cc, 0, sizeof(CHOOSECOLOR));
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.hwndOwner = hDlg;
			cc.lpCustColors = win->_customColors;
			cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
			cc.rgbResult = win->_boundingRectColor;

			if (ChooseColor(&cc)) {
				win->_boundingRectColor = cc.rgbResult;
				InvalidateRect(hDlg, NULL, TRUE);
			}

			break;

		case IDC_EDGE_COLOR:
			memset(&cc, 0, sizeof(CHOOSECOLOR));
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.hwndOwner = hDlg;
			cc.lpCustColors = win->_customColors;
			cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
			cc.rgbResult = win->_edgeColor;

			if (ChooseColor(&cc)) {
				win->_edgeColor = cc.rgbResult;
				InvalidateRect(hDlg, NULL, TRUE);
			}

			break;

		case IDOK:
			win = (OverlayWindow *) GetWindowLong(hDlg, GWL_USERDATA);

			if (win) {
				win->_penWidth = GetDlgItemInt(hDlg, IDC_PEN_WIDTH, NULL, TRUE);

				if (win->_penWidth < 1) {
					win->_penWidth = 1;
				}
				else if (win->_penWidth > 3) {
					win->_penWidth = 3;
				}
			}

			EndDialog(hDlg, 1);
			break;

		case IDCANCEL:
			EndDialog(hDlg, 0);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

/*
  =======================================================================================
  static function
  =======================================================================================
*/
bool OverlayWindow::registerWinClass(HINSTANCE hInstance)
{
	WNDCLASS wc;
	
	memset(&wc, 0, sizeof(wc));

	wc.lpfnWndProc = OverlayWindow::WinProc;																
	wc.cbWndExtra = sizeof(OverlayWindow *);							
	wc.hInstance = hInstance; 		
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMALG)); 
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);   			
	wc.lpszMenuName = "OVERLAY_WINDOW_MENU";	
	wc.lpszClassName = OverlayWindow::WinClass;	
	
	if (!RegisterClass(&wc)) {
		return false;
	}

	wc.lpfnWndProc = OverlayWindow::MilWinProc;
	wc.lpszClassName = OverlayWindow::MilWinClass;
	wc.hIcon = 0;
	wc.lpszMenuName = 0;

	if (!RegisterClass(&wc)) {
		return false;
	}

	OverlayWindow::Registered = true;
	
	return OverlayWindow::Registered;
}
