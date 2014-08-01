/*!
 * <kinect base class>
 * 
 * Copyright (c) 2011-2012 by <yltang @ vipl>
 */
#ifndef KINECT_H
#define KINECT_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cv.h>
#include <highgui.h>
#include <math.h>
#include <vector>
#include <map>
#include <algorithm>

// #include "boost\thread.hpp"
// #include "boost\filesystem.hpp"
// #include "boost\progress.hpp"
//#include "common\depth.h"

using namespace std;
using namespace cv;


#define PI 3.1415926535
#define ORIWIDTH     640  //original camera width
#define ORIHEIGHT    480  //original camera height

//
// kinect data content: used in writeDataContent(); getDataContent();
//
#define KINECT_RGB 0x01
#define KINECT_ORI_DEPTH 0x02
#define KINECT_MONOCOLOR_DEPTH 0x04
#define KINECT_COLOR_DEPTH 0x08
#define KINECTmPointCloud 0x10
#define KINECT_PLAYER  0x20	   //unused
#define KINECT_SKELETON 0x40   //unused
#define KINECT_GESTURE 0x80	   //unused


// record data suffix, color/depth/skeleton and its frame number
#define COLOR_REC_DATA  "\\color.avi"
#define COLOR_REC_FRAME "\\color.frame"

#define DEPTH_REC_DATA  "\\depth.dat"
#define DEPTH_REC_FRAME "\\depth.frame"

#define SKELE_REC_DATA  "\\skeleton.dat"
#define SKELE_REC_FRAME "\\skeleton.frame"

//#define MODELTYPE_PLY
typedef struct _Vector2i
{
    int x;
    int y;
}Vector2i;
typedef struct _Vector4f
{
    float x;
    float y;
    float z;
    float w;
} Vector4f;

struct SLR_ST_Skeleton
{
    Vector4f _3dPoint[20];
    Vector2i _2dPoint[20];
}; 

struct SLR_STR_Frame
{
    LONGLONG rgbTimeStamp;
    LONGLONG depthTimeStamp;
    LONGLONG skeletonTimeStamp;

    Mat rgb;
    Mat depth;
    SLR_ST_Skeleton skeleton;
    // TODO: we can add skeleton and other info
};


class Kinect{
public:


    enum E_KINECT_RECOARD_STATUS
    {
        E_KRS_DEFAULT,  //not recording
        E_KRS_BEGIN,    //trigger begin record
        E_KRS_DURATION, //during record
        E_KRS_END       //trigger stop record
    };

    enum E_RECORD_TYPE
    {
        E_RT_NOT_SET,
        E_RT_ONI,      //record to .oni file                      (1 file, only support openNI, small and tight)
        E_RT_AVI,      //record rgb to .avi, depth to .dat/.avi?? (2 file, bigger but common)
        E_RT_FOLDER     //record rgb to .jpg, depth to .png        (N*2 files, biggest but clear)
    };

    enum E_OPEN_TYPE
    {
        E_OT_DEFAULT,  // NOT SET

        E_OT_MSKINECT, // microsoft sdk
        E_OT_MS_KEEP,  // microsoft record file( may be in the future )
        E_OT_PSKINECT, // openNI sdk
        E_OT_ONI,      // openNI record file
                       
        E_OT_AVI = 10, // rgb to avi, depth to dat
        E_OT_FOLDER     // rgb to jpg, depth to png
    };

protected:
    bool bIsOpen;              //open state of kinect	
    bool bFirstFrameGet;       //check first frame arrive, to sync capture thread and process thread

    int nFrameWidth;            //process frame width
	int nFrameHeight;		    //process frame height
    int nOriWidth;              //original width from file: i.e. ONI, AVI, JPG
    int nOriHeight;             //original height from file:
	


    ushort *depthData;
    uchar  *colorData;

    // origin data
    LONGLONG i_ts_rgb;
    LONGLONG i_ts_depth;
    LONGLONG i_ts_skeleton;

    SLR_ST_Skeleton mSkeleton;    
	Mat mRgb;			   //BGR image
    
	Mat mDepth;            //16bit depth value
	
    Mat mPointCloud;	   //x,y,z in mm

    // driver output curFrame
    SLR_STR_Frame stCurFrame;

    //VideoWriter   rgbTest;
    Mat mTestRgb;

    void initMat();
    /************************************************************************/
	/* Kinect record use
	/************************************************************************/
    E_KINECT_RECOARD_STATUS   eKinectRecordStatus;
    E_RECORD_TYPE             eRecordFileType;
    E_OPEN_TYPE               eOpenType;

//     boost::mutex  mtxFrame; // lock for return current frame;
// 
//     //boost::mutex  mtxTestWriter;
//     boost::mutex  mtxColorWriter;
//     boost::mutex  mtxDepthWriter;
//     boost::mutex  mtxSkeletonWriter;

    string       strDumpPath;
    string       strDumpFile; //without suffix, it can be a folder!
    long int     nCurFrameNumber;
    long int     nCurRecordFrameNumber;
    long int     nTotalFrameNumber;

    bool beginRecordAVI();
    bool endRecordAVI();
    bool beginRecordFolder();
    bool endRecordFolder();
    
    bool writeOneFrame();

//    boost::progress_timer recordTimer;
    double recordElapseTime;
    int depthFrameCount;
    int colorFrameCount;
    int skeletonFrameCount;
    bool writeOneFrameDepth();    // .png
    bool writeOneFrameColor();    // .jpg
    bool writeOneFrameSkeleton(); // .dat

    bool updataDataFile();

    bool updataDataAVI();
    bool updataDataFolder();

private:

	Mat mRT;			   //for point cloud transform
	Mat mDepthColorful;      //transfer 16bit depthMat into colorful hsv space
	Mat mDepthMonoColor;  //transfer 16bit depthMat into blue hsv space
	Mat mErrorReturn;     //if image didn't create, show this error image
    
    virtual void singleCaptureThread(){};
    virtual void colorCaptureThread(){};
    virtual void depthCaptureThread(){};
    virtual void skeletonCaptureThread(){};

// data
    VideoWriter  rgbVideoWriter;
	VideoCapture rgbVideoReader;

	ofstream     depthFileWriter;
	ifstream     depthFileReader;

    ofstream     skeletonFileWriter;
    ifstream     skeletonFileReader;
// frame
    ofstream     colorFrameWriter;
    ifstream     colorFrameReader;

    ofstream     depthFrameWriter;
    ifstream     depthFrameReader; 

    ofstream     skeletonFrameWriter;
    ifstream     skeletonFrameReader;

    ofstream     recordLogFile;
//

    string       strRgbFile;
    string       strDepthFile;
    string       strFrameFolder;

    virtual void _beginRecord(){};
    virtual void _endRecord(){};


	inline double degreeToRad(double degree) {return (degree*PI/180.0);}
	void ConvertProjectiveToRealWorld(Mat& depthImage);
	Point ConvertRealWorldToProjective(float x, float y, float z);

public:
	Kinect();
	~Kinect();	
    
//    boost::thread     trdColorCapture, trdDepthCapture, trdSkeletonCapture, trdSingleCapture;

	/************************************************************************/
	/* Kinect open and close
	/************************************************************************/
    virtual bool init(bool mirror,bool changeView,int height,int width){return false;} //initalize kinect
    bool init(string folderPath, int height, int width);
    bool init(string folderPath, int totalFrameNumber, int height, int width);

    virtual bool close(){ return false; }		  //shutdown kinect
    virtual bool updateData(){ return false; }    //updateData
	inline bool isOpen() {return bIsOpen;}
    void runCaptureThread();

	/************************************************************************/
	/* Kinect controller
	/************************************************************************/	
    virtual bool tiltToAngle(int angle){ return false; }

	/************************************************************************/
	/* Kinect retrive
	/************************************************************************/	
//     Mat retrieveRGB() { boost::mutex::scoped_lock lock( mtxFrame ); return stCurFrame.rgb; }	    //retrieve rgb image
//     Mat retrieveDepth() { boost::mutex::scoped_lock lock( mtxFrame ); return stCurFrame.depth; } //retrieve depth image
//     Mat retrieveFrame() { boost::mutex::scoped_lock lock( mtxFrame ); return mTestRgb; }

	Mat retrievePointCloud(Mat& depthImage);	  //retrieve point cloud data
	void setRtMatrix(bool isDegree, double alpha, double beta, double gamma, double x, double y, double z);


    bool setDumpPath(string path);
    inline string getDumpPath() { return strDumpPath; }
	/************************************************************************/
	/* Get and write image
	/************************************************************************/
	Mat getDataContent(int kinectDataContent);
	void writeDataContent(string filePathWithoutFilesuffix, int kinectDataContent);
	void writeObj(Mat& pointCloud, string filePathWithoutFilesuffix, int zmax, int zmin = 0);
	void writePly(Mat& pointCloud, Mat& rgbImage, string filePathWithoutFilesuffix, int zmax, int zmin = 0);
	/************************************************************************/
	/* Write and Read frame image
	/************************************************************************/
	bool beginReadFoler(string path, int frameNumber);
	//bool readOneFrame(int curFrame); //if read finished, return false

	/************************************************************************/
	/* Write and read video (RGB to .avi, D to .dat)
	/************************************************************************/
    inline long int getFrameNumber() { return nCurFrameNumber; }
    inline long int getTotalFrameNumber() { return nTotalFrameNumber; }
    inline long int getRecordFrameNumber() { return nCurRecordFrameNumber; }
	void beginRecord(E_RECORD_TYPE rtype, string path);
    inline void endRecord() { eKinectRecordStatus = E_KRS_END; }

	bool beginReadAVI(string folder);
    bool openBinaryFile(string& filepath, ifstream& openStream);
};


#endif