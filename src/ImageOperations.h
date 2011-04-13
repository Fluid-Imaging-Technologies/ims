/*
 * $Id: ImageOperations.h,v 1.11 2009/04/30 19:01:05 scott Exp $
 *
 */

#ifndef IMAGE_OPERATIONS_H
#define IMAGE_OPERATIONS_H

#include "version.h"
#include "StringList.h"
#include <mil.h>

#define MSG_APPLY_IMAGE_OPS (WM_USER + 3333)

#define NUM_CONTRAST_OPTIONS 5
#define NUM_STORED_CONFIGURATIONS 5

class ImageOpConfig
{
public:
	ImageOpConfig();
	ImageOpConfig& operator=(ImageOpConfig &rhs);

	void setDefaults();

	StringList _rawList;
	StringList _maskList;
	StringList _grayscaleList;
	StringList _binaryList;
	int _contrastIncrease;
	double _segmentationThreshold;
	double _segmentationThresholdAuto;
	bool _useAutoThreshold;
	bool _useAdaptiveThreshold;
	int _adaptiveThresholdNeighborhood;
};


class ImageOperations
{
public:
	ImageOperations();
	~ImageOperations();

	void showDlg(HWND hWndParent);
	
	bool processRawOps(MIL_ID milBuffer);
	bool processMaskOps(MIL_ID srcBuffer, MIL_ID dstBuffer, MIL_ID bkgdBuffer);
	void increaseContrast(MIL_ID milBuffer);
	bool processGrayscaleOps(MIL_ID milBuffer, MIL_ID edgeBuffer);
	void binarize(MIL_ID srcBuffer, MIL_ID dstBuffer, MIL_ID threshBuffer);
	bool processBinaryOps(MIL_ID milBuffer);
	void computeAdaptiveThreshold(MIL_ID bkgdBuffer, MIL_ID threshBuffer);

private:
	void wmDestroy(HWND hWnd);

	bool showListDlg(HWND hWndParent, StringList *availableList, StringList *currentList);
	void addListItems(StringList *src, StringList *dst);
	bool populateCSVList(char *buff, int maxLen, StringList *srcList);
	void loadListFromCSV(const char *srcBuff, StringList *dstList, StringList *availList, int maxEntries);
	void populateDlgControlsWithConfig(HWND hDlg, int configNum);
	void populateConfigWithDlgControls(HWND hDlg, int configNum);

	double computeAutoThreshold(MIL_ID maskedBuffer);
	void setAutoThreshold(double thresh);
	int neighborhoodAverageIntensity(unsigned char *src, int pos, int pitch, int size, int neighborhood);

	void apply();

	bool saveSetup(int n);
	bool loadSetup(int n);

	static BOOL CALLBACK ImageOperationsDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK OpListDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static int contrastAdjustments[NUM_CONTRAST_OPTIONS];

	HWND _hWndParent;
	HWND _hDlg;

	StringList _rawAvailableList;
	StringList _maskAvailableList;
	StringList _grayscaleAvailableList;
	StringList _binaryAvailableList;
	
	ImageOpConfig _config[NUM_STORED_CONFIGURATIONS];

	StringList _opList;
	StringList _availableList;
	char _opDlgTitle[64];
	int _powLut[5][256];
};

#endif // ifndef IMAGE_OPERATIONS_H
