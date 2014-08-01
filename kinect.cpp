#include "kinect.h"
//#include "logger.h"
#include "depth.h"

#include <direct.h>

Kinect::Kinect()
{
	mErrorReturn = Mat(480,640,CV_8UC3,Scalar(0));
	putText(mErrorReturn,"Image haven't captured!",Point(100,240),1,2,Scalar(0,0,255));

	nFrameWidth = 320;
	nFrameHeight = 240;
	bIsOpen = false;
    bFirstFrameGet = false;

    depthData = new ushort[640*480];
    colorData = new uchar[640*480*3];

    i_ts_rgb = -1;
    i_ts_depth = -1;
    i_ts_skeleton = -1;


    stCurFrame.rgb.create(480, 640, CV_8UC3);
    stCurFrame.rgb.setTo(0);
    stCurFrame.depth.create(480, 640, CV_16UC1);
    stCurFrame.depth.setTo(0);
    stCurFrame.rgbTimeStamp = -1;
    stCurFrame.depthTimeStamp = -1;
    stCurFrame.skeletonTimeStamp = -1;

    eOpenType = E_OT_DEFAULT;
}

Kinect::~Kinect()
{
    delete depthData;
    delete colorData;
}

void Kinect::initMat()
{
    mRgb.create( nFrameHeight, nFrameWidth, CV_8UC3 );
	mDepth.create( nFrameHeight, nFrameWidth, CV_16UC1 );
	mPointCloud.create(nFrameHeight,nFrameWidth,CV_32FC3);
}

bool Kinect::init(string folder, int height, int width)
{    
    if( !beginReadAVI(folder) )
        return false;
    nFrameWidth = width;
    nFrameHeight = height;
    bIsOpen = true;
    eOpenType = E_OT_AVI;
    return true;
}

bool Kinect::openBinaryFile(string& filepath, ifstream& openStream)
{
    openStream.open(filepath, ios::binary);
    if(! openStream.is_open() )
        return false;
    return true;
}

bool Kinect::beginReadAVI(string folder)
{
    string file = folder.substr( folder.rfind("\\")+1, folder.length() - folder.rfind("\\")-1 );
    file = folder + "\\" + file;

    string rgbFilePath = file + COLOR_REC_DATA;
    string rgbFramePath = file + COLOR_REC_FRAME;

    string depthFilePath= file + DEPTH_REC_DATA;
    string depthFramePath= file + DEPTH_REC_FRAME;

    string skeletonFilePath= file + SKELE_REC_DATA;
    string skeletonFramePath= file + SKELE_REC_FRAME;

    rgbVideoReader = VideoCapture(rgbFilePath);
    if( !openBinaryFile(rgbFramePath, colorFrameReader ) )  return false;

    if( !openBinaryFile(depthFilePath, depthFileReader ) )  return false;
    if( !openBinaryFile(depthFramePath, depthFrameReader ) )  return false;

    if( !openBinaryFile(skeletonFilePath, skeletonFileReader ) )  return false;
    if( !openBinaryFile(skeletonFramePath, skeletonFrameReader ) )  return false;
   

	nOriWidth = rgbVideoReader.get(CV_CAP_PROP_FRAME_WIDTH);
	nOriHeight = rgbVideoReader.get(CV_CAP_PROP_FRAME_HEIGHT);
	nTotalFrameNumber = rgbVideoReader.get(CV_CAP_PROP_FRAME_COUNT);
	return true;
}

bool Kinect::init(string folderPath, int totalFrameNumber, int height, int width)
{
    if( !beginReadFoler(folderPath, totalFrameNumber) )
        return false;

    strFrameFolder = folderPath;
    nCurFrameNumber = 0;
    nFrameWidth = width;
    nFrameHeight = height;
    bIsOpen = true;
    eOpenType = E_OT_FOLDER;
    return true;
}

bool Kinect::beginReadFoler(string folderPath, int totalFrameNumber)
{
    string rgbFilePath = folderPath + "\\0.jpg";
    mRgb = imread(rgbFilePath);
    if( mRgb.data == NULL )
    {
        //PROLOGGER << PROLOCATION + rgbFilePath + " no found!";         
        return false;
    }
    nOriWidth = mRgb.cols;
    nOriHeight = mRgb.rows;    
    nTotalFrameNumber = totalFrameNumber;
    return true;
}

Mat Kinect::retrievePointCloud(Mat& depthImage)
{
	ConvertProjectiveToRealWorld(depthImage);
	return mPointCloud;
}

void Kinect::ConvertProjectiveToRealWorld(Mat& depthImage)
{
	//mPointCloud.create(nFrameHeight,nFrameWidth,CV_32FC3);
	double fXToZ = 1.1114666461944582; //RealWorldXtoZ = tan(HFOV/2)*2;
	double fYToZ = 0.83359998464584351;//RealWorldYtoZ = tan(VFOV/2)*2;

	if (mRT.empty())
	{
		for (int i=0; i<nFrameHeight;i++)
		{
			float* data = mPointCloud.ptr<float>(i);
			ushort* depthData = depthImage.ptr<ushort>(i);
			for (int j=0; j<nFrameWidth;j++)
			{
				double fNormalizedX = (float)j /nFrameWidth - 0.5;
				double fNormalizedY = 0.5 - (float)i /nFrameHeight;
				float Z = *depthData++;
				*data++ = -(float)(fNormalizedX * Z * fXToZ);
				*data++ = (float)(fNormalizedY * Z * fYToZ);
				*data++ = Z;
			}
		}	
	}
	else
	{
		for (int i=0; i<nFrameHeight;i++)
		{
			float* data = mPointCloud.ptr<float>(i);
			ushort* depthData = depthImage.ptr<ushort>(i);
			for (int j=0; j<nFrameWidth;j++)
			{
				double fNormalizedX = (float)j /nFrameWidth - 0.5;
				double fNormalizedY = 0.5 - (float)i /nFrameHeight;
				float Z = *depthData++;

				Mat pointVector = ( Mat_<float>(4,1) << -(float)(fNormalizedX * Z * fXToZ), (float)(fNormalizedY * Z * fYToZ), Z, 1 );
				Mat pointVectorTransed = mRT * pointVector;			
				*data++ = pointVectorTransed.at<float>(0,0);
				*data++ = pointVectorTransed.at<float>(1,0);
				*data++ = pointVectorTransed.at<float>(2,0);
			}
		}	
	}
}

Point Kinect::ConvertRealWorldToProjective(float x, float y, float z)
{
	double fXToZ = 1.1114666461944582; //RealWorldXtoZ = tan(HFOV/2)*2;
	double fYToZ = 0.83359998464584351;//RealWorldYtoZ = tan(VFOV/2)*2;

	double fCoeffX = nFrameWidth / fXToZ;
	double fCoeffY = nFrameHeight / fYToZ;

	int nHalfXres = nFrameWidth / 2;
	int nHalfYres = nFrameHeight / 2;

	Point aProjective;
	aProjective.x = (float)fCoeffX * x / z + nHalfXres;
	aProjective.y = nHalfYres - (float)fCoeffY * y / z;

	return aProjective;
}

Mat Kinect::getDataContent(int kinectDataContent)
{
	switch(kinectDataContent)
	{
	case KINECT_RGB:
		if (!mRgb.empty()) 
			return mRgb; 
		return mErrorReturn;
		break;

	case KINECT_ORI_DEPTH:
		if (!mDepth.empty()) 
			return mDepth; 
		return mErrorReturn;
		break;

	case KINECT_MONOCOLOR_DEPTH:
		if (!mDepthMonoColor.empty()) 
			return mDepthMonoColor; 
		return mErrorReturn;
		break;

	case KINECT_COLOR_DEPTH:
		if (!mDepthColorful.empty()) 
			return mDepthColorful; 
		return mErrorReturn;
		break;

	case KINECTmPointCloud:
		if (!mPointCloud.empty()) 
			return mPointCloud; 
		return mErrorReturn;
		break;
	}
}

void Kinect::writeDataContent(string filePathWithoutFilesuffix, int kinectDataContent)
{
	if (kinectDataContent & KINECT_RGB)
	{	
		string filePath;
		filePath = filePathWithoutFilesuffix + ".jpg";
		if(!mRgb.empty()) 
			imwrite(filePath,mRgb);
	}

	if (kinectDataContent & KINECT_ORI_DEPTH)
	{
		string filePath;
		filePath = filePathWithoutFilesuffix + ".png";
		if(!mDepth.empty()) 
			imwrite(filePath,mDepth);
	}

	if (kinectDataContent & KINECT_MONOCOLOR_DEPTH)
	{
		string filePath;
		filePath = filePathWithoutFilesuffix + "_md.jpg";
		if(!mDepthMonoColor.empty()) 
			imwrite(filePath,mDepthMonoColor);
	}

	if (kinectDataContent & KINECT_COLOR_DEPTH)
	{
		string filePath;
		filePath = filePathWithoutFilesuffix + "_cd.jpg";
		if(!mDepthColorful.empty()) 
			imwrite(filePath,mDepthColorful);
	}

	if (kinectDataContent & KINECTmPointCloud)
	{
		// smooth
		Depth frame(mDepth);
		//frame.smooth();
		frame.smooth();
		mDepth = frame.data();
		// retrieve cloud
		retrievePointCloud(mDepth);
#ifdef MODELTYPE_PLY
		writePly(mPointCloud, mRgb, filePathWithoutFilesuffix, 1000);
#else 
		writeObj(mPointCloud, filePathWithoutFilesuffix, 1000);
#endif
	}
}

void Kinect::writePly(Mat& pointCloud, Mat& rgbImage, string filePathWithoutFilesuffix, int zmax, int zmin)
{
	string filePath;
	filePath = filePathWithoutFilesuffix + ".ply";
	stringstream tempfile;
	ofstream logfile(filePath.c_str());
	if (logfile.fail())
		cout << "file open failure"<< endl;
	int count = 0;
	logfile<<"ply"<<endl
		<<"format ascii 1.0"<<endl
		<<"element vertex ";
	for (int i=0; i<nFrameHeight;i++)
	{
		for (int j=0; j<nFrameWidth;j++)
		{
			float zVal = pointCloud.at<Vec3f>(i,j)[2];
			if ( zVal > zmin && zVal < zmax )
			{
				tempfile<<-pointCloud.at<Vec3f>(i,j)[0]/1000<<" "
					<<pointCloud.at<Vec3f>(i,j)[1]/1000<<" "
					<<pointCloud.at<Vec3f>(i,j)[2]/1000<<" "
					<<(int)rgbImage.at<Vec3b>(i,j)[2]<<" "
					<<(int)rgbImage.at<Vec3b>(i,j)[1]<<" "
					<<(int)rgbImage.at<Vec3b>(i,j)[0]<<endl;
				count ++ ;
			}			
		}
	}
	logfile<<count<<endl
		<<"property float x"<<endl
		<<"property float y"<<endl
		<<"property float z"<<endl
		<<"property uchar red"<<endl
		<<"property uchar green"<<endl
		<<"property uchar blue"<<endl
		<<"end_header"<<endl;
	logfile<<tempfile.str();
	logfile.close();	
}

void Kinect::writeObj(Mat& pointCloud, string filePathWithoutFilesuffix, int zmax, int zmin)
{
	string filePath;
	filePath = filePathWithoutFilesuffix + ".obj";
	stringstream tempfile;
	ofstream logfile(filePath.c_str());
	if (logfile.fail())
		cout << "file open failure"<< endl;
	int count = 0;
	int *index = new int[nFrameHeight*nFrameWidth];

	for (int i=0; i<nFrameHeight;i++)
	{
		for (int j=0; j<nFrameWidth;j++)
		{
			float zVal = pointCloud.at<Vec3f>(i,j)[2];
			if ( zVal > zmin && zVal < zmax )
			{
				tempfile<<"v "<<-pointCloud.at<Vec3f>(i,j)[0]/1000<<" "
					<<pointCloud.at<Vec3f>(i,j)[1]/1000<<" "
					<<pointCloud.at<Vec3f>(i,j)[2]/1000<<endl;
				count ++ ;
				index[i*nFrameWidth+j] = count;
			}
			else
				index[i*nFrameWidth+j] = 0;
		}
	}

	for (int i=0; i<nFrameHeight;i++)
	{
		for (int j=0; j<nFrameWidth;j++)
		{
			int nw = i*nFrameWidth+j;
			if (index[nw] == 0)
				continue;
			else	
			{	
				int ne = i*nFrameWidth+j+1;
				int sw = (i+1)*nFrameWidth+j;
				int se = (i+1)*nFrameWidth+j+1;
				if( index[ne] && index[sw] )
					tempfile<<"f "<<index[nw]<<" "<<index[ne]<<" "<<index[sw]<<endl;
				else
					continue;
				if( index[se] )
					tempfile<<"f "<<index[ne]<<" "<<index[se]<<" "<<index[sw]<<endl;
			}
		}
	}

	delete []index;

	logfile<<tempfile.str();
	logfile.close();		
}

void Kinect::setRtMatrix(bool isDegree = true, double alpha = 0, double beta = 0, double gamma = 0, double x = 0, double y = 0, double z = 0) //rotation order: z first(beta), x second(alpha), y third(gamma)
{
	if (isDegree)
	{
		alpha = degreeToRad(alpha);
		beta = degreeToRad(beta);
		gamma = degreeToRad(gamma);	
	}

	mRT =  ( Mat_<float>(4,4) << 
		cos(beta)*cos(gamma) - sin(alpha)*sin(beta)*sin(gamma), cos(beta)*sin(gamma) + cos(gamma)*sin(alpha)*sin(beta), -cos(alpha)*sin(beta), x ,							 
		-cos(alpha)*sin(gamma),                                  cos(alpha)*cos(gamma),            sin(alpha), y ,						 
		cos(gamma)*sin(beta) + cos(beta)*sin(alpha)*sin(gamma), sin(beta)*sin(gamma) - cos(beta)*cos(gamma)*sin(alpha),  cos(alpha)*cos(beta), z ,						     
		0													  , 0												      , 0					 , 1 );
}

bool Kinect::updataDataAVI()
{
    //read .avi and .dat
    if( !depthFileReader.eof() )
    {   
        do{
            skeletonFileReader.read((char *)&mSkeleton, sizeof(mSkeleton));
            skeletonFrameReader.read((char*)&i_ts_skeleton, sizeof(LONGLONG));        
        }while( mSkeleton._3dPoint[0].z == 0 ); // ignore skeleton not useful

        do{
            depthFileReader.read((char *)depthData, nOriWidth*nOriHeight*sizeof(ushort));
            depthFrameReader.read((char*)&i_ts_depth, sizeof(LONGLONG));        
        }while( i_ts_skeleton != i_ts_depth );  // ignore depth not equal to skeleton

        do{
            rgbVideoReader >> mRgb;
            colorFrameReader.read((char*)&i_ts_rgb, sizeof(LONGLONG));       
        }while( abs(i_ts_rgb - i_ts_depth) > 15 );  // ignore depth - rgb < 30

        mDepth = Mat(nOriHeight,nOriWidth,CV_16UC1,depthData);
        if( nOriWidth != nFrameWidth || nOriHeight != nFrameHeight )
        {
            cv::resize(mRgb, stCurFrame.rgb, Size(nFrameWidth,nFrameHeight));
            cv::resize(mDepth, stCurFrame.depth, Size(nFrameWidth,nFrameHeight));   
        }
        else
        {
            stCurFrame.rgb = mRgb.clone();
            stCurFrame.depth = mDepth.clone();
        }

        nCurFrameNumber++;
        return true;
    }
    else
    {
        rgbVideoReader.~VideoCapture();
        depthFileReader.close();
        return false;        
    }
}

bool Kinect::updataDataFolder()
{
    //read .jpg and .png
    if( nCurFrameNumber < nTotalFrameNumber )
    {
        stringstream fileNo; 
        fileNo << nCurFrameNumber; 
        string rgbFilePath = strFrameFolder + "\\" + fileNo.str() + ".jpg";
        string depthFilePath = strFrameFolder + "\\" + fileNo.str() + ".png"; 
        mRgb = imread(rgbFilePath);
        mDepth = imread(depthFilePath,-1);
        if( mDepth.data == NULL || mRgb.data == NULL )
            return false;
        int oriWidth = mRgb.cols;
        int oriHeight = mRgb.rows;

        if( oriWidth != nFrameWidth || oriHeight != nFrameHeight )
        {
            cv::resize(mRgb, stCurFrame.rgb, Size(nFrameWidth,nFrameHeight));
            cv::resize(mDepth, stCurFrame.depth, Size(nFrameWidth,nFrameHeight));        
        }
        else
        {
            stCurFrame.rgb = mRgb.clone();
            stCurFrame.depth = mDepth.clone();
        }

        //nCurFrameNumber++;
        return true;
    }
    else
        return false;
}

bool Kinect::updataDataFile()
{
	if( eOpenType == E_OT_AVI )
	{
		return updataDataAVI();
	}
	else if( eOpenType == E_OT_FOLDER )
    {
        return updataDataFolder();
	}
	return false;
}

bool Kinect::writeOneFrame()
{
    if( eRecordFileType == E_RT_AVI )
    {   
        //boost::mutex::scoped_lock lock( mtxColorWriter ); 
	    rgbVideoWriter << mRgb;
        depthFileWriter.write((char *)mDepth.data, nFrameWidth*nFrameHeight*sizeof(ushort));
    }
    else if( eRecordFileType == E_RT_FOLDER )
    {
        char path[MAX_PATH];
        sprintf(path, "%s\\%d.jpg", (strDumpPath+strDumpFile).c_str(), nCurRecordFrameNumber);
        imwrite(path,mRgb);
        sprintf(path, "%s\\%d.png", (strDumpPath+strDumpFile).c_str(), nCurRecordFrameNumber);
        imwrite(path,mDepth);
    }
	return true;
}

bool Kinect::writeOneFrameDepth()    // .png
{
    if( eRecordFileType == E_RT_AVI )
    {   
        depthFrameCount++;
        {
 //           boost::mutex::scoped_lock lock( mtxDepthWriter ); 
            depthFileWriter.write((char *)mDepth.data, nFrameWidth*nFrameHeight*sizeof(ushort));
            depthFileWriter.flush();
            depthFrameWriter.write((char *)&i_ts_depth, sizeof(LONGLONG));
            depthFrameWriter.flush();        
        }

    }
    else if( eRecordFileType == E_RT_FOLDER )
    {
        char path[MAX_PATH];
        sprintf(path, "%s\\%lld.png", (strDumpPath+strDumpFile).c_str(), i_ts_depth);
        imwrite(path,mDepth);
    }
	return true;
}

bool Kinect::writeOneFrameColor()    // .jpg
{
    if( eRecordFileType == E_RT_AVI )
    {      
        colorFrameCount++;
        {
  //          boost::mutex::scoped_lock lock( mtxColorWriter ); 
	        rgbVideoWriter << mRgb;
            colorFrameWriter.write((char *)&i_ts_rgb, sizeof(LONGLONG));
            colorFrameWriter.flush();        
        }
    }
    else if( eRecordFileType == E_RT_FOLDER )
    {
        char path[MAX_PATH];
        sprintf(path, "%s\\%lld.jpg", (strDumpPath+strDumpFile).c_str(), i_ts_rgb);
        imwrite(path,mRgb);
    }
	return true;
}

bool Kinect::writeOneFrameSkeleton() // .dat
{
    skeletonFrameCount++;
    {
 //       boost::mutex::scoped_lock lock( mtxSkeletonWriter ); 
        skeletonFrameWriter.write((char *)&i_ts_skeleton, sizeof(LONGLONG));
        skeletonFrameWriter.flush();
        skeletonFileWriter.write((char*)&mSkeleton, sizeof(SLR_ST_Skeleton));
        skeletonFileWriter.flush();    
    }
    return true;
}

bool Kinect::beginRecordAVI()
{
    _mkdir( strDumpFile.c_str() );

    //rgbTest = VideoWriter(strDumpPath + strDumpFile + "\\test.avi",CV_FOURCC('M','J','P','G'),30,Size(nFrameWidth,nFrameHeight),true);

    string rgbFilePath = strDumpPath + strDumpFile + COLOR_REC_DATA;
    string rgbFramePath = strDumpPath + strDumpFile + COLOR_REC_FRAME;

    string depthFilePath= strDumpPath + strDumpFile + DEPTH_REC_DATA;
    string depthFramePath= strDumpPath + strDumpFile + DEPTH_REC_FRAME;

    string skeletonFilePath= strDumpPath + strDumpFile + SKELE_REC_DATA;
    string skeletonFramePath= strDumpPath + strDumpFile + SKELE_REC_FRAME;

    depthFrameCount = 0;
    colorFrameCount = 0;
    skeletonFrameCount = 0;

	rgbVideoWriter = VideoWriter(rgbFilePath,CV_FOURCC('M','J','P','G'),30,Size(nFrameWidth,nFrameHeight),true);
	depthFileWriter.open(depthFilePath.c_str(),ios::binary);	
    skeletonFileWriter.open(skeletonFilePath.c_str(),ios::binary);

    colorFrameWriter.open(rgbFramePath, ios::binary);
    depthFrameWriter.open(depthFramePath, ios::binary);
    skeletonFrameWriter.open(skeletonFramePath, ios::binary);

    string logPath = strDumpPath + strDumpFile + "\\log.txt";
//     recordLogFile.open(logPath);
// 
//     recordTimer.restart();

	return true;
}

bool Kinect::endRecordAVI()
{
//     recordElapseTime = recordTimer.elapsed();
//     cout << recordElapseTime << endl;

    recordLogFile << "Record Time: " << recordElapseTime << " s" << endl
                  << "Color: " << colorFrameCount << " frames; " << colorFrameCount/recordElapseTime <<" fps" << endl
                  << "Depth: " << depthFrameCount << " frames; " << depthFrameCount/recordElapseTime <<" fps"<< endl
                  << "Skeleton: " << skeletonFrameCount << " frames; " << skeletonFrameCount/recordElapseTime <<" fps" <<endl;
    recordLogFile.close();

    //{
    //    boost::mutex::scoped_lock lock( mtxTestWriter );  
    //    rgbTest.~VideoWriter();   
    //}
    {
 //       boost::mutex::scoped_lock lock1( mtxColorWriter ); 
        rgbVideoWriter.~VideoWriter();
        colorFrameWriter.close();    
    }
    {
 //       boost::mutex::scoped_lock lock2( mtxDepthWriter ); 
	    depthFileWriter.close();
        depthFrameWriter.close();    
    }
    {
   //     boost::mutex::scoped_lock lock3( mtxSkeletonWriter ); 
        skeletonFileWriter.close();
        skeletonFrameWriter.close();    
    }

	return true;
}

bool Kinect::endRecordFolder()
{
    skeletonFileWriter.close();
	return true;
}

bool Kinect::beginRecordFolder()
{
    string skeletonFilePath= strDumpPath + strDumpFile + "_s.dat";
    skeletonFileWriter.open(skeletonFilePath.c_str(),ios::binary);

    //boost::filesystem::path boostpath( strDumpPath + strDumpFile );
    //boost::filesystem::create_directories( boostpath );
    return true;
}


void Kinect::runCaptureThread()
{
    //boost::function0< void> fc = boost::bind(&Kinect::colorCaptureThread,this);
    //boost::function0< void> fd = boost::bind(&Kinect::depthCaptureThread,this);
    //boost::function0< void> fs = boost::bind(&Kinect::skeletonCaptureThread,this);

    //trdColorCapture = boost::thread(fc);
    //trdDepthCapture = boost::thread(fd);
    //trdSkeletonCapture = boost::thread(fs);

 //   boost::function0< void> ft = boost::bind(&Kinect::singleCaptureThread,this);
 //   trdSingleCapture = boost::thread(ft);
    

    //trdColorCapture.join();
    //trdDepthCapture.join();
    //trdSkeletonCapture.join();

    //trdCapture = boost::thread(f);
    //trdCapture.join();
}

bool Kinect::setDumpPath(string path)
{
    //try{
    //    struct tm *tt;
    //    time_t t = time(NULL);
    //    tt = gmtime(&t);
    //    stringstream ssPath;
    //    ssPath << path << "\\" << tt->tm_year+1900 << "-"
    //                           << tt->tm_mon+1 << "-"
    //                           << tt->tm_mday << "-"
    //                           << tt->tm_hour << "-"
    //                           << tt->tm_min << "-"
    //                           << tt->tm_sec << "\\";
    //    strDumpPath = ssPath.str();
    //    boost::filesystem::path boostpath( strDumpPath );
    //    //if( boost::filesystem::is_directory( boostpath ) )
    //    {
    //        if( !boost::filesystem::exists( boostpath ) )
    //        {
    //            boost::filesystem::create_directories( boostpath );
    //        }
    //    }
    //    //else
    //    return true;
    //}
    //catch(boost::filesystem::filesystem_error e) { 
    //    cout << e.what() << endl;
    //}
    return true;
}


void Kinect::beginRecord(E_RECORD_TYPE rtype, string path) 
{ 
    eKinectRecordStatus = E_KRS_BEGIN;
    eRecordFileType = rtype;
    strDumpFile = path;
    nCurRecordFrameNumber = 0;
}
