/*
 * $Id: PixelChain.cpp,v 1.1.1.1 2008/10/10 11:05:51 scott Exp $
 *
 */

#include <math.h>

#include "PixelChain.h"
#include "utility.h"


double PixelChain::_pixelDistance[9] = { 0.0, 1.414, 1.0, 1.414, 1.0, 1.0, 1.414, 1.0, 1.414 };



/*
  =======================================================================================
  =======================================================================================
*/
PixelChain::PixelChain()
{
	_numPoints = 0;
	_maxPoints = 0;
	_pt = NULL;
	_maxFeretAngle = 0.0;
	_maxFeretDiameter = 0.0;
	_boundingRect.left = 0;
	_boundingRect.right = 0;
	_boundingRect.top = 0;
	_boundingRect.bottom = 0;
}

/*
  =======================================================================================
  =======================================================================================
*/
PixelChain::~PixelChain()
{
	if (_pt) {
		delete [] _pt;
		_pt = NULL;
	}

	_numPoints = 0;
	_maxPoints = 0;
}

/*
  =======================================================================================
  =======================================================================================
*/
PixelChain& PixelChain::operator=(PixelChain &rhs)
{
	if (this != &rhs) {
		if (_pt) {
			delete [] _pt;
			_pt = NULL;
		}

		_maxPoints = 0;
		_numPoints = 0;

		if (rhs._numPoints > 0) {
			if (growList(rhs._numPoints)) {
				memcpy(_pt, rhs._pt, rhs._numPoints * sizeof(_pt[0]));
				_numPoints = rhs._numPoints;
			}
		}

		_maxFeretAngle = rhs._maxFeretAngle;
		_maxFeretDiameter = rhs._maxFeretDiameter;
		_boundingRect = rhs._boundingRect;
	}

	return *this;
}

/*
  =======================================================================================
  =======================================================================================
*/
bool PixelChain::load(double max_feret_angle, double max_feret_diameter,
					  int left, int right, int top, int bottom,
					  int num_points, long *chain_id, long *x, long *y)
{
	_numPoints = 0;

	if (num_points < 1) {
		return true;
	}

	if (!chain_id || !x || !y) {
		return false;
	}

	// we only care about the outer chain, chain_id == 1
	for (_numPoints = 0; _numPoints < num_points; _numPoints++) {
		if (chain_id[_numPoints] != 1) {
			break;
		}
	}

	if (_numPoints == 0) {
		// should never happen
		return false;
	}

	if (_numPoints > _maxPoints) {
		if (!growList(_numPoints)) {
			return false;
		}
	}

	for (int i = 0; i < _numPoints; i++) {
		_pt[i].x = x[i];
		_pt[i].y = y[i];
	}

	_maxFeretAngle = max_feret_angle;
	_maxFeretDiameter = max_feret_diameter;
	_boundingRect.left = left;
	_boundingRect.right = right;
	_boundingRect.top = top;
	_boundingRect.bottom = bottom;

	return true;
}

/*
  =======================================================================================
  Change the y coordinates so that positive y is up
  from 
  0 (top) and limit - 1 (bottom) 
  to 
  0 (bottom) and limit - 1 (top) 
  =======================================================================================
*/
void PixelChain::flipY(int limit)
{
	limit -= 1;

	_boundingRect.top = limit - _boundingRect.top;
	_boundingRect.bottom = limit - _boundingRect.bottom;

	for (int i = 0; i < _numPoints; i++) {
		_pt[i].y = limit - _pt[i].y;
	}
}

/*
  =======================================================================================
  Translate the image so that the center of the _boundingRect is at x, y
  =======================================================================================
*/
void PixelChain::center(int x, int y)
{
	POINT center, translation;
	
	center.x = (_boundingRect.right + _boundingRect.left) / 2;
	center.y = (_boundingRect.bottom + _boundingRect.top) / 2;

	translation.x = x - center.x;
	translation.y = y - center.y;

	_boundingRect.left += translation.x;
	_boundingRect.right += translation.x;
	_boundingRect.top += translation.y;
	_boundingRect.bottom += translation.y;

	for (int i = 0; i < _numPoints; i++) {
		_pt[i].x += translation.x;
		_pt[i].y += translation.y;
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
void PixelChain::rotate(double angle)
{
	POINT delta;
	double radians, x, y, c, s;
	int i;

	if (angle == 0.0 || angle == 180.0) {
		return;
	}

	radians = angle * (PI / 180.0);

	delta.x = (_boundingRect.left + _boundingRect.right) / 2;
	delta.y = (_boundingRect.top + _boundingRect.bottom) / 2;

	// first center the image around 0,0 also changing the y-coordinates so that up is positive
	_boundingRect.left = _boundingRect.left - delta.x;
	_boundingRect.right = _boundingRect.right - delta.x;
	_boundingRect.top = delta.y - _boundingRect.top;
	_boundingRect.bottom = delta.y - _boundingRect.bottom;

	_corners[0].x = _boundingRect.left;
	_corners[0].y = _boundingRect.top;
	_corners[1].x = _boundingRect.right;
	_corners[1].y = _boundingRect.top;
	_corners[2].x = _boundingRect.right;
	_corners[2].y = _boundingRect.bottom;
	_corners[3].x = _boundingRect.left;
	_corners[3].y = _boundingRect.bottom;

	for (i = 0; i < _numPoints; i++) {
		_pt[i].x = _pt[i].x - delta.x;
		_pt[i].y = delta.y - _pt[i].y;
	}

	c = cos(radians);
	s = sin(radians);

	// now rotate 
	// | cos@  -sin@ |
	// | sin@   cos@ |
	for (i = 0; i < 4; i++) {
		x = _corners[i].x;
		y = _corners[i].y;
		_corners[i].x = (int) ((x * c) - (y * s));
		_corners[i].y = (int) ((x * s) + (y * c));
	}

	for (i = 0; i < _numPoints; i++) {
		x = _pt[i].x;
		y = _pt[i].y;
		_pt[i].x = (int) ((x * c) - (y * s));
		_pt[i].y = (int) ((x * s) + (y * c));
	}
 
	// translate back to center of window, putting the y-coord back to GUI orientation
	for (i = 0; i < 4; i++) {
		_corners[i].x = _corners[i].x + delta.x;
		_corners[i].y = delta.y - _corners[i].y;
	}

	for (i = 0; i < _numPoints; i++) {
		_pt[i].x = _pt[i].x + delta.x;
		_pt[i].y = delta.y - _pt[i].y;
	}
}

/*
  =======================================================================================
  =======================================================================================
*/
double PixelChain::calculatePerimeter()
{
	double perim;
	
	unsigned char *chain = getPerimeterChain(NULL);

	if (!chain) {
		return 0.0;
	}

	perim = 0.0;

	for (int i = 0; i < _numPoints - 1; i++) {
		perim += _pixelDistance[chain[i]];
	}

	delete [] chain;

	return perim;
}

/*
  =======================================================================================
  =======================================================================================
*/
unsigned char* PixelChain::getPerimeterChain(int *num_points)
{
	unsigned char *chain;
	int i, n;

	if (num_points) {
		*num_points = 0;
	}

	if (_numPoints < 2) {
		return NULL;
	}

	chain = new unsigned char[_numPoints];

	if (!chain) {
		return NULL;
	}

	memset(chain, 0, sizeof(unsigned char) * _numPoints);
	n = _numPoints - 1;

	for (i = 0; i < n; i++) {
		chain[i] = mapPointMovement(i);
	}

	chain[n] = 0;

	if (num_points) {
		*num_points = _numPoints;
	}

	return chain;
}

/*
  =======================================================================================
  =======================================================================================
*/
POINT PixelChain::getFirstPerimeterPoint()
{
	POINT pt;

	if (_numPoints > 0) {
		pt = _pt[0];
	}
	else {
		pt.x = 0;
		pt.y = 0;
	}

	return pt;
}

/*
  =======================================================================================
  =======================================================================================
*/
int PixelChain::comparePoints(const void *a, const void *b)
{
	POINT *pa, *pb;

	pa = (POINT *) a;
	pb = (POINT *) b;

	if (pa->x < pb->x) {
		return -1;
	}
	else if (pa->x > pb->x) {
		return 1;
	}
	else {
		if (pa->y < pb->y) {
			return -1;
		}
		else if (pa->y == pb->y) {
			return 0;
		}
		else {
			return 1;
		}
	}
}

/*
  =======================================================================================
  We count the perimeter points as part of the area.
  =======================================================================================
*/
int PixelChain::calculateArea()
{
	int area;
	POINT *pt;
	int i, n;

	if (_numPoints < 2) {
		return _numPoints;
	}

	pt = new POINT[_numPoints];

	if (!pt) {
		return 0;
	}

	// one less since the first and last points are the same
	n = _numPoints - 1;

	memcpy(pt, _pt, n * sizeof(POINT));

	qsort(pt, n, sizeof(POINT), PixelChain::comparePoints);

	i = 0;
	n--;
	area = 0;

	while (i < n) {
		// a span
		if (pt[i].x == pt[i+1].x) {
			area += 1 + pt[i+1].y - pt[i].y;
			i += 2;
		}
		// a point
		else {
			area++;
			i++;
		}
	}

	if (i == n) {
		area++;
	}

	delete [] pt;

	return area;
}

/*
  =======================================================================================
  =======================================================================================
*/
void PixelChain::skeleton()
{
	POINT min, max;

	min.x = 20000;
	min.y = 20000;
	max.x = -1;
	max.y = -1;

	for (int i = 0; i < _numPoints; i++) {
		if (_pt[i].x < min.x) {
			min.x = _pt[i].x;
		}
		else if (_pt[i].x > max.x) {
			max.x = _pt[i].x;
		}

		if (_pt[i].y < min.y) {
			min.y = _pt[i].y;
		}
		else if (_pt[i].y > max.y) {
			max.y = _pt[i].y;
		}
	}

}

/*
  =======================================================================================
  Map coordinates of the edge by first marking the first coordinate, and then using
  the following mapping, all of the remaining coordinates using 1 byte each.

  4 | 3 | 2
  5 | 0 | 1
  6 | 7 | 8
  =======================================================================================
*/
bool PixelChain::write(HANDLE fh, int id)
{
	char buff[256], temp[32];
	unsigned long bytesWritten;
	int len;
	
	if (!fh) {
		return false;
	}

	memset(buff, 0, sizeof(buff));
	sprintf_s(buff, sizeof(buff), "%d,%d", id, _numPoints);
	
	if (_numPoints > 1) {
		sprintf_s(temp, sizeof(temp), ",%d,%d", _pt[0].x, _pt[0].y);
		strncat_s(buff, sizeof(buff), temp, _TRUNCATE);

		len = strlen(buff);
		memset(buff + len, 0, sizeof(buff) - len);

		if (_numPoints > 2) {
			buff[len] = ',';
			len++;
		
			for (int i = 1; i < _numPoints; i++) {
				buff[len] = '0' + mapPointMovement(i - 1);
				len++;

				if (len > sizeof(buff) - 4) {
					WriteFile(fh, buff, len, &bytesWritten, NULL);
					memset(buff, 0, sizeof(buff));
					len = 0;
				}
			}
		}
	}

	strncat_s(buff, sizeof(buff), "\r\n", _TRUNCATE);

	WriteFile(fh, buff, strlen(buff), &bytesWritten, NULL);

	return true;
}

/*
  =======================================================================================
  4 | 3 | 2
  5 | 0 | 1
  6 | 7 | 8
  =======================================================================================
*/
int PixelChain::mapPointMovement(int current_idx)
{
	int next_x, next_y;
	char ret;

	ret = 0;

	if (current_idx < _numPoints - 1) {
		next_x = _pt[current_idx + 1].x - _pt[current_idx].x;
		next_y = _pt[current_idx + 1].y - _pt[current_idx].y;
	
		if (next_x == -1) {
			if (next_y == -1) {
				ret = 4;
			}
			else if (next_y == 0) {
				ret = 5;
			}
			else if (next_y == 1) {
				ret = 6;
			}		
		}
		else if (next_x == 0) {
			if (next_y == -1) {
				ret = 3;
			}
			else if (next_y == 1) {
				ret = 7;
			}
		}
		else if (next_x == 1) {
			if (next_y == -1) {
				ret = 2;
			}
			else if (next_y == 0) {
				ret = 1;
			}
			else if (next_y == 1) {
				ret = 8;
			}
		}
	}

	return ret;
}

/*
  =======================================================================================
  =======================================================================================
*/
bool PixelChain::growList(int size_required)
{
	POINT *new_list;

	if (size_required < 128) {
		size_required = 128;
	}
	else {
		size_required += 127;
		size_required &= 0xff80;
	}

	new_list = new POINT[size_required];

	if (!new_list) {
		return false;
	}

	if (_pt) {
		delete [] _pt;
	}

	_pt = new_list;
	_maxPoints = size_required;

	return true;
}

