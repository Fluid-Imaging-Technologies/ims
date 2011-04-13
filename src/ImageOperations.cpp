/*
 * $Id: ImageOperations.cpp,v 1.12 2009/04/30 19:01:05 scott Exp $
 *
 */

#include <math.h>

#include "ImageOperations.h"
#include "utility.h"
#include "resource.h"

#define DEFAULT_SEGMENTATION_THRESHOLD 35.0

int ImageOperations::contrastAdjustments[NUM_CONTRAST_OPTIONS] = { 255, 160, 128, 96, 64 };


#define CONTRAST_POW_6		4
#define CONTRAST_POW_7		3
#define CONTRAST_POW_8		2
#define CONTRAST_POW_9		1


/*
  =======================================================================================  
  =======================================================================================
*/
ImageOpConfig::ImageOpConfig()
{
	setDefaults();
}

/*
  =======================================================================================  
  =======================================================================================
*/
ImageOpConfig& ImageOpConfig::operator=(ImageOpConfig &rhs)
{
	if (this != &rhs) {
		_rawList = rhs._rawList;
		_maskList = rhs._maskList;
		_grayscaleList = rhs._grayscaleList;
		_binaryList = rhs._binaryList;
		_contrastIncrease = rhs._contrastIncrease;
		_segmentationThreshold = rhs._segmentationThreshold;
		_segmentationThresholdAuto = rhs._segmentationThresholdAuto;
		_useAutoThreshold = rhs._useAutoThreshold;
		_useAdaptiveThreshold = rhs._useAdaptiveThreshold;
		_adaptiveThresholdNeighborhood = rhs._adaptiveThresholdNeighborhood;
	}

	return *this;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOpConfig::setDefaults()
{
	_rawList.clearList();
	_maskList.addString("Pixels Darker");
	_grayscaleList.clearList();
	_binaryList.clearList();
	_contrastIncrease = 0;
	_segmentationThreshold = DEFAULT_SEGMENTATION_THRESHOLD;
	_segmentationThresholdAuto = 0.0;
	_useAutoThreshold = false;
	_useAdaptiveThreshold = false;
	_adaptiveThresholdNeighborhood = 1;
}

/*
  =======================================================================================  
  =======================================================================================
*/
ImageOperations::ImageOperations()
{
	double c;

	_hDlg = 0;

	_rawAvailableList.addString("Sharpen Aggressive");
	_rawAvailableList.addString("Sharpen");
	_rawAvailableList.addString("Smooth");

	_maskAvailableList.addString("Pixels Darker");
	_maskAvailableList.addString("Pixels Lighter");	
	_maskAvailableList.addString("All Pixels Different");

	_grayscaleAvailableList.addString("Open");
	_grayscaleAvailableList.addString("Close");
	_grayscaleAvailableList.addString("Smooth");
	_grayscaleAvailableList.addString("Sobel");
	_grayscaleAvailableList.addString("Sobel+");
	_grayscaleAvailableList.addString("Prewitt");
	_grayscaleAvailableList.addString("Prewitt+");
	
	_binaryAvailableList.addString("Open");
	_binaryAvailableList.addString("Close");

	memset(_opDlgTitle, 0, sizeof(_opDlgTitle));

	c = 255.0 / pow(256.0, 0.6);

	for (int i = 0; i < 256; i++) {
		_powLut[CONTRAST_POW_6][i] = (int)(c * pow((double)i, 0.6));
	}

	c = 255.0 / pow(256.0, 0.7);

	for (int i = 0; i < 256; i++) {
		_powLut[CONTRAST_POW_7][i] = (int)(c * pow((double)i, 0.7));
	}

	c = 255.0 / pow(256.0, 0.8);

	for (int i = 0; i < 256; i++) {
		_powLut[CONTRAST_POW_8][i] = (int)(c * pow((double)i, 0.8));
	}

	c = 255.0 / pow(256.0, 0.9);

	for (int i = 0; i < 256; i++) {
		_powLut[CONTRAST_POW_9][i] = (int)(c * pow((double)i, 0.9));
	}	
}


/*
  =======================================================================================  
  =======================================================================================
*/
ImageOperations::~ImageOperations()
{
}

/*
  =======================================================================================  
  	StringList _rawList;
	StringList _maskList;
	StringList _grayscaleList;
	StringList _binaryList;
	int _contrastIncrease;
	double _segmentationThreshold;
  =======================================================================================
*/
bool ImageOperations::saveSetup(int n)
{
	char section[16], buff[256];
	
	if (!loadIniFilename()) {
		return false;
	}

	if (n < 0 || n >= NUM_STORED_CONFIGURATIONS) {
		return false;
	}

	sprintf_s(section, sizeof(section), "ImageOpSetup%d", n);
	
	memset(buff, 0, sizeof(buff));
	populateCSVList(buff, sizeof(buff), &_config[n]._rawList);
	WritePrivateProfileString(section, "RawOps", buff, gIniFilename);

	memset(buff, 0, sizeof(buff));
	populateCSVList(buff, sizeof(buff), &_config[n]._maskList);
	WritePrivateProfileString(section, "MaskOps", buff, gIniFilename);

	memset(buff, 0, sizeof(buff));
	populateCSVList(buff, sizeof(buff), &_config[n]._grayscaleList);
	WritePrivateProfileString(section, "GrayscaleOps", buff, gIniFilename);

	memset(buff, 0, sizeof(buff));
	populateCSVList(buff, sizeof(buff), &_config[n]._binaryList);
	WritePrivateProfileString(section, "BinaryOps", buff, gIniFilename);

	sprintf_s(buff, sizeof(buff), "%d", _config[n]._contrastIncrease);
	WritePrivateProfileString(section, "ContrastIncrease", buff, gIniFilename);

	sprintf_s(buff, sizeof(buff), "%0.0lf", _config[n]._segmentationThreshold);
	WritePrivateProfileString(section, "SegmentationThreshold", buff, gIniFilename);

	WritePrivateProfileString(section, "UseAutoThreshold", _config[n]._useAutoThreshold ? "1" : "0", gIniFilename);
	WritePrivateProfileString(section, "UseAdaptiveThreshold", _config[n]._useAdaptiveThreshold ? "1" : "0", gIniFilename);

	sprintf_s(buff, sizeof(buff), "%d", _config[n]._adaptiveThresholdNeighborhood);
	WritePrivateProfileString(section, "AdaptiveThresholdNeighborhood", buff, gIniFilename);
	
	return true;
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool ImageOperations::loadSetup(int n)
{
	char section[16], buff[256];
	int val;

	if (!loadIniFilename()) {
		return false;
	}

	if (n < 0 || n >= NUM_STORED_CONFIGURATIONS) {
		return false;
	}

	sprintf_s(section, sizeof(section), "ImageOpSetup%d", n);

	memset(buff, 0, sizeof(buff));
	GetPrivateProfileString(section, "RawOps", NULL, buff, sizeof(buff) - 1, gIniFilename);
	loadListFromCSV(buff, &_config[n]._rawList, &_rawAvailableList, _rawAvailableList.getNumStrings());

	memset(buff, 0, sizeof(buff));
	GetPrivateProfileString(section, "MaskOps", NULL, buff, sizeof(buff) - 1, gIniFilename);
	loadListFromCSV(buff, &_config[n]._maskList, &_maskAvailableList, 1);

	memset(buff, 0, sizeof(buff));
	GetPrivateProfileString(section, "GrayscaleOps", NULL, buff, sizeof(buff) - 1, gIniFilename);
	loadListFromCSV(buff, &_config[n]._grayscaleList, &_grayscaleAvailableList, _grayscaleAvailableList.getNumStrings());

	memset(buff, 0, sizeof(buff));
	GetPrivateProfileString(section, "BinaryOps", NULL, buff, sizeof(buff) - 1, gIniFilename);
	loadListFromCSV(buff, &_config[n]._binaryList, &_binaryAvailableList, _binaryAvailableList.getNumStrings());

	val = GetPrivateProfileInt(section, "ContrastIncrease", 0, gIniFilename);

	if (val < 0 || val >= NUM_CONTRAST_OPTIONS) {
		val = 0;
	}

	_config[n]._contrastIncrease = val;

	val = GetPrivateProfileInt(section, "SegmentationThreshold", 35, gIniFilename);

	if (val < 0 || val > 255) {
		val = 35;
	}

	_config[n]._segmentationThreshold = val;

	_config[n]._useAutoThreshold = (1 == GetPrivateProfileInt(section, "UseAutoThreshold", 0, gIniFilename));
	_config[n]._useAdaptiveThreshold = (1 == GetPrivateProfileInt(section, "UseAdaptiveThreshold", 0, gIniFilename));

	val = GetPrivateProfileInt(section, "AdaptiveThresholdNeighborhood", 1, gIniFilename);

	if (val < 1) {
		val = 1;
	}
	else if (val > 10) {
		val = 10;
	}

	_config[n]._adaptiveThresholdNeighborhood = val;

	return true;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::loadListFromCSV(const char *srcBuff, StringList *dstList, 
									  StringList *availList, int maxEntries)
{
	char buff[256];
	char *s, *next_token;
	
	if (strlen(srcBuff) < 1) {
		return;
	}

	dstList->clearList();

	strncpy_s(buff, sizeof(buff), srcBuff, _TRUNCATE);

	s = strtok_s(buff, ",", &next_token);

	while (s) {
		if (availList->findString(s) >= 0) {
			dstList->addString(s);
		}
		
		if (dstList->getNumStrings() >= maxEntries) {
			break;
		}

		s = strtok_s(NULL, ",", &next_token);
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::apply()
{
	PostMessage(_hWndParent, WM_COMMAND, MSG_APPLY_IMAGE_OPS, 0);
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool ImageOperations::processRawOps(MIL_ID milBuffer)
{
	bool result;
	const char *op;
	int op_cnt;

	result = true;

	op_cnt = _config[0]._rawList.getNumStrings();

	for (int i = 0; i < op_cnt; i++) {
		op = _config[0]._rawList.getString(i);

		if (op && *op) {
			if (!_strcmpi(op, "Sharpen Aggressive")) {
				MimConvolve(milBuffer, milBuffer, M_SHARPEN);
				MimConvolve(milBuffer, milBuffer, M_SMOOTH);
				MimConvolve(milBuffer, milBuffer, M_SMOOTH);
			}
			else if (!_strcmpi(op, "Sharpen")) {
				MimConvolve(milBuffer, milBuffer, M_SHARPEN2);
				MimConvolve(milBuffer, milBuffer, M_SMOOTH);
			}
			else if (!_strcmpi(op, "Smooth")) {
				MimConvolve(milBuffer, milBuffer, M_SMOOTH);
			}
			else {
				result = false;
				break;
			}
		}
		else {
			result = false;
			break;
		}
	}

	return result;
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool ImageOperations::processMaskOps(MIL_ID srcBuffer, MIL_ID dstBuffer, MIL_ID bkgdBuffer)
{
	bool result;
	const char *op;
	
	result = true;

	op = _config[0]._maskList.getString(0);

	if (op && *op) {
		if (!_strcmpi(op, "Pixels Darker")) {
			MimArith(bkgdBuffer, srcBuffer, dstBuffer, M_SUB + M_SATURATION);
		}
		else if (!_strcmpi(op, "Pixels Lighter")) {
			MimArith(srcBuffer, bkgdBuffer, dstBuffer, M_SUB + M_SATURATION);
		}		
		else if (!_strcmpi(op, "All Pixels Different")) {
			MimArith(srcBuffer, bkgdBuffer, dstBuffer, M_SUB_ABS + M_SATURATION);
		}
		else {
			result = false;
		}
	}
	else {
		result = false;
	}

	return result;
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool ImageOperations::processGrayscaleOps(MIL_ID milBuffer, MIL_ID edgeBuffer)
{
	bool result;
	const char *op;
	int op_cnt;
	
	result = true;

	op_cnt = _config[0]._grayscaleList.getNumStrings();

	if (_config[0]._contrastIncrease > 0) {
		increaseContrast(milBuffer);

		if (op_cnt == 0) {
			// need an update if we aren't calling any MIL functions
			MbufControl(milBuffer, M_MODIFIED, M_DEFAULT);
		}
	}

	for (int i = 0; i < op_cnt; i++) {
		op = _config[0]._grayscaleList.getString(i);

		if (op && *op) {
			if (!_strcmpi(op, "Smooth")) {
				MimConvolve(milBuffer, milBuffer, M_SMOOTH);
			}
			else if (!_strcmpi(op, "Open")) {
				MimOpen(milBuffer, milBuffer, 1, M_GRAYSCALE);
			}
			else if (!_strcmpi(op, "Close")) {
				MimClose(milBuffer, milBuffer, 1, M_GRAYSCALE);
			}
			/*
			else if (!_strcmpi(op, "Increase Contrast")) {
				increaseContrast(milBuffer);
			}
			else if (!_strcmpi(op, "Sharpen Aggressive")) {
				MimConvolve(milBuffer, milBuffer, M_SHARPEN);
			}			
			else if (!_strcmpi(op, "Sharpen")) {
				MimConvolve(milBuffer, milBuffer, M_SHARPEN2);
			}
			*/
			else if (!_strcmpi(op, "Sobel")) {
				MimConvolve(milBuffer, milBuffer, M_EDGE_DETECT);
			}
			else if (!_strcmpi(op, "Sobel+")) {
				MbufCopy(milBuffer, edgeBuffer);
				MimConvolve(edgeBuffer, edgeBuffer, M_EDGE_DETECT);
				MimArith(milBuffer, edgeBuffer, milBuffer, M_MAX);
			}
			else if (!_strcmpi(op, "Prewitt")) {
				MimConvolve(milBuffer, milBuffer, M_EDGE_DETECT2);
			}
			else if (!_strcmpi(op, "Prewitt+")) {
				MbufCopy(milBuffer, edgeBuffer);
				MimConvolve(edgeBuffer, edgeBuffer, M_EDGE_DETECT2);
				MimArith(milBuffer, edgeBuffer, milBuffer, M_MAX);
			}

			/*
			else if (!_strcmpi(op, "Shen")) {
				MimConvolve(milBuffer, milBuffer, M_SHEN_FILTER(M_EDGE_DETECT, M_DEFAULT));
			}
			else if (!_strcmpi(op, "Deriche")) {
				MimConvolve(milBuffer, milBuffer, M_DERICHE_FILTER(M_EDGE_DETECT, M_DEFAULT));
			}		
			else if (!_strcmpi(op, "Multiply")) {
				if (multiplierValue > 0.0) {
					MimArith(milBuffer, multiplierValue, milBuffer, M_MULT_CONST + M_SATURATION);
				}
				else {
					result = false;
				}
			}
			*/
			else {
				result = false;
				break;
			}
		}
		else {
			result = false;
			break;
		}
	}

	if (result) {
		computeAutoThreshold(milBuffer);
	}

	return result;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::increaseContrast(MIL_ID milBuffer)
{
	unsigned char *p;
	int pitch, size, i, j, pow_index;
	//double m;

	if (_config[0]._contrastIncrease < CONTRAST_POW_9 || _config[0]._contrastIncrease > CONTRAST_POW_6) {
		return;
	}

	MbufInquire(milBuffer, M_HOST_ADDRESS, &p);
	pitch = MbufInquire(milBuffer, M_PITCH, M_NULL);
	size = MbufInquire(milBuffer, M_SIZE_BYTE, M_NULL);

	/*
	if (_config[0]._contrastIncrease < (NUM_CONTRAST_OPTIONS - 1)) {
		high = ImageOperations::contrastAdjustments[_config[0]._contrastIncrease];

		if (high == 0) {
			return;
		}
	
		m = 255.0 / (double) high;

		i = 0;

		while (i < size) {
			j = i + pitch;

			for (; i < j; i++) {
				val = p[i];

				if (val > 0) {
					if (val >= high) {
						val = 255;
					}
					else {
						val = (int) (m * (double) val);
					}
				
					p[i] = val;
				}
			}
		}
	}
	// log increase
	else {
	*/
	i = 0;
	pow_index = _config[0]._contrastIncrease;

	while (i < size) {
		j = i + pitch;

		for (; i < j; i++) {
			p[i] = _powLut[pow_index][p[i]];
		}
	}	
}

/*
  ======================================================================================= 
  [0] = particle
  [1] = background
  =======================================================================================
*/
double ImageOperations::computeAutoThreshold(MIL_ID maskedBuffer)
{
	unsigned char *p;
	int pitch, size, i, j;
	double new_thresh, old_thresh, avg[2];
	int ithresh, count[2];
	bool done;

	// starting point
	//if (_config[0]._segmentationThresholdAuto > 0.0) {
	//	thresh = _config[0]._segmentationThresholdAuto;
	//}
	//else 
	if (_config[0]._segmentationThreshold > 0.0) {
		old_thresh = _config[0]._segmentationThreshold;
	}
	else {
		old_thresh = DEFAULT_SEGMENTATION_THRESHOLD;
	}

	MbufInquire(maskedBuffer, M_HOST_ADDRESS, &p);
	pitch = MbufInquire(maskedBuffer, M_PITCH, M_NULL);
	size = MbufInquire(maskedBuffer, M_SIZE_BYTE, M_NULL);

	done = false;

	while (!done) {
		done = true;
		ithresh = (int) old_thresh;
		count[0] = count[1] = 0;
		avg[0] = avg[1] = 0.0;

		i = 0;

		while (i < size) {
			j = i + pitch;

			for (; i < j; i++) {
				if (p[i] > ithresh) {
					avg[0] += p[i];
					count[0]++;
				}
				else {
					avg[1] += p[i];
					count[1]++;
				}			
			}
		}
		
		for (j = 0; j < 2; j++) {
			if (count[j] > 0) {
				avg[j] /= (double) count[j];
			}
			else {
				avg[j] = 0.0;
			}
		}

		new_thresh = (avg[0] + avg[1]) / 2.0;

		if (new_thresh != old_thresh) {
			done = false;
		}

		old_thresh = new_thresh;
	}

	setAutoThreshold(old_thresh);

	return old_thresh;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::setAutoThreshold(double thresh)
{
	_config[0]._segmentationThresholdAuto = thresh;

	if (_hDlg) {
		SetDlgItemInt(_hDlg, IDC_SEGMENTATION_THRESHOLD_AUTO, (int) thresh, TRUE);
	}
}

/*
  ======================================================================================= 
  [0] = particle
  [1] = background
  =======================================================================================
*/
void ImageOperations::computeAdaptiveThreshold(MIL_ID bkgdBuffer, MIL_ID threshBuffer)
{
	unsigned char *p, *q;
	int pitch, size, i, j;
	
	MbufInquire(bkgdBuffer, M_HOST_ADDRESS, &p);
	pitch = MbufInquire(bkgdBuffer, M_PITCH, M_NULL);
	size = MbufInquire(bkgdBuffer, M_SIZE_BYTE, M_NULL);
	MbufInquire(threshBuffer, M_HOST_ADDRESS, &q);

	i = 0;

	while (i < size) {
		j = i + pitch;

		for (; i < j; i++) {
			q[i] = neighborhoodAverageIntensity(p, i, pitch, size, _config[0]._adaptiveThresholdNeighborhood);
		}
	}
}

/*
  =======================================================================================  
	(pixel - 1) - pitch | pixel	+ pitch | (pixel + 1) - pitch 		
	pixel - 1			| pixel			| pixel + 1
	(pixel - 1) + pitch | pixel + pitch	| (pixel + 1) + pitch		
  =======================================================================================
*/
int ImageOperations::neighborhoodAverageIntensity(unsigned char *src, int pos, int pitch, int size, int neighborhood)
{
	int k, row, col, total, count;
	int row_min, row_max, col_min, col_max;

	total = 0; 
	count = 0;

	row = pos / pitch;

	row_min = row - neighborhood;

	if (row_min < 0) {
		row_min = 0;
	}

	row_max = row + neighborhood + 1;

	if (row_max > (size / pitch)) {
		row_max = size / pitch;
	}

	col = pos % pitch;

	col_min = col - neighborhood;

	if (col_min < 0) {
		col_min = 0;
	}

	col_max = col + neighborhood;

	if (col_max > pitch) {
		col_max = pitch;
	}

	for (row = row_min; row < row_max; row++) {
		for (col = col_min; col < col_max; col++) {
			k = col + (row * pitch);
			total += src[k];
			count++;
		}
	}

	if (count > 0) {
		total /= count;
	}

	return total;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::binarize(MIL_ID srcBuffer, MIL_ID dstBuffer, MIL_ID threshBuffer)
{
	unsigned char *src, *dst, *thresh;
	int pitch, size, i, j, iseg;
	
	if (_config[0]._useAdaptiveThreshold && threshBuffer) {
		MbufInquire(srcBuffer, M_HOST_ADDRESS, &src);
		MbufInquire(dstBuffer, M_HOST_ADDRESS, &dst);
		MbufInquire(threshBuffer, M_HOST_ADDRESS, &thresh);

		pitch = MbufInquire(srcBuffer, M_PITCH, M_NULL);
		size = MbufInquire(srcBuffer, M_SIZE_BYTE, M_NULL);

		i = 0;
		iseg = (int) _config[0]._segmentationThreshold;

		while (i < size) {
			j = i + pitch;

			for (; i < j; i++) {
				if (src[i] >= thresh[i] + iseg) {
					dst[i] = 255;
				}
				else {
					dst[i] = 0;
				}
			}
		}
	}
	else if (_config[0]._useAutoThreshold) {
		MimBinarize(srcBuffer, dstBuffer, M_GREATER_OR_EQUAL, _config[0]._segmentationThresholdAuto, 0.0);
	}
	else {
		MimBinarize(srcBuffer, dstBuffer, M_GREATER_OR_EQUAL, _config[0]._segmentationThreshold, 0.0);
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool ImageOperations::processBinaryOps(MIL_ID milBuffer)
{
	bool result;
	const char *op;
	int op_cnt;

	result = true;

	op_cnt = _config[0]._binaryList.getNumStrings();

	for (int i = 0; i < op_cnt; i++) {
		op = _config[0]._binaryList.getString(i);

		if (op && *op) {
			if (!_strcmpi(op, "Sobel")) {
				MimConvolve(milBuffer, milBuffer, M_EDGE_DETECT);
			}
			else if (!_strcmpi(op, "Prewitt")) {
				MimConvolve(milBuffer, milBuffer, M_EDGE_DETECT2);
			}
			else if (!_strcmpi(op, "Open")) {
				MimOpen(milBuffer, milBuffer, 1, M_BINARY);
			}
			else if (!_strcmpi(op, "Close")) {
				MimClose(milBuffer, milBuffer, 1, M_BINARY);
			}
			else if (!_strcmpi(op, "Dilate")) {
				MimDilate(milBuffer, milBuffer, 1, M_BINARY);
			}
			else if (!_strcmpi(op, "Erode")) {
				MimErode(milBuffer, milBuffer, 1, M_BINARY);
			}
			else {
				result = false;
				break;
			}
		}
		else {
			result = false;
			break;
		}
	}

	return result;
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::showDlg(HWND hWndParent)
{
	RECT rect, rectDefault;

	if (_hDlg) {
		SetWindowPos(_hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else {
		_hWndParent = hWndParent;
		
		for (int i = 0; i < NUM_STORED_CONFIGURATIONS; i++) {
			loadSetup(i);
		}

		_hDlg = CreateDialogParam(GetModuleHandle(NULL), 
									MAKEINTRESOURCE(IDD_CONTEXT_SETUP), 
									hWndParent,
									ImageOperations::ImageOperationsDlg,
									(LPARAM) this);
		
		if (_hDlg) {
			rectDefault.top = 12;
			rectDefault.left = 11;

			ShowWindow(_hDlg, SW_SHOWNORMAL);

			if (loadWindowRect("ImageOpWindow", &rect, &rectDefault)) {
				// only if we really read some saved settings do we reposition
				if (rect.top != 12 && rect.left != 11) {
					SetWindowPos(_hDlg, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_NOSIZE);
				}
			}
		}
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::addListItems(StringList *src, StringList *dst)
{
	int cnt;
	const char *s;

	dst->clearList();

	cnt = src->getNumStrings();

	if (cnt > 0) {
		dst->addString(src->getString(0));

		for (int i = 1; i < cnt; i++) {
			s = src->getString(i);

			if (-1 == dst->findString(s)) {
				dst->addString(s);
			}
		}
	}
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool ImageOperations::populateCSVList(char *buff, int maxLen, StringList *srcList)
{
	const char *s;
	int cnt, i;

	if (!buff || maxLen < 64 || !srcList) {
		return false;
	}

	memset(buff, 0, maxLen);

	if (srcList) {
		cnt = srcList->getNumStrings();

		for (i = 0; i < cnt; i++) {
			s = srcList->getString(i);

			if (s) {
				if (i != 0) {
					strncat_s(buff, maxLen, ", ", _TRUNCATE);
				}

				strncat_s(buff, maxLen, s, _TRUNCATE);
			}
		}
	}

	return true;	
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::wmDestroy(HWND hWnd)
{
	for (int i = 0; i < NUM_STORED_CONFIGURATIONS; i++) {
		saveSetup(i);
	}

	saveWindowRect("ImageOpWindow", hWnd);
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::populateDlgControlsWithConfig(HWND hDlg, int configNum)
{
	char buff[256];
	int i;
	HWND ctl;

	if (populateCSVList(buff, sizeof(buff), &_config[configNum]._rawList)) {
		SetDlgItemText(hDlg, IDC_RAW_LIST, buff);
	}

	if (populateCSVList(buff, sizeof(buff), &_config[configNum]._grayscaleList)) {
		SetDlgItemText(hDlg, IDC_GRAYSCALE_LIST, buff);
	}

	if (populateCSVList(buff, sizeof(buff), &_config[configNum]._binaryList)) {
		SetDlgItemText(hDlg, IDC_BINARY_LIST, buff);
	}

	i = _maskAvailableList.findString(_config[configNum]._maskList.getString(0));

	if (i < 0) {
		i = 0;
	}

	SendMessage(GetDlgItem(hDlg, IDC_BACKGROUND_ELIMINATION), CB_SETCURSEL, i, 0);

	ctl = GetDlgItem(hDlg, IDC_INCREASE_CONTRAST);
	i = _config[configNum]._contrastIncrease;

	if (i >= 0 && i < NUM_CONTRAST_OPTIONS) {
		SendMessage(ctl, CB_SETCURSEL, i, 0);
	}
	else {
		SendMessage(ctl, CB_SETCURSEL, 0, 0);
	}

	i = (int) _config[configNum]._segmentationThreshold;
	SetDlgItemInt(hDlg, IDC_SEGMENTATION_THRESHOLD, i, TRUE);

	SetDlgItemInt(hDlg, IDC_SEGMENTATION_THRESHOLD_AUTO, 0, TRUE);
	CheckDlgButton(hDlg, IDC_USE_AUTO_THRESHOLD, _config[configNum]._useAutoThreshold ? TRUE : FALSE);
	CheckDlgButton(hDlg, IDC_USE_ADAPTIVE_THRESHOLD, _config[configNum]._useAdaptiveThreshold ? TRUE : FALSE);
	SetDlgItemInt(hDlg, IDC_NEIGHBORHOOD_SIZE, _config[configNum]._adaptiveThresholdNeighborhood, TRUE);
}

/*
  =======================================================================================  
  =======================================================================================
*/
void ImageOperations::populateConfigWithDlgControls(HWND hDlg, int configNum)
{
	char buff[256];
	HWND ctl;
	int i;
	const char *s;

	memset(buff, 0, sizeof(buff));
	ctl = GetDlgItem(hDlg, IDC_BACKGROUND_ELIMINATION);

	i = (int) SendMessage(ctl, CB_GETCURSEL, 0, 0);

	s = _maskAvailableList.getString(i);

	if (!s) {
		s = _maskAvailableList.getString(0);
	}

	_config[configNum]._maskList.clearList();
	_config[configNum]._maskList.addString(s);

	ctl = GetDlgItem(hDlg, IDC_INCREASE_CONTRAST);
	i = (int) SendMessage(ctl, CB_GETCURSEL, 0, 0);

	if (i == LB_ERR) {
		i = 0;
	}

	_config[configNum]._contrastIncrease = i;
		
	i = GetDlgItemInt(hDlg, IDC_SEGMENTATION_THRESHOLD, NULL, TRUE);
	_config[configNum]._segmentationThreshold = i;

	i = GetDlgItemInt(hDlg, IDC_SEGMENTATION_THRESHOLD_AUTO, NULL, TRUE);
	_config[configNum]._segmentationThresholdAuto = i;

	_config[configNum]._useAutoThreshold = IsDlgButtonChecked(hDlg, IDC_USE_AUTO_THRESHOLD) ? true : false;
	_config[configNum]._useAdaptiveThreshold = IsDlgButtonChecked(hDlg, IDC_USE_ADAPTIVE_THRESHOLD) ? true : false;
	
	i = GetDlgItemInt(hDlg, IDC_NEIGHBORHOOD_SIZE, NULL, TRUE);
	
	if (i < 0) {
		i = 1;
	}
	else if (i > 10) {
		i = 10;
	}

	_config[configNum]._adaptiveThresholdNeighborhood = i;

}

/*
  =======================================================================================  
  =======================================================================================
*/
BOOL CALLBACK
ImageOperations::ImageOperationsDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImageOperations *io;
	int i, cnt;
	const char *s;
	HWND ctl;
	char buff[128];
	
	switch (msg)
	{
	case WM_INITDIALOG:
		io = (ImageOperations *) lParam;

		if (!io) {
			return FALSE;
		}

		SetWindowLong(hDlg, GWL_USERDATA, lParam);
	
		ctl = GetDlgItem(hDlg, IDC_BACKGROUND_ELIMINATION);
		cnt = io->_maskAvailableList.getNumStrings();

		for (i = 0; i < cnt; i++) {
			s = io->_maskAvailableList.getString(i);
			SendMessage(ctl, CB_ADDSTRING, 0, (LPARAM) s);	
		}

		ctl = GetDlgItem(hDlg, IDC_INCREASE_CONTRAST);

		SendMessage(ctl, CB_ADDSTRING, 0, (LPARAM) "None");
		SendMessage(ctl, CB_ADDSTRING, 0, (LPARAM) "1 (pow9)");
		SendMessage(ctl, CB_ADDSTRING, 0, (LPARAM) "2 (pow8)");
		SendMessage(ctl, CB_ADDSTRING, 0, (LPARAM) "3 (pow7)");
		SendMessage(ctl, CB_ADDSTRING, 0, (LPARAM) "4 (pow6)");

		io->populateDlgControlsWithConfig(hDlg, 0);

		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RAW_PROCESSING:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);
			
			if (io) {
				strncpy_s(io->_opDlgTitle, sizeof(io->_opDlgTitle), "Raw Image Operations", _TRUNCATE);
	
				if (io->showListDlg(hDlg, &io->_rawAvailableList, &io->_config[0]._rawList)) {
					memset(buff, 0, sizeof(buff));
					io->populateCSVList(buff, sizeof(buff), &io->_config[0]._rawList);
					SetDlgItemText(hDlg, IDC_RAW_LIST, buff);		
					InvalidateRect(hDlg, NULL, TRUE);
				}
			}

			break;

		case IDC_GRAYSCALE_PROCESSING:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				strncpy_s(io->_opDlgTitle, sizeof(io->_opDlgTitle), "Grayscale Image Operations", _TRUNCATE);

				if (io->showListDlg(hDlg, &io->_grayscaleAvailableList, &io->_config[0]._grayscaleList)) {
					memset(buff, 0, sizeof(buff));
					io->populateCSVList(buff, sizeof(buff), &io->_config[0]._grayscaleList);
					SetDlgItemText(hDlg, IDC_GRAYSCALE_LIST, buff);		
					InvalidateRect(hDlg, NULL, TRUE);
				}
			}

			break;

		case IDC_BINARY_PROCESSING:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				strncpy_s(io->_opDlgTitle, sizeof(io->_opDlgTitle), "Binary Image Operations", _TRUNCATE);

				if (io->showListDlg(hDlg, &io->_binaryAvailableList, &io->_config[0]._binaryList)) {
					memset(buff, 0, sizeof(buff));
					io->populateCSVList(buff, sizeof(buff), &io->_config[0]._binaryList);
					SetDlgItemText(hDlg, IDC_BINARY_LIST, buff);		
					InvalidateRect(hDlg, NULL, TRUE);
				}
			}

			break;

		case IDC_APPLY:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->populateConfigWithDlgControls(hDlg, 0);
				io->apply();
			}

			break;

		case IDC_USE_DEFAULTS:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->_config[0].setDefaults();
				io->populateDlgControlsWithConfig(hDlg, 0);
				io->apply();
			}

			break;

		case IDC_SAVE_ONE:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->populateConfigWithDlgControls(hDlg, 0);
				io->_config[1] = io->_config[0];				
			}

			break;

		case IDC_SAVE_TWO:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->populateConfigWithDlgControls(hDlg, 0);
				io->_config[2] = io->_config[0];		
			}

			break;

		case IDC_SAVE_THREE:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->populateConfigWithDlgControls(hDlg, 0);
				io->_config[3] = io->_config[0];		
			}

			break;

		case IDC_SAVE_FOUR:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->populateConfigWithDlgControls(hDlg, 0);
				io->_config[4] = io->_config[0];		
			}

			break;

		case IDC_LOAD_ONE:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->_config[0] = io->_config[1];
				io->populateDlgControlsWithConfig(hDlg, 0);
				io->apply();
			}

			break;

		case IDC_LOAD_TWO:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->_config[0] = io->_config[2];
				io->populateDlgControlsWithConfig(hDlg, 0);
				io->apply();
			}

			break;

		case IDC_LOAD_THREE:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->_config[0] = io->_config[3];
				io->populateDlgControlsWithConfig(hDlg, 0);
				io->apply();
			}

			break;

		case IDC_LOAD_FOUR:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (io) {
				io->_config[0] = io->_config[4];
				io->populateDlgControlsWithConfig(hDlg, 0);
				io->apply();
			}

			break;

		case IDCANCEL:
			DestroyWindow(hDlg);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	
	case WM_DESTROY:
		io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

		if (io) {
			io->wmDestroy(hDlg);
		}

		break;
	}

	return FALSE;
}

/*
  =======================================================================================  
  =======================================================================================
*/
bool ImageOperations::showListDlg(HWND hWndParent, StringList *availableList, 
								 StringList *currentList)
{
	const char *s;
	int n, cnt;

	if (!availableList || !currentList) {
		return false;
	}

	_availableList = *availableList;
	_opList = *currentList;

	cnt = _opList.getNumStrings();

	for (int i = 0; i < cnt; i++) {
		s = _opList.getString(i);

		n = _availableList.findString(s);
		
		if (n >= 0) {
			_availableList.removeString(n);
		}
	}

	if (DialogBoxParam(GetModuleHandle(NULL), 
						MAKEINTRESOURCE(IDD_IMAGE_OPERATIONS), 
						hWndParent,
						ImageOperations::OpListDlg,
						(LPARAM) this)) {
	
			*currentList = _opList;
			return true;
	}

	return false;
}

/*
  =======================================================================================  
  =======================================================================================
*/
BOOL CALLBACK
ImageOperations::OpListDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImageOperations *io;
	int i, cnt;
	const char *s;
	HWND src, dst;
	char buff[64];

	switch (msg)
	{
	case WM_INITDIALOG:
		io = (ImageOperations *) lParam;

		if (!io) {
			return FALSE;
		}

		SetWindowLong(hDlg, GWL_USERDATA, lParam);

		if (strlen(io->_opDlgTitle) > 0) {
			SetWindowText(hDlg, io->_opDlgTitle);
		}

		dst = GetDlgItem(hDlg, IDC_AVAILABLE_LIST);
		cnt = io->_availableList.getNumStrings();

		for (i = 0; i < cnt; i++) {
			s = io->_availableList.getString(i);

			if (s) {
				SendMessage(dst, LB_ADDSTRING, 0, (LPARAM) s);
			}
		}

		if (cnt > 0) {
			SendMessage(dst, LB_SETCURSEL, -1, 0);	
		}

		EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);

		dst = GetDlgItem(hDlg, IDC_CURRENT_LIST);
		cnt = io->_opList.getNumStrings();

		for (i = 0; i < cnt; i++) {
			s = io->_opList.getString(i);

			if (s) {
				SendMessage(dst, LB_ADDSTRING, 0, (LPARAM) s);
			}
		}

		if (cnt > 0) {
			SendMessage(dst, LB_SETCURSEL, -1, 0);
		}

		EnableWindow(GetDlgItem(hDlg, IDC_REMOVE), FALSE);
		
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_AVAILABLE_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				src = (HWND) lParam;

				if (LB_ERR != SendMessage(src, LB_GETCURSEL, 0, 0)) {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
				}
			}

			break;

		case IDC_CURRENT_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				src = (HWND) lParam;

				if (LB_ERR != SendMessage(src, LB_GETCURSEL, 0, 0)) {
					EnableWindow(GetDlgItem(hDlg, IDC_REMOVE), TRUE);				
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_REMOVE), FALSE);
				}
			}

			break;

		case IDC_ADD:
			src = GetDlgItem(hDlg, IDC_AVAILABLE_LIST);
			dst = GetDlgItem(hDlg, IDC_CURRENT_LIST);

			i = (int) SendMessage(src, LB_GETCURSEL, 0, 0);

			if (i != LB_ERR) {
				memset(buff, 0, sizeof(buff));

				if (LB_ERR != SendMessage(src, LB_GETTEXT, i, (LPARAM) buff)) {
					SendMessage(dst, LB_ADDSTRING, 0, (LPARAM) buff);
					SendMessage(src, LB_DELETESTRING, i, 0);

					cnt = (int) SendMessage(src, LB_GETCOUNT, 0, 0);

					if (cnt > 0) {
						if (i >= cnt) {
							i--;
						}

						SendMessage(src, LB_SETCURSEL, i, 0);
					}
					else {
						EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
					}
				}
			}

			break;

		case IDC_REMOVE:
			dst = GetDlgItem(hDlg, IDC_AVAILABLE_LIST);
			src = GetDlgItem(hDlg, IDC_CURRENT_LIST);

			i = (int) SendMessage(src, LB_GETCURSEL, 0, 0);

			if (i != LB_ERR) {
				memset(buff, 0, sizeof(buff));

				if (LB_ERR != SendMessage(src, LB_GETTEXT, i, (LPARAM) buff)) {
					SendMessage(dst, LB_ADDSTRING, 0, (LPARAM) buff);
					SendMessage(src, LB_DELETESTRING, i, 0);

					cnt = (int) SendMessage(src, LB_GETCOUNT, 0, 0);
					
					if (cnt > 0) {
						if (i >= cnt) {
							i--;
						}

						SendMessage(src, LB_SETCURSEL, i, 0);
					}
					else {
						EnableWindow(GetDlgItem(hDlg, IDC_REMOVE), FALSE);
					}
				}
			}

			break;

		case IDOK:
			io = (ImageOperations *) GetWindowLong(hDlg, GWL_USERDATA);

			if (!io) {
				EndDialog(hDlg, FALSE);
			}
			else {
				io->_opList.clearList();

				src = GetDlgItem(hDlg, IDC_CURRENT_LIST);
				cnt = (int) SendMessage(src, LB_GETCOUNT, 0, 0);
				
				for (i = 0; i < cnt; i++) {
					memset(buff, 0, sizeof(buff));

					if (LB_ERR != SendMessage(src, LB_GETTEXT, i, (LPARAM) buff)) {
						io->_opList.addString(buff);
					}
				}

				EndDialog(hDlg, TRUE);
			}

			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}
