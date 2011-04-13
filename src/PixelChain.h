/*
 * $Id: PixelChain.h,v 1.1.1.1 2008/10/10 11:05:51 scott Exp $
 *
 */

#ifndef PIXEL_CHAIN_H
#define PIXEL_CHAIN_H

#include "version.h"

class PixelChain
{
public:
	PixelChain();
	~PixelChain();

	PixelChain& operator=(PixelChain &rhs);

	bool load(double max_feret_angle, double max_feret_diameter, 
				int left, int right, int top, int bottom, 
				int num_points, long *chain_id, long *x, long *y);

	void flipY(int limit);
	void center(int x, int y);
	void rotate(double angle);
	double calculatePerimeter();
	int calculateArea();
	void skeleton();
	unsigned char* getPerimeterChain(int *num_points);
	POINT getFirstPerimeterPoint();

	static int PixelChain::comparePoints(const void *a, const void *b);
	static double _pixelDistance[9];

	bool write(HANDLE fh, int id);
	int mapPointMovement(int current_idx);

	bool growList(int size_required);

	int _numPoints;
	int _maxPoints;
	POINT *_pt;
	double _maxFeretAngle;
	double _maxFeretDiameter;
	RECT _boundingRect;
	POINT _corners[4];
	
};


#endif // ifndef PIXEL_CHAIN_H
