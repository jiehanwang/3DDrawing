/*!
 * <depth processing class>
 * 
 * Copyright (c) 2011-2012 by <yltang @ vipl>
 * 
 * History: 1. Create on 2012-3-31 
 * 
 */
#ifndef DEPTH_H
#define DEPTH_H

#include <iostream>
#include <cv.h>

using namespace std;
using namespace cv;

class Depth{
protected:
	Mat mDepth;
    Mat mDepthMonoColor;
    Mat mDepthColorful;

    int nFrameWidth;
    int nFrameHeight;

public:
	//io
	Depth(Mat& depth);
	~Depth();

	inline Mat& data() {return mDepth;}

	//process
	void smooth();

    //show
    Mat ToMonoShow();
    Mat ToColorShow();
};

#endif