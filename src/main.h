/*
 * $Id: main.h,v 1.7 2009/04/27 19:17:54 scott Exp $
 *
 */

#ifndef MAIN_H
#define MAIN_H

#include "version.h"
#include <mil.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow);
bool initApplication(HINSTANCE hInstance, int cmdShow);
LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void wmPaint(HWND hWnd);
void wmClose(HWND hWnd);
void wmCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
void wmCreate(HWND hWnd);
void loadImage(HWND hWnd);
void closeImage();
void loadBackgroundImage(HWND hWnd, bool smooth);
void closeBackgroundImage();
void closeMaskedImage();
void closeGrayscaleImage();
void closeBinarizedImage();
void closeAll();
void setZoomLevel(double zoom);

void saveImage(HWND hWnd, MIL_ID displayBuff, const char *prompt);

void saveDisplayWindowPrefs(MIL_ID mil_display, const char *name);
void restoreDisplayWindowPrefs(HWND hWndParent, MIL_ID mil_display, const char *name);
bool allocMilVars(HWND hWnd);
void freeMilVars();

void updateDisplayTitle(MIL_ID milDisplay, int opType);

void doRawOpUpdates();
void doMaskOpUpdates();
void doGrayscaleOpUpdates();
void doBinaryOpUpdates();

#endif // ifndef MAIN_H
