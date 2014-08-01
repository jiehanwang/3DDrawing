/*!
 * <microsoft kinect sdk v1.0 beta2 wrapper>
 * 
 * Copyright (c) 2011-2012 by <yltang @ vipl>
 */
#ifndef MICROSOFT_KINECT_H
#define MICROSOFT_KINECT_H

#include <windows.h>
#include "kinect.h"
#include "NuiApi.h"
#include "NuiImageCamera.h"
#include "NuiSensor.h"
#include "NuiSkeleton.h"
#include "Ini.h"

class MsKinect : public Kinect{
private:
    INuiSensor* m_pNuiSensor;

	HANDLE hColorStream;
	HANDLE hDepthStream;

    HANDLE hNextFrame;
    HANDLE hNextFrameReady;
    HANDLE hNextSkeletonReady;

    HANDLE hNextColor;
    HANDLE hNextDepth;
    HANDLE hNextSkeleton;

    bool bUserSkeletonTracked;
    DWORD  nUserId;

	const NUI_IMAGE_FRAME     pImageFrameColor;
	const NUI_IMAGE_FRAME     pImageFrameDepth;
    NUI_SKELETON_FRAME  imageFrameSkeleton;
	INuiFrameTexture * pTextureColor;
	INuiFrameTexture * pTextureDepth;

	NUI_LOCKED_RECT LockedRectColor;
	NUI_LOCKED_RECT LockedRectDepth;

    bool updataDataDevice();
    bool updataDataDeviceSingle();

//    boost::mutex  mtxColor;
    bool queryColor();

//    boost::mutex  mtxDepth;
    bool queryDepth();

//    boost::mutex  mtxSkeleton;
    bool querySkeleton();
    //bool queryDevice();

    //boost::mutex mtxFrame;
    bool queryFrame();

    Mat oriDepthImg;

    CIni iniFile;
    fstream debugFile;
    NUI_TRANSFORM_SMOOTH_PARAMETERS someLatencyParams;
public:
	MsKinect();
	~MsKinect();

	virtual bool init(bool mirror,bool changeView,int height,int width);
	virtual bool close();
	virtual bool updateData();
	virtual bool tiltToAngle(int angle);

	//Mat& retrieveRGB();						    //retrieve rgb image
	//Mat& retrieveDepth();						//retrieve depth image

    virtual void singleCaptureThread();
    virtual void colorCaptureThread();
    virtual void depthCaptureThread();
    virtual void skeletonCaptureThread();

    virtual void _beginRecord();
    virtual void _endRecord();
};

#endif