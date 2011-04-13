/*
 * $Id: main.cpp,v 1.11 2009/04/27 19:17:54 scott Exp $
 *
 * A possible series of steps -
 *
 * Raw: Sharpen 1
 * Raw: Smooth (2x)
 * Mask:
 * Grayscale: Smooth
 * Binarize
 * Bin: Close
 *
 */


#include "main.h"
#include <crtdbg.h>
#include <commctrl.h>

#include "utility.h"
#include "OverlayWindow.h"
#include "StringList.h"
#include "ImageOperations.h"

#include "resource.h"


MIL_ID gMilApplication;
MIL_ID gMilSystem;
MIL_ID gMilRawDisplay;
MIL_ID gMilBackgroundDisplay;
MIL_ID gMilBackgroundBuff;
MIL_ID gMilThresholdBuff;
MIL_ID gMilRawOriginalBuff;
MIL_ID gMilRawYUVBuff;
MIL_ID gMilSingleYUVPlaneBuff;
MIL_ID gMilMaskedDisplay;
MIL_ID gMilMaskedImageBuff;
MIL_ID gMilTempBuff;
MIL_ID gMilGrayscaleDisplay;
MIL_ID gMilGrayscaleBuff;
MIL_ID gMilLogBuff;
MIL_ID gMilBinarizedDisplay;
MIL_ID gMilBinarizedBuff;


long gImageWidth;
long gImageHeight;
bool gColorImage;
long gImageDepth;
long gImageSign;
long gFileFormat;

bool gImageShowing;
bool gBackgroundImageShowing;
bool gMaskedImageShowing;
bool gGrayscaleImageShowing;
bool gBinarizedImageShowing;

HWND ghWndMain;
OverlayWindow *gOverlayWindow;


ImageOperations gImageOps;

#define RAW_OPS 0
#define MASK_OPS 1
#define GRAYSCALE_OPS 2
#define BINARY_OPS 3
#define NUM_OP_TYPES 4

bool gTimingOn;
unsigned long gElapsedTime;

/*
  =======================================================================================
  =======================================================================================
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	MSG msg;

#if defined (_DEBUG)

	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 

#endif 

	gTimingOn = true;

	if (!initApplication(hInstance, cmdShow)) {
		return 0;
	}
	
	while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (gMilApplication) {
		MappFree(gMilApplication);
		gMilApplication = 0;
	}

	return msg.wParam;
}

/*
  =======================================================================================
  =======================================================================================
*/
bool initApplication(HINSTANCE hInstance, int cmdShow)
{
	WNDCLASS wc;
	RECT rectDefault, rect;
	char title[128];

	wc.style = NULL;							
	wc.lpfnWndProc = wndProc;																
	wc.cbClsExtra = 0;							
	wc.cbWndExtra = 0;							
	wc.hInstance = hInstance; 						
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMALG));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);  
	wc.hbrBackground = (HBRUSH) (1 + COLOR_BTNFACE); 
	wc.lpszMenuName = "MAIN_MENU";			
	wc.lpszClassName = "imsMainWindow";		

	InitCommonControls();	   
	
	if (!RegisterClass(&wc)) {
		return false;
	}
	
	gMilApplication = MappAlloc(M_DEFAULT, M_NULL);
	
	if (!gMilApplication) {
		return false;
	}

	rectDefault.left = 10;
	rectDefault.top = 1;
	rectDefault.right = rectDefault.left + 400;
	rectDefault.bottom = rectDefault.top + 100; 

	if (!loadWindowRect("MainWindow", &rect, &rectDefault)) {
		rect = rectDefault;
	}
	
	sprintf_s(title, sizeof(title), "%s %d.%d.%d", 
				(char *) SOFTWARE_NAME, (int) VER_MAJOR, (int) VER_MINOR, (int) VER_BUILD);

	ghWndMain = CreateWindow("imsMainWindow",	  
						title,	  
						WS_OVERLAPPEDWINDOW,						
						rect.left,	  
						rect.top,	
						rect.right - rect.left,
						rect.bottom - rect.top,
						NULL,   
						NULL,
						hInstance,	  
						NULL);  

	if (!ghWndMain) {
		MessageBox(NULL, "System error creating main application window!", "Fatal Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	ShowWindow(ghWndMain, cmdShow);
	UpdateWindow(ghWndMain);

	return true;
}

/*
  =======================================================================================  
  =======================================================================================
*/
LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
		case WM_PAINT:
			wmPaint(hWnd);
			break;
	
		case WM_COMMAND:
			wmCommand(hWnd, wParam, lParam);
			break;			  

		case WM_CLOSE:		
			wmClose(hWnd);
			break;

		case WM_OVERLAY_WINDOW_DESTROYED:
			if (gOverlayWindow) {
				delete gOverlayWindow;
				gOverlayWindow = NULL;
			}
			
			break;

		case WM_CREATE:
			wmCreate(hWnd);
			break;

		case WM_DESTROY:			
			saveWindowRect("MainWindow", hWnd); 
			PostQuitMessage(0);
			break;

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
void wmPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	char buff[64];
	HFONT oldFont;
	int oldBkMode;

	HDC hdc = BeginPaint(hWnd, &ps);

	if (gTimingOn) {
		oldFont = (HFONT) SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
		oldBkMode = SetBkMode(hdc, TRANSPARENT);

		sprintf_s(buff, sizeof(buff), "Elapsed Time: %u mSec", gElapsedTime);	
		TextOut(hdc, 4, 4, buff, strlen(buff));

		SelectObject(hdc, oldFont);
		SetBkMode(hdc, oldBkMode);
	}

	EndPaint(hWnd, &ps);
}

/*
  =======================================================================================  
  =======================================================================================
*/
void wmClose(HWND hWnd)
{	
	if (gOverlayWindow) {
		delete gOverlayWindow;
		gOverlayWindow = NULL;
	}

	freeMilVars();

	DestroyWindow(hWnd);
}

/*
  =======================================================================================  
  =======================================================================================
*/
void wmCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case ID_FILE_LOAD_IMAGE:
		loadImage(hWnd);
		break;

	case ID_FILE_LOAD_BACKGROUND_IMAGE:
		loadBackgroundImage(hWnd, false);
		break;

	case ID_FILE_LOAD_BACKGROUND_IMAGE_SMOOTHED:
		loadBackgroundImage(hWnd, true);
		break;

	case MSG_APPLY_IMAGE_OPS:
		if (gTimingOn) {
			gElapsedTime = 0;
			timeBeginPeriod(1);
		}

		MbufCopyColor(gMilRawOriginalBuff, gMilRawYUVBuff, M_ALL_BAND);
		doRawOpUpdates();
		
		if (gTimingOn) {		
			timeEndPeriod(1);
			InvalidateRect(hWnd, NULL, TRUE);
		}

		break;

	case ID_FILE_EXIT:
		PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;

	case ID_SAVE_MASKED_IMAGE:
		if (gMaskedImageShowing) {
			saveImage(hWnd, gMilMaskedImageBuff, "Save Masked Image");
		}

		break;

	case ID_SAVE_GREYSCALE_IMAGE:
		if (gGrayscaleImageShowing) {
			saveImage(hWnd, gMilGrayscaleBuff, "Save Grayscale Image");
		}

		break;

	case ID_SAVE_BINARIZED_IMAGE:
		if (gBinarizedImageShowing) {
			saveImage(hWnd, gMilBinarizedBuff, "Save Binarized Image");
		}

		break;
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void wmCreate(HWND hWnd)
{
	EnableMenuItem(GetMenu(hWnd), ID_FILE_LOAD_BACKGROUND_IMAGE, MF_GRAYED);	

	/*
	if (!allocMilVars(hWnd)) {
		MessageBox(hWnd, "Error initializing Matrox libraries.", "MIL Error", MB_ICONWARNING | MB_OK);

		EnableMenuItem(GetMenu(hWnd), ID_FILE_LOAD_IMAGE, MF_GRAYED);
		EnableMenuItem(GetMenu(hWnd), ID_FILE_LOAD_BACKGROUND_IMAGE, MF_GRAYED);
	}
	*/
}

/*
  =======================================================================================  
  =======================================================================================
*/
void updateDisplayTitle(MIL_ID milDisplay, int opType)
{
	char title[32];
	
	memset(title, 0, sizeof(title));

	if (opType == RAW_OPS) {
		strncpy_s(title, sizeof(title), "Raw Image", _TRUNCATE);
	}
	else if (opType == MASK_OPS) {
		strncpy_s(title, sizeof(title), "Masked Image", _TRUNCATE);
	}
	else if (opType == GRAYSCALE_OPS) {
		strncpy_s(title, sizeof(title), "Grayscale Image", _TRUNCATE);
	}
	else if (opType == BINARY_OPS) {
		strncpy_s(title, sizeof(title), "Binary Image", _TRUNCATE);
	}
	else {
		return;
	}

	MdispControl(milDisplay, M_TITLE, (long) title);
}

/*
  =======================================================================================  
  =======================================================================================
*/
void doRawOpUpdates()
{
	unsigned long startTime;

	MbufCopyColor(gMilRawOriginalBuff, gMilRawYUVBuff, M_ALL_BAND);

	if (gTimingOn) {
		startTime = timeGetTime();
	}

	gImageOps.processRawOps(gMilRawYUVBuff);

	if (gTimingOn) {
		gElapsedTime += timeGetTime() - startTime;
	}

	MdispSelect(gMilRawDisplay, gMilRawYUVBuff);
	updateDisplayTitle(gMilRawDisplay, RAW_OPS);	

	doMaskOpUpdates();
}

/*
  =======================================================================================  
  =======================================================================================
*/
void doMaskOpUpdates()
{
	unsigned long startTime;

	MbufClear(gMilTempBuff, 0.0);
	MbufCopyColor(gMilRawYUVBuff, gMilTempBuff, M_Y);

	if (gTimingOn) {
		startTime = timeGetTime();
	}

	gImageOps.processMaskOps(gMilTempBuff, gMilMaskedImageBuff, gMilBackgroundBuff);

	if (gTimingOn) {
		gElapsedTime += timeGetTime() - startTime;
	}
	
	MdispSelect(gMilMaskedDisplay, gMilMaskedImageBuff);
	updateDisplayTitle(gMilMaskedDisplay, MASK_OPS);
	gMaskedImageShowing = true;

	doGrayscaleOpUpdates();
}

/*
  =======================================================================================  
  =======================================================================================
*/
void doGrayscaleOpUpdates()
{
	unsigned long startTime;

	MbufCopy(gMilMaskedImageBuff, gMilGrayscaleBuff);
	
	if (gTimingOn) {
		startTime = timeGetTime();
	}

	gImageOps.processGrayscaleOps(gMilGrayscaleBuff, gMilLogBuff);

	if (gTimingOn) {
		gElapsedTime += timeGetTime() - startTime;
	}

	MdispSelect(gMilGrayscaleDisplay, gMilGrayscaleBuff);
	updateDisplayTitle(gMilGrayscaleDisplay, GRAYSCALE_OPS);
	gGrayscaleImageShowing = true;	

	doBinaryOpUpdates();			
}

/*
  =======================================================================================  
  =======================================================================================
*/
void doBinaryOpUpdates()
{
	unsigned long startTime;

	if (gTimingOn) {
		startTime = timeGetTime();
	}

	gImageOps.binarize(gMilGrayscaleBuff, gMilBinarizedBuff, gMilThresholdBuff);

	gImageOps.processBinaryOps(gMilBinarizedBuff);
	
	if (gTimingOn) {
		gElapsedTime += timeGetTime() - startTime;
	}

	MdispSelect(gMilBinarizedDisplay, gMilBinarizedBuff);
	updateDisplayTitle(gMilBinarizedDisplay, BINARY_OPS);
	gBinarizedImageShowing = true;
	
	if (gOverlayWindow) {				
		gOverlayWindow->updateWindow(gMilRawYUVBuff, gMilBinarizedBuff);
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void loadControls(HWND hWndParent)
{
	gImageOps.showDlg(hWndParent);	

	if (!gOverlayWindow) {
		gOverlayWindow = new OverlayWindow(hWndParent);
	}

	if (gOverlayWindow) {				
		gOverlayWindow->updateWindow(gMilRawYUVBuff, gMilBinarizedBuff);
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void saveImage(HWND hWnd, MIL_ID displayBuff, const char *prompt)
{
	OPENFILENAME ofn;
	char filename[MAX_PATH];

	memset(filename, 0, sizeof(filename));

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = "TIFF files (*.tif)\0*.tif\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename) - 1;
	ofn.lpstrTitle = prompt;
	ofn.lpstrDefExt = "tif";

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&ofn)) {
		return;
	}

	MbufExport(filename, M_TIFF, displayBuff);
}

/*
  =======================================================================================  
  int gImageWidth;
int gImageHeight;
bool gColorImage;
  =======================================================================================
*/
bool getImageProperties(const char *image_file)
{
	long bands;

	if (!file_exists(image_file)) {
		return false;
	}

	gImageWidth = MbufDiskInquire((MIL_TEXT_PTR)image_file, M_SIZE_X, M_NULL);

	if (M_INVALID == gImageWidth) {
		return false;
	}

	gImageHeight = MbufDiskInquire((MIL_TEXT_PTR)image_file, M_SIZE_Y, M_NULL);

	if (M_INVALID == gImageHeight) {
		return false;
	}

	bands = MbufDiskInquire((MIL_TEXT_PTR)image_file, M_SIZE_BAND, M_NULL);

	if (M_INVALID == bands) {
		return false;
	}

	gColorImage = (bands == 3) ? true : false;

	gImageDepth = MbufDiskInquire((MIL_TEXT_PTR)image_file, M_SIZE_BIT, M_NULL);

	if (M_INVALID == gImageDepth) {
		return false;
	}

	gImageSign = MbufDiskInquire((MIL_TEXT_PTR)image_file, M_SIGN, M_NULL);

	if (M_INVALID == gImageSign) {
		return false;
	}

	gFileFormat = MbufDiskInquire((MIL_TEXT_PTR)image_file, M_FILE_FORMAT, M_NULL);

	if (M_INVALID == gFileFormat) {
		return false;
	}

	return true;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void loadImage(HWND hWnd)
{
	OPENFILENAME ofn;
	char filename[MAX_PATH];
	char title[MAX_PATH + 32];
	
	memset(filename, 0, sizeof(filename));
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = "TIFF files (*.tif)\0*.tif\0JPG files (*.jpg)\0*.jpg\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename) - 1;
	ofn.lpstrTitle = "Load Image";
	ofn.Flags = OFN_FILEMUSTEXIST;

	if (!GetOpenFileName(&ofn)) {
		return;
	}
	
	if (!getImageProperties(filename)) {
		return;
	}

	if (!allocMilVars(hWnd)) {
		return;
	}

	MbufImport(filename, gFileFormat, M_LOAD, gMilSystem, &gMilRawOriginalBuff);
	//MbufImport(filename, M_JPEG, M_LOAD, gMilSystem, &gMilRawOriginalBuff);

	MbufCopyColor(gMilRawOriginalBuff, gMilRawYUVBuff, M_ALL_BAND);

	strncpy_s(title, sizeof(title), "Raw: ", _TRUNCATE);
	strncat_s(title, sizeof(title), filename, _TRUNCATE);
	MdispControl(gMilRawDisplay, M_TITLE, (long) title); 

	EnableMenuItem(GetMenu(hWnd), ID_FILE_LOAD_BACKGROUND_IMAGE, MF_ENABLED);

	if (!gImageShowing) {
		restoreDisplayWindowPrefs(hWnd, gMilRawDisplay, "MilRawDisplay");
	}

	MdispSelect(gMilRawDisplay, gMilRawYUVBuff);	

	if (!gImageShowing) {
		gImageShowing = true;

		if (gBackgroundImageShowing) {
			loadControls(hWnd);
		}
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void closeImage()
{
	if (gImageShowing) {
		if (gMilRawDisplay) {
			saveDisplayWindowPrefs(gMilRawDisplay, "MilRawDisplay");
			MdispSelect(gMilRawDisplay, M_NULL);
		}

		gImageShowing = false;
		EnableMenuItem(GetMenu(ghWndMain), ID_FILE_LOAD_BACKGROUND_IMAGE, MF_GRAYED);		
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void loadBackgroundImage(HWND hWnd, bool smooth)
{
	OPENFILENAME ofn;
	char filename[MAX_PATH];
	char title[MAX_PATH + 32];
	
	memset(filename, 0, sizeof(filename));
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = "TIFF files (*.tif)\0*.tif\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename) - 1;
	ofn.lpstrTitle = "Load Background Image";
	ofn.Flags = OFN_FILEMUSTEXIST;

	if (!GetOpenFileName(&ofn)) {
		return;
	}
	
	MbufImport(filename, M_TIFF, M_LOAD, gMilSystem, &gMilBackgroundBuff);
	
	if (smooth) {
		MimConvolve(gMilBackgroundBuff, gMilBackgroundBuff, M_SMOOTH);
	}

	strncpy_s(title, sizeof(title), "Background: ", _TRUNCATE);
	strncat_s(title, sizeof(title), filename, _TRUNCATE);
	MdispControl(gMilBackgroundDisplay, M_TITLE, (long) title); 

	if (!gBackgroundImageShowing) {
		restoreDisplayWindowPrefs(hWnd, gMilBackgroundDisplay, "MilBackgroundDisplay");
	}

	MdispSelect(gMilBackgroundDisplay, gMilBackgroundBuff);	

	if (!gBackgroundImageShowing) {
		gBackgroundImageShowing = true;

		if (gImageShowing) {
			loadControls(hWnd);
		}
	}

	//gImageOps.computeAdaptiveThreshold(gMilBackgroundBuff, gMilThresholdBuff);
}

/*
  =======================================================================================  
  =======================================================================================
*/
void closeBackgroundImage()
{
	if (gBackgroundImageShowing) {
		if (gMilBackgroundDisplay) {
			saveDisplayWindowPrefs(gMilBackgroundDisplay, "MilBackgroundDisplay");
			MdispSelect(gMilBackgroundDisplay, M_NULL);
		}

		gBackgroundImageShowing = false;
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void closeMaskedImage()
{
	if (gMaskedImageShowing) {
		if (gMilMaskedDisplay) {
			saveDisplayWindowPrefs(gMilMaskedDisplay, "MilMaskedDisplay");
			MdispSelect(gMilMaskedDisplay, M_NULL);
		}

		gMaskedImageShowing = false;
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void closeGrayscaleImage()
{
	if (gGrayscaleImageShowing) {
		if (gMilGrayscaleDisplay) {
			saveDisplayWindowPrefs(gMilGrayscaleDisplay, "MilGrayscaleDisplay");
			MdispSelect(gMilGrayscaleDisplay, M_NULL);
		}

		gGrayscaleImageShowing = false;
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void closeBinarizedImage()
{
	if (gBinarizedImageShowing) {
		if (gMilBinarizedDisplay) {
			saveDisplayWindowPrefs(gMilBinarizedDisplay, "MilBinarizedDisplay");
			MdispSelect(gMilBinarizedDisplay, M_NULL);
		}

		gBinarizedImageShowing = false;
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void closeAll()
{
	closeBinarizedImage();
	closeGrayscaleImage();
	closeMaskedImage();
	closeBackgroundImage();
	closeImage();
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool allocMilVars(HWND hWnd)
{
	if (gMilSystem) {
		return true;
	}

	gMilSystem = M_DEFAULT_HOST;

	gMilRawDisplay = MdispAlloc(gMilSystem, M_DEFAULT, M_DEF_DISPLAY_FORMAT, M_DEFAULT, M_NULL);

	if (!gMilRawDisplay) {
		return false;
	}

	MdispControl(gMilRawDisplay, M_WINDOW_SYSBUTTON, M_DISABLE);
	restoreDisplayWindowPrefs(hWnd, gMilRawDisplay, "MilRawDisplay");

	gMilBackgroundDisplay = MdispAlloc(gMilSystem, M_DEFAULT, M_DEF_DISPLAY_FORMAT, M_DEFAULT, M_NULL);

	if (!gMilBackgroundDisplay) {
		return false;
	}

	MdispControl(gMilBackgroundDisplay, M_WINDOW_SYSBUTTON, M_DISABLE);
	restoreDisplayWindowPrefs(hWnd, gMilBackgroundDisplay, "MilBackgroundDisplay");


	gMilMaskedDisplay = MdispAlloc(gMilSystem, M_DEFAULT, M_DEF_DISPLAY_FORMAT, M_DEFAULT, M_NULL);

	if (!gMilMaskedDisplay) {
		return false;
	}

	MdispControl(gMilMaskedDisplay, M_WINDOW_SYSBUTTON, M_DISABLE);
	restoreDisplayWindowPrefs(hWnd, gMilMaskedDisplay, "MilMaskedDisplay");


	gMilGrayscaleDisplay = MdispAlloc(gMilSystem, M_DEFAULT, M_DEF_DISPLAY_FORMAT, M_DEFAULT, M_NULL);

	if (!gMilGrayscaleDisplay) {
		return false;
	}

	MdispControl(gMilGrayscaleDisplay, M_WINDOW_SYSBUTTON, M_DISABLE);
	restoreDisplayWindowPrefs(hWnd, gMilGrayscaleDisplay, "MilGrayscaleDisplay");


	gMilBinarizedDisplay = MdispAlloc(gMilSystem, M_DEFAULT, M_DEF_DISPLAY_FORMAT, M_DEFAULT, M_NULL);

	if (!gMilBinarizedDisplay) {
		return false;
	}

	MdispControl(gMilBinarizedDisplay, M_WINDOW_SYSBUTTON, M_DISABLE);
	MdispControl(gMilBinarizedDisplay, M_TITLE, (long) "Binarized: None");
	restoreDisplayWindowPrefs(hWnd, gMilBinarizedDisplay, "MilBinarizedDisplay");
	
	gMilRawOriginalBuff = MbufAllocColor(gMilSystem, 3, gImageWidth, gImageHeight, gImageSign + gImageDepth,
									M_IMAGE | M_DISP | M_PROC | M_YUV16_YUYV + M_PACKED, M_NULL);

	if (!gMilRawOriginalBuff) {
		return false;
	}

	gMilRawYUVBuff = MbufAllocColor(gMilSystem, 3, gImageWidth, gImageHeight, gImageSign + gImageDepth,
									M_IMAGE | M_DISP | M_PROC | M_YUV16_YUYV + M_PACKED, M_NULL);

	if (!gMilRawYUVBuff) {
		return false;
	}

	gMilSingleYUVPlaneBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
										M_IMAGE | M_DISP | M_PROC, M_NULL);

	gMilBackgroundBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
								M_IMAGE | M_DISP | M_PROC, M_NULL);

	gMilThresholdBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
								M_IMAGE + M_DISP | M_PROC, M_NULL);

	gMilMaskedImageBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
								M_IMAGE | M_DISP | M_PROC, M_NULL);

	gMilTempBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
								M_IMAGE | M_DISP | M_PROC, M_NULL);

	gMilGrayscaleBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
								M_IMAGE | M_DISP | M_PROC, M_NULL);

	gMilLogBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
								M_IMAGE | M_PROC, M_NULL);

	gMilBinarizedBuff = MbufAlloc2d(gMilSystem, gImageWidth, gImageHeight, gImageSign + gImageDepth,
								M_IMAGE | M_DISP | M_PROC, M_NULL);


	setZoomLevel(-2.0);
	CheckMenuItem(GetMenu(hWnd), ID_ZOOM_ONE_HALF_X, MF_CHECKED);

	return true;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void setZoomLevel(double zoom)
{
	if (zoom != 1.0 && zoom != -2.0) {
		return;
	}

	if (gMilRawDisplay) {
		MdispZoom(gMilRawDisplay, zoom, zoom);
	}

	if (gMilBackgroundDisplay) {
		MdispZoom(gMilBackgroundDisplay, zoom, zoom);
	}

	if (gMilMaskedDisplay) {
		MdispZoom(gMilMaskedDisplay, zoom, zoom);
	}

	if (gMilGrayscaleDisplay) {
		MdispZoom(gMilGrayscaleDisplay, zoom, zoom);
	}

	if (gMilBinarizedDisplay) {
		MdispZoom(gMilBinarizedDisplay, zoom, zoom);
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void saveDisplayWindowPrefs(MIL_ID mil_display, const char *name)
{
	HWND hWndMil;

	if (!mil_display || !name || !*name) {
		return;
	}

	hWndMil = (HWND) MdispInquire(mil_display, M_WINDOW_HANDLE, M_NULL);

	if (hWndMil) {
		saveWindowRect(name, hWndMil);
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void restoreDisplayWindowPrefs(HWND hWndParent, MIL_ID mil_display, const char *name)
{
	RECT rect, rectDefault;

	if (!mil_display || !name || !*name) {
		return;
	}

	GetWindowRect(hWndParent, &rect);

	rectDefault.left = rect.left + 2;
	rectDefault.top = 1;
	rectDefault.right = rectDefault.left;
	rectDefault.bottom = rectDefault.top;

	loadWindowRect(name, &rect, &rectDefault);

	MdispControl(mil_display, M_WINDOW_INITIAL_POSITION_X, rect.left);
	MdispControl(mil_display, M_WINDOW_INITIAL_POSITION_Y, rect.top);	
}

/*
  =======================================================================================  
  =======================================================================================
*/
void freeMilVars()
{
	closeBinarizedImage();
	closeGrayscaleImage();
	closeMaskedImage();
	closeBackgroundImage();
	closeImage();

	if (gMilBinarizedBuff) {
		MbufFree(gMilBinarizedBuff);
		gMilBinarizedBuff = 0;
	}

	if (gMilTempBuff) {
		MbufFree(gMilTempBuff);
		gMilTempBuff = 0;
	}
	
	if (gMilLogBuff) {
		MbufFree(gMilLogBuff);
		gMilLogBuff = 0;
	}

	if (gMilGrayscaleBuff) {
		MbufFree(gMilGrayscaleBuff);
		gMilGrayscaleBuff = 0;
	}

	if (gMilMaskedImageBuff) {
		MbufFree(gMilMaskedImageBuff);
		gMilMaskedImageBuff = 0;
	}

	if (gMilThresholdBuff) {
		MbufFree(gMilThresholdBuff);
		gMilThresholdBuff = 0;
	}

	if (gMilBackgroundBuff) {
		MbufFree(gMilBackgroundBuff);
		gMilBackgroundBuff = 0;
	}

	if (gMilSingleYUVPlaneBuff) {
		MbufFree(gMilSingleYUVPlaneBuff);
		gMilSingleYUVPlaneBuff = 0;
	}

	if (gMilRawYUVBuff) {
		MbufFree(gMilRawYUVBuff);
		gMilRawYUVBuff = 0;
	}

	if (gMilRawOriginalBuff) {
		MbufFree(gMilRawOriginalBuff);
		gMilRawOriginalBuff = 0;
	}

	if (gMilBinarizedDisplay) {
		MdispFree(gMilBinarizedDisplay);
		gMilBinarizedDisplay = 0;
	}

	if (gMilGrayscaleDisplay) {
		MdispFree(gMilGrayscaleDisplay);
		gMilGrayscaleDisplay = 0;
	}

	if (gMilMaskedDisplay) {
		MdispFree(gMilMaskedDisplay);
		gMilMaskedDisplay = 0;
	}
	
	if (gMilBackgroundDisplay) {
		MdispFree(gMilBackgroundDisplay);
		gMilBackgroundDisplay = 0;
	}

	if (gMilRawDisplay) {
		MdispFree(gMilRawDisplay);
		gMilRawDisplay = 0;
	}

	gMilSystem = 0;
}

