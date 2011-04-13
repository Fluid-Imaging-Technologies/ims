/*
 * $Id: OverlayWindow.h,v 1.3 2009/04/30 19:01:05 scott Exp $
 *
 */

#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H

#include "version.h"
#include <mil.h>

#include "PixelChain.h"

#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 768

#define WM_OVERLAY_WINDOW_DESTROYED (WM_USER + 100)


class OverlayWindow
{
public:
	OverlayWindow(HWND hWndParent);
	~OverlayWindow();

	static bool registerWinClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK MilWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static char WinClass[32];
	static char MilWinClass[32];
	static bool Registered;

	HWND getWindowHandle();
	void updateWindow(MIL_ID srcImage, MIL_ID binImage);
	
	void closeWindow();
	void findBlobs(bool redraw = false);
	void clearBlobs(bool redraw = false);

protected:
	void wmCreate(HWND hWnd);
	void wmDestroy();
	void wmPaint(HWND hWnd);
	void wmSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void wmExitSizeMove();
	void wmGetMinMaxInfo(MINMAXINFO *pMinMax);
	void wmCommand(WPARAM wParam, LPARAM lParam);
	void wmLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void wmRButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);

	void wmMilPaint(HWND hWnd);
	void clearBackground(HWND hWnd, HDC hdc);
	void drawOverlayPixels(HDC hdc);
	void drawBoundingRects(HDC hdc);
	void drawBlobEdges(HDC hdc);
	void drawSelectedBlob(HDC hdc);
	void drawToolbar(HDC hdc);

	void showWindow();
	void setPixels();

	void zoomIn();
	void zoomOut();
	void setControlStates();
	void createToolbar(HWND hWnd);

	bool growBlobBuffers(int blobs_required);
	void freeBlobBuffers();
	
	void saveSettings();
	void loadSettings();
	void doSettingsDlg();

	void initializeMilVars();
	int findBlobFromPt(int x, int y);

private:
	HWND _hWnd;
	HWND _hWndParent;
	HWND _hWndMil;
	//HFONT _hFont;
	HMENU _popupMenu;
	HWND *_hWndTBButtons;
	unsigned long _grey;

	int _zoomFactor;
	SIZE _minWindowSize;
	SIZE _maxWindowSize;

	MIL_ID _srcImage;
	MIL_ID _binImage;
	MIL_ID _milDisplay;
	MIL_ID _milFeatureList;
	MIL_ID _milBlobResult;

	bool _skipPaintMessage;
	bool _showBoundingRects;
	bool _showOverlayPixels;
	bool _showEdges;
	bool _showImages;
	unsigned long _overlayColor;
	unsigned long _boundingRectColor;
	unsigned long _edgeColor;
	unsigned long _customColors[16];
	int _penWidth;

	unsigned char _pixels[IMAGE_HEIGHT][IMAGE_WIDTH];

	int _selectedBlob;
	int _maxBlobs;
	int _numBlobs;
	long *_blobLeft;
	long *_blobRight;
	long *_blobTop;
	long *_blobBottom;
	long *_blobNumChainedPixels;
	long *_blobLabels;
	double *_blobFeretMaxAngle;
	double *_blobFeretMaxDiameter;
	double *_blobPerimeter;
	double *_blobConvexPerimeter;
	double *_blobCompactness;
	double *_blobArea;
	PixelChain *_pixelChains;
};


#endif // ifndef OVERLAY_WINDOW_H
