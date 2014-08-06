#include "opencv2/opencv.hpp"
#include "opencv/cv.h"
#include "F.h"


void drawHist(CvHistogram *hist)
{
	int h_bins = 30, s_bins = 32;
	int scale = 10;
	IplImage* hist_img = cvCreateImage(  
		cvSize( h_bins * scale, s_bins * scale ), 
		8, 
		3
        ); 
	cvZero( hist_img );
	
	// populate our visualization with little gray squares.
	
	float max_value = 0;
	cvGetMinMaxHistValue( hist, 0, &max_value, 0, 0 );
	
	for( int h = 0; h < h_bins; h++ ) {
		for( int s = 0; s < s_bins; s++ ) {
			float bin_val = cvQueryHistValue_2D( hist, h, s );
			int intensity = cvRound( bin_val * 255 / max_value );
			cvRectangle( 
				hist_img, 
				cvPoint( h*scale, s*scale ),
				cvPoint( (h+1)*scale - 1, (s+1)*scale - 1),
				CV_RGB(intensity,intensity,intensity), 
				CV_FILLED
                );
		}
	}
	cvNamedWindow( "H-S Histogram", 1 );
	cvShowImage("H-S Histogram", hist_img );
	cvWaitKey(0);
//	cvDestroyWindow("H-S Histogram");
	cvReleaseImage(&hist_img);
}
// CvHistogram *skinModel(IplImage* image,int x,int y,int width,int height)
// {
// 	IplImage* facePic=grabPic(image,x,y,width,height);
// 	IplImage* faceHsv = cvCreateImage( cvGetSize(facePic), 8, 3 ); 
// 	cvCvtColor( facePic, faceHsv, CV_BGR2HSV );
// 	IplImage* h_plane  = cvCreateImage( cvGetSize(facePic), 8, 1 );
// 	IplImage* s_plane  = cvCreateImage( cvGetSize(facePic), 8, 1 );
// 	IplImage* v_plane  = cvCreateImage( cvGetSize(facePic), 8, 1 );
// 	IplImage* planes[] = { h_plane, s_plane };
// 	cvCvtPixToPlane( faceHsv, h_plane, s_plane, v_plane, 0 );
// 	int h_bins = 30, s_bins = 32; 
//     CvHistogram* hist;
// 	{
// 		int    hist_size[] = { h_bins, s_bins };
// 		float  h_ranges[]  = { 0, 180 };          // hue is [0,180]
// 		float  s_ranges[]  = { 0, 255 }; 
//         float* ranges[]    = { h_ranges, s_ranges };
// 		hist = cvCreateHist( 
//             2, 
//             hist_size, 
//             CV_HIST_ARRAY, 
//             ranges, 
//             1 
// 			); 
// 	}
// 	cvCalcHist( planes, hist, 0, 0 );
// 	cvNamedWindow( "Source", 1 );
// 	cvShowImage("Source", facePic );
// 	cvWaitKey(0);
// 	cvDestroyWindow("Source");
// 	cvReleaseImage(&facePic);
// 	cvReleaseImage(&faceHsv);
// 	cvReleaseImage(&h_plane);
// 	cvReleaseImage(&s_plane);
// 	cvReleaseImage(&v_plane);
// 	//cvReleaseImage(&planes[]);
// 	return hist;
// 	
// }

CvHistogram *skinModel(IplImage* facePic)
{
	IplImage* faceHsv = cvCreateImage( cvGetSize(facePic), 8, 3 ); 
	cvCvtColor( facePic, faceHsv, CV_BGR2HSV );
	IplImage* h_plane  = cvCreateImage( cvGetSize(facePic), 8, 1 );
	IplImage* s_plane  = cvCreateImage( cvGetSize(facePic), 8, 1 );
	IplImage* v_plane  = cvCreateImage( cvGetSize(facePic), 8, 1 );
	IplImage* planes[] = { h_plane, s_plane };
	cvCvtPixToPlane( faceHsv, h_plane, s_plane, v_plane, 0 );
	int h_bins = 30, s_bins = 32; 
    CvHistogram* hist;
	{
		int    hist_size[] = { h_bins, s_bins };
		float  h_ranges[]  = { 0, 180 };          // hue is [0,180]
		float  s_ranges[]  = { 0, 255 }; 
        float* ranges[]    = { h_ranges, s_ranges };
		hist = cvCreateHist( 
            2, 
            hist_size, 
            CV_HIST_ARRAY, 
            ranges, 
            1 
			); 
	}
	cvCalcHist( planes, hist, 0, 0 );
	//cvNamedWindow( "Source", 1 );
	//cvShowImage("Source", facePic );
    //cvWaitKey(0);
	//cvDestroyWindow("Source");
	//cvReleaseImage(&facePic);
	cvReleaseImage(&faceHsv);
	cvReleaseImage(&h_plane);
	cvReleaseImage(&s_plane);
	cvReleaseImage(&v_plane);

	return hist;
}