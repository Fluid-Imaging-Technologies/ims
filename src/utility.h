/*
 * $Id: utility.h,v 1.1.1.1 2008/10/10 11:05:51 scott Exp $
 *
 */

#ifndef UTILITY_H
#define UTILITY_H

#include "version.h"

#define PI ((double)3.1415926)

extern char gIniFilename[MAX_PATH];

double computeCircularity(double area, double perimeter);

bool loadWindowRect(const char *windowName, RECT *rect, RECT *rectDefault);
bool saveWindowRect(const char *windowName, HWND hWnd); 
bool loadIniFilename();
bool directory_exists(const char *dirname);
bool file_exists(const char *filename);
char *newStrDup(const char *src);
void showLastError(const char *lpszFunction);

#endif // ifndef UTILITY_H
