#include "depth.h"

Depth::Depth(Mat &mat)
{
	mDepth = mat;
    nFrameWidth = mat.cols;
    nFrameHeight = mat.rows;
}

Depth::~Depth()
{

}

void Depth::smooth()
{
	Mat copyTemp;
	mDepth.copyTo(copyTemp);

	int w = copyTemp.cols;
	int h = copyTemp.rows;
	for(int j = 0 ; j < h ; ++j)
	{
		ushort* depthData = mDepth.ptr<ushort>(j);
		for(int i = 0 ; i < w ; ++i)
		{
			int count = 0;
			ushort depth = 0;
			int fourNeigh[4] = { copyTemp.at<ushort>(j,i), copyTemp.at<ushort>(j+1,i), 
								 copyTemp.at<ushort>(j,i+1) ,copyTemp.at<ushort>(j+1,i+1) };
			int k = 0;
			while( k < 4 )
			{
				if( fourNeigh[k] )
				{
					count++;
					depth += fourNeigh[k];				
				}
				k++;
			}
			if( count )
				*depthData++ = depth/count;
			else
				depthData++;
 		}	
	}
}


Mat Depth::ToMonoShow()
{
	mDepthMonoColor.create(nFrameHeight, nFrameWidth, CV_8UC3);
	Mat hsvMat;
	Mat dRgbMat;
	hsvMat.create(nFrameHeight, nFrameWidth, CV_8UC3);
	int nl= 1, nc= nFrameWidth * nFrameHeight ;
	for (int j=0; j<nl; j++)
	{
		ushort* data = mDepth.ptr<ushort>(j);
		uchar* odata = hsvMat.ptr<uchar>(j);
		for (int i=0; i<nc; i++) 
		{
			//data+=2;
			*odata++ = 120;
			*odata++ = 240;
			*odata++ = (*data==0.0)?0:255-ushort(log(*data/500.0)/log(20.0)*255);//if depth here is zero, leave it unchanged; otherwise process an exp map
			data++;
		}
	}
	cv::cvtColor(hsvMat, dRgbMat, CV_HSV2BGR);
	nl = nFrameHeight, nc = nFrameWidth;
	for (int j = 0; j < nl; j++)
	{
		uchar* odata = mDepthMonoColor.ptr<uchar>(nFrameHeight-nl+j)+(nFrameWidth-nc)*3;
		uchar* data = dRgbMat.ptr<uchar>(j);
		for (int i=0; i<nc; i++) 
		{
			*odata++ = *data++;
			*odata++ = *data++;
			*odata++ = *data++;
		}
	}
	return mDepthMonoColor;
}

Mat Depth::ToColorShow()
{
	double maxDisp=-1.f;
	float S=1.f;
	float V=1.f;
	Mat disp;
	disp.create( Size(nFrameWidth,nFrameHeight), CV_32FC1);
	disp = cv::Scalar::all(0);
	for( int y = 0; y < nFrameHeight; y++ )
	{
		for( int x = 0; x < nFrameWidth; x++ )
		{
			unsigned short curDepth = mDepth.at<unsigned short>(y,x);
			if( curDepth != 0 )
				disp.at<float>(y,x) = (75.0 * 757) / curDepth;
		}
	}	
	Mat gray;
	disp.convertTo( gray, CV_8UC1 );
	if( maxDisp <= 0 )
	{
		maxDisp = 0;
		minMaxLoc( gray, 0, &maxDisp );
	}
	mDepthColorful.create( gray.size(), CV_8UC3 );
	mDepthColorful = Scalar::all(0);

	for( int y = 0; y < nFrameHeight; y++ )
	{
		for( int x = 0; x < nFrameWidth; x++ )
		{
			uchar d = gray.at<uchar>(y,x);
			if (d == 0)
				continue;

			unsigned int H = ((uchar)maxDisp - d) * 240 / (uchar)maxDisp;

			unsigned int hi = (H/60) % 6;
			float f = H/60.f - H/60;
			float p = V * (1 - S);
			float q = V * (1 - f * S);
			float t = V * (1 - (1 - f) * S);

			Point3f res;

			if( hi == 0 ) //R = V,   G = t,   B = p
				res = Point3f( p, t, V );
			if( hi == 1 ) // R = q,   G = V,   B = p
				res = Point3f( p, V, q );
			if( hi == 2 ) // R = p,   G = V,   B = t
				res = Point3f( t, V, p );
			if( hi == 3 ) // R = p,   G = q,   B = V
				res = Point3f( V, q, p );
			if( hi == 4 ) // R = t,   G = p,   B = V
				res = Point3f( V, p, t );
			if( hi == 5 ) // R = V,   G = p,   B = q
				res = Point3f( q, p, V );

			uchar b = (uchar)(std::max(0.f, std::min (res.x, 1.f)) * 255.f);
			uchar g = (uchar)(std::max(0.f, std::min (res.y, 1.f)) * 255.f);
			uchar r = (uchar)(std::max(0.f, std::min (res.z, 1.f)) * 255.f);

			mDepthColorful.at<Point3_<uchar> >(y,x) = Point3_<uchar>(b, g, r);     
		}
	}
	return mDepthColorful;
}