#include "msKinect.h"


MsKinect::MsKinect()
{
    m_pNuiSensor = NULL;
    bFirstFrameGet = false;
    bUserSkeletonTracked = false;
    nUserId = -1;

    hNextFrameReady = CreateEvent( NULL, FALSE, FALSE, NULL);
    hNextSkeletonReady = CreateEvent( NULL, FALSE, FALSE, NULL);

    //debugFile.open("frameId.txt",ios::out);
}

MsKinect::~MsKinect()
{
    //debugFile.close();
//     trdColorCapture.interrupt();
//     trdDepthCapture.interrupt();
//     trdSkeletonCapture.interrupt();
    if (m_pNuiSensor)
    {
        m_pNuiSensor->NuiShutdown();
    }
    if (hNextDepth != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hNextDepth);
    }
    if (hNextColor != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hNextColor);
    }
    if (hNextSkeleton != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hNextSkeleton);
    }
    CloseHandle(hNextFrameReady);
    CloseHandle(hNextSkeletonReady);
    //debugFile.close();
    bIsOpen = false;
}

bool MsKinect::init(bool mirror,bool changeView,int height,int width)
{   
    INuiSensor * pNuiSensor;
    HRESULT hr;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return false;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }



    eOpenType = E_OT_MSKINECT;

    iniFile.SetPathName(L".\\settings.ini");
    BOOL bTrackUpper = iniFile.GetBool(L"Track", L"bUpperBody", 1);
    //smooth parameter set
    someLatencyParams.fSmoothing = iniFile.GetDouble(L"SmoothSkeleton", L"fSmoothing", 0.5);
    someLatencyParams.fCorrection = iniFile.GetDouble(L"SmoothSkeleton", L"fCorrection", 0.1);
    someLatencyParams.fPrediction = iniFile.GetDouble(L"SmoothSkeleton", L"fPrediction", 0.5);
    someLatencyParams.fJitterRadius = iniFile.GetDouble(L"SmoothSkeleton", L"fJitterRadius", 0.1);
    someLatencyParams.fMaxDeviationRadius = iniFile.GetDouble(L"SmoothSkeleton", L"fMaxDeviationRadius", 0.1);
     

	hr = m_pNuiSensor->NuiInitialize( NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_COLOR);
	
    if( hr != S_OK ) { cout<<"NuiInitialize failed"<<endl; return false; }

    //hNextColor = CreateEvent(NULL, TRUE, FALSE, NULL);
	hr = m_pNuiSensor->NuiImageStreamOpen(
        NUI_IMAGE_TYPE_COLOR,
        NUI_IMAGE_RESOLUTION_640x480, 0, 2, 
        NULL, 
        &hColorStream);
	if( FAILED( hr ) ) { cout<<"Could not open image stream video"<<endl; return false; }

    //hNextDepth = CreateEvent(NULL, TRUE, FALSE, NULL);
	hr = m_pNuiSensor->NuiImageStreamOpen( 
        NUI_IMAGE_TYPE_DEPTH, 
        NUI_IMAGE_RESOLUTION_640x480, 0, 2, 
        NULL, 
        &hDepthStream);
	if( FAILED( hr ) ) { cout<<"Could not open depth stream video"<<endl; return false; }
    
    //hNextSkeleton = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(bTrackUpper)
    {
        hr = m_pNuiSensor->NuiSkeletonTrackingEnable( 
            hNextSkeleton, 
            NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT | NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS | NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA );    
    }
    else
    {
        hr = m_pNuiSensor->NuiSkeletonTrackingEnable( 
            hNextSkeleton, 
            NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS | NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA ); 
    }
    if( FAILED( hr ) ) { cout<<"Could not open skeleton stream"<<endl; return false; }
    
    hNextFrame = CreateEvent(NULL, FALSE, FALSE, NULL);
    hr = m_pNuiSensor->NuiSetFrameEndEvent(hNextFrame, NUI_IMAGE_STREAM_FLAG_SUPPRESS_NO_FRAME_DATA);
    if( FAILED( hr ) ) { cout<<"Could not open frame end handle"<<endl; return false; }


	nFrameWidth = width;
	nFrameHeight = height;

	mRgb.create( nFrameHeight, nFrameWidth, CV_8UC3 );
	mDepth.create( nFrameHeight, nFrameWidth, CV_16UC1 );
	mPointCloud.create(nFrameHeight,nFrameWidth,CV_32FC3);

    oriDepthImg.create(480,640,CV_16UC1);

	bIsOpen = true;

    runCaptureThread();

	return true;
}

bool MsKinect::close()
{
	if(bIsOpen)
	{
//         trdColorCapture.interrupt();
//         trdDepthCapture.interrupt();
//         trdSkeletonCapture.interrupt();

		NuiShutdown();
		bIsOpen = false;
	}
	return true;
}

bool MsKinect::updateData()
{
    while( eOpenType == E_OT_DEFAULT );
    if( eOpenType <  E_OT_AVI) // from kinect driver
    {
        switch( eOpenType  )
        {
        case E_OT_MS_KEEP:
            //return updataDataMSKeep();
            break;
        case E_OT_MSKINECT:
            return updataDataDeviceSingle();
            break;
        case E_OT_AVI:
            return updataDataAVI();
            break;
        default:
            break;
        }
    }
    else
    {
        return updataDataFile();
    }
}

bool MsKinect::queryFrame()
{
    HRESULT hr = S_OK;
    const NUI_IMAGE_FRAME *pNewFrame = NULL;
    const NUI_IMAGE_FRAME *pFrame = NULL;

    // update color frame
    do
    {
        hr = NuiImageStreamGetNextFrame( hColorStream, 0, &pNewFrame);
        if( hr == S_OK )
        {
            // drop stale frame
            //if(pFrame != NULL)
            //    debugFile << "dc " << pFrame->dwFrameNumber << " ";
            NuiImageStreamReleaseFrame(hColorStream, pFrame);  
            //pFrame = NULL;
        }
        pFrame = pNewFrame;
    }while(hr == S_OK);
    debugFile << " " << hr << endl;

    if( pFrame == NULL )
    {
        //cout << "no color" << endl;
        return false;    
    }

    const NUI_IMAGE_FRAME *pNewDepthFrame = NULL;
    const NUI_IMAGE_FRAME *pDepthFrame = NULL;
    // update depth frame
    do
    {
        hr = NuiImageStreamGetNextFrame( hDepthStream, 0, &pNewDepthFrame);
        //m_pNuiSensor->NuiSkeletonGetNextFrame(0, &imageFrameSkeleton);
        if( hr == S_OK )
        {
            // drop stale frame
            //if(pDepthFrame != NULL)
            //    debugFile << "dd " << pDepthFrame->dwFrameNumber << " ";
            NuiImageStreamReleaseFrame(hDepthStream, pDepthFrame);
        }
        pDepthFrame = pNewDepthFrame;
    }while(hr == S_OK);

    //if(hr == E_INVALIDARG)
    //    debugFile << " E_INVALIDARG" << endl;
    //else if(hr == E_NUI_FRAME_NO_DATA)
    //    debugFile << " E_NUI_FRAME_NO_DATA" << endl;
    //else if(hr == E_POINTER)
    //    debugFile << " E_POINTER" << endl;

    if( pDepthFrame == NULL )
    {
        //cout << "no depth" << endl;
        return false;    
    }

    // update skeleton frame
    hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &imageFrameSkeleton);
    if( hr != S_OK )
    {
        //cout << "no skeleton" << endl;
        return false;
    }
    //debugFile << " " << hr << endl;

    //do
    //{
    //    hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &imageFrameSkeleton);
    //}while(hr == S_OK);
    //if( imageFrameSkeleton )
    //{
    //    cout << "no skeleton" << endl;
    //    return false;   
    //}

//    boost::mutex::scoped_lock lock( mtxFrame );
    
    //process Color
    {
        pTextureColor = pFrame->pFrameTexture;
        pTextureColor->LockRect( 0, &LockedRectColor, NULL, 0 );
        BYTE * pBuffer = (BYTE*) LockedRectColor.pBits;
        i_ts_rgb = pFrame->liTimeStamp.QuadPart;
        debugFile << i_ts_rgb << " ";
        for (int j = 0; j < nFrameHeight; j++ )
        {
            uchar* desData = mRgb.ptr<uchar>(j);
            for (int i = 0; i < nFrameWidth; i++) 
            {
                *desData++ = pBuffer[(j*nFrameWidth+i)*4];
                *desData++ = pBuffer[(j*nFrameWidth+i)*4+1];;
                *desData++ = pBuffer[(j*nFrameWidth+i)*4+2];;
            }
        }        
        pTextureColor->UnlockRect(0);
        NuiImageStreamReleaseFrame(hColorStream, pFrame);
    }


    //process Depth
    {
        pTextureDepth = pDepthFrame->pFrameTexture;
        pTextureDepth->LockRect( 0, &LockedRectDepth, NULL, 0 );
        LONG plColorX, plColorY;
        unsigned short* pBuffer = (unsigned short*) LockedRectDepth.pBits;
        i_ts_depth = pDepthFrame->liTimeStamp.QuadPart;
        debugFile << i_ts_depth << " ";

        LONG* colorCoordinates = new LONG[nFrameHeight*nFrameWidth*2];
        m_pNuiSensor->NuiImageGetColorPixelCoordinateFrameFromDepthPixelFrameAtResolution(
            NUI_IMAGE_RESOLUTION_640x480,
            NUI_IMAGE_RESOLUTION_640x480,
            nFrameHeight*nFrameWidth,
            pBuffer,
            nFrameHeight*nFrameWidth*2,
            colorCoordinates
            );
        mDepth.setTo(0);
        for (int j = 0; j < nFrameHeight; j++ )
        {
            for (int i = 0; i < nFrameWidth ; i++)
            {
                int index = j * nFrameWidth + i;

                int px = colorCoordinates[index*2];
                int py = colorCoordinates[index*2+1];

                if ( px >= 0 && px < 640 && py >=0 && py < 480 )
                    mDepth.at<ushort>(py,px) = NuiDepthPixelToDepth( pBuffer[index] );
                else
                    mDepth.at<ushort>(py,px) = 0;
            }
        }        
        delete colorCoordinates;

        pTextureDepth->UnlockRect(0);
        NuiImageStreamReleaseFrame(hDepthStream, pDepthFrame);
    }

    SetEvent(hNextFrameReady);

    //process Skeleton
    {
        // track not initialized
        if( !bUserSkeletonTracked || nUserId == -1 )
        {
            float closestDistance = 100000.0f;
            for ( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
            {
                NUI_SKELETON_TRACKING_STATE trackingState = imageFrameSkeleton.SkeletonData[i].eTrackingState;

                if (trackingState != NUI_SKELETON_NOT_TRACKED)
                {
                    if (imageFrameSkeleton.SkeletonData[i].Position.z < closestDistance)
                    {
                        nUserId = imageFrameSkeleton.SkeletonData[i].dwTrackingID;
                        closestDistance = imageFrameSkeleton.SkeletonData[i].Position.z;
                    }
                    bUserSkeletonTracked = true;
                }
            }
            if (nUserId > 0)
            {
                DWORD closestIDs[NUI_SKELETON_MAX_TRACKED_COUNT] = {nUserId, 0};
                m_pNuiSensor->NuiSkeletonSetTrackedSkeletons(closestIDs); // Track this skeleton
            }
            else
            {
                // track lose(maybe not execute? I don't know)
                bUserSkeletonTracked = false;
                nUserId = -1;
                //debugFile << "ini fail " << endl;
                return false;                
            }
        }

        // ensure the idx of the tracked person, which should be equal to nUserId set before
        int idx = -1;
        for ( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
        {
            if (imageFrameSkeleton.SkeletonData[i].dwTrackingID == nUserId)
            {
                // find array index of SkeletonData with the same dwTrackingID
                idx = i;
                break;
            }
        } 
        if( idx == -1 ) // trackd user has lost, retrack another person
        {
            bUserSkeletonTracked = false;
            nUserId = -1;
            //debugFile << "track lost " << endl;
            return false; 
        }

        i_ts_skeleton = imageFrameSkeleton.liTimeStamp.QuadPart;  

        debugFile << i_ts_skeleton << " " << endl;
        // smooth the skeleton data
        //const NUI_TRANSFORM_SMOOTH_PARAMETERS someLatencyParams = {0.5f,0.1f,0.5f,0.1f,0.1f};  //best
        m_pNuiSensor->NuiTransformSmooth(&imageFrameSkeleton,&someLatencyParams); //NULL modify xu
        for(int i = 0 ; i < 20 ; i++)
        {
            Vector4 spacePoint = imageFrameSkeleton.SkeletonData[idx].SkeletonPositions[i];
            mSkeleton._3dPoint[i].x = spacePoint.x;
            mSkeleton._3dPoint[i].y = spacePoint.y;
            mSkeleton._3dPoint[i].z = spacePoint.z;
            mSkeleton._3dPoint[i].w = spacePoint.w;

            LONG x, y;
            USHORT depth;
            LONG cx,cy;
            NuiTransformSkeletonToDepthImage( spacePoint, &x, &y, &depth );
            m_pNuiSensor->NuiImageGetColorPixelCoordinatesFromDepthPixel(NUI_IMAGE_RESOLUTION_640x480,0,x,y,depth,&cx,&cy);

            mSkeleton._2dPoint[i].x = (int)(cx);
            mSkeleton._2dPoint[i].y = (int)(cy);
        }     
        SetEvent(hNextSkeletonReady);
    }
    
    //cout << "ok" << endl;


    nCurFrameNumber++; 
    return true;
}

bool MsKinect::querySkeleton()
{
    //while(1)
    //{
    //    DWORD ret = WaitForSingleObject(hNextSkeleton, INFINITE);

    //    if ( SUCCEEDED( m_pNuiSensor->NuiSkeletonGetNextFrame(0, &imageFrameSkeleton) ) )
    //    {
    //        // track not initialized
    //        if( !bUserSkeletonTracked || nUserId == -1 )
    //        {
    //            float closestDistance = 100000.0f;
    //            for ( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
    //            {
    //                NUI_SKELETON_TRACKING_STATE trackingState = imageFrameSkeleton.SkeletonData[i].eTrackingState;

    //                if (trackingState != NUI_SKELETON_NOT_TRACKED)
    //                {
    //                    if (imageFrameSkeleton.SkeletonData[i].Position.z < closestDistance)
    //                    {
    //                        nUserId = imageFrameSkeleton.SkeletonData[i].dwTrackingID;
    //                        closestDistance = imageFrameSkeleton.SkeletonData[i].Position.z;
    //                    }
    //                    bUserSkeletonTracked = true;
    //                }
    //            }
    //            if (nUserId > 0)
    //            {
    //                DWORD closestIDs[NUI_SKELETON_MAX_TRACKED_COUNT] = {nUserId, 0};
    //                m_pNuiSensor->NuiSkeletonSetTrackedSkeletons(closestIDs); // Track this skeleton
    //            }
    //            else
    //            {
    //                // track lose(maybe not execute? I don't know)
    //                bUserSkeletonTracked = false;
    //                nUserId = -1;
    //                continue;                
    //            }
    //        }

    //        // ensure the idx of the tracked person, which should be equal to nUserId set before
    //        int idx = -1;
    //        for ( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
    //        {
    //            if (imageFrameSkeleton.SkeletonData[i].dwTrackingID == nUserId)
    //            {
    //                // find array index of SkeletonData with the same dwTrackingID
    //                idx = i;
    //                break;
    //            }
    //        } 
    //        if( idx == -1 ) // trackd user has lost, retrack another person
    //        {
    //            bUserSkeletonTracked = false;
    //            nUserId = -1;
    //            continue;
    //        }

    //    
    //        {
    //            boost::mutex::scoped_lock lock( mtxSkeleton );
    //            i_ts_skeleton = imageFrameSkeleton.liTimeStamp.QuadPart;        
    //            // smooth the skeleton data
    //            //const NUI_TRANSFORM_SMOOTH_PARAMETERS someLatencyParams = {0.5f,0.1f,0.5f,0.1f,0.1f};  //best
    //            m_pNuiSensor->NuiTransformSmooth(&imageFrameSkeleton,&someLatencyParams); //NULL modify xu
    //            for(int i = 0 ; i < 20 ; i++)
    //            {
    //                Vector4 spacePoint = imageFrameSkeleton.SkeletonData[idx].SkeletonPositions[i];
    //                mSkeleton._3dPoint[i].x = spacePoint.x;
    //                mSkeleton._3dPoint[i].y = spacePoint.y;
    //                mSkeleton._3dPoint[i].z = spacePoint.z;
    //                mSkeleton._3dPoint[i].w = spacePoint.w;

    //                LONG x, y;
    //                USHORT depth;
    //                LONG cx,cy;
    //                NuiTransformSkeletonToDepthImage( spacePoint, &x, &y, &depth );
    //                m_pNuiSensor->NuiImageGetColorPixelCoordinatesFromDepthPixel(NUI_IMAGE_RESOLUTION_640x480,0,x,y,depth,&cx,&cy);

    //                mSkeleton._2dPoint[i].x = (int)(cx);
    //                mSkeleton._2dPoint[i].y = (int)(cy);
    //            }           
    //        }

    //        break;
    //    }
    //}

    return true;  
}


bool MsKinect::queryDepth()
{
//    DWORD ret = WaitForSingleObject(hNextDepth, INFINITE);
//
//    if ( SUCCEEDED( m_pNuiSensor->NuiImageStreamGetNextFrame( hDepthStream, 0, pImageFrameDepth) ) )
//    {
//	    pTextureDepth = pImageFrameDepth->pFrameTexture;
//	    pTextureDepth->LockRect( 0, &LockedRectDepth, NULL, 0 );
//    
//	    LONG plColorX, plColorY;
//        unsigned short* pBuffer = (unsigned short*) LockedRectDepth.pBits;
//        LONG* colorCoordinates = new LONG[nFrameHeight*nFrameWidth*2];
//
//        m_pNuiSensor->NuiImageGetColorPixelCoordinateFrameFromDepthPixelFrameAtResolution(
//            NUI_IMAGE_RESOLUTION_640x480,
//            NUI_IMAGE_RESOLUTION_640x480,
//            nFrameHeight*nFrameWidth,
//            pBuffer,
//            nFrameHeight*nFrameWidth*2,
//            colorCoordinates
//            );
//
//        {
//            boost::mutex::scoped_lock lock( mtxDepth );
//            i_ts_depth = pImageFrameDepth->liTimeStamp.QuadPart;
//            mDepth.setTo(0);
//            for (int j = 0; j < nFrameHeight; j++ )
//	        {
//                for (int i = 0; i < nFrameWidth ; i++)
//		        {
//                    int index = j * nFrameWidth + i;
//
//                    int px = colorCoordinates[index*2];
//                    int py = colorCoordinates[index*2+1];
//
//			        if ( px >= 0 && px < 640 && py >=0 && py < 480 )
//				        mDepth.at<ushort>(py,px) = NuiDepthPixelToDepth( pBuffer[index] );
//                    else
//                        mDepth.at<ushort>(py,px) = 0;
//		        }
//	        }        
//        }
//
//
//        delete colorCoordinates;
//        pTextureDepth->UnlockRect(0);
//        m_pNuiSensor->NuiImageStreamReleaseFrame(hDepthStream, pImageFrameDepth);
//    }
//
//
//
	return true;
}

bool MsKinect::queryColor()
{
//    DWORD ret = WaitForSingleObject(hNextColor, INFINITE);
//
//    if ( SUCCEEDED( m_pNuiSensor->NuiImageStreamGetNextFrame( hColorStream, 0, pImageFrameColor) ) )
//    {
//	    pTextureColor = pImageFrameColor->pFrameTexture;
//	    pTextureColor->LockRect( 0, &LockedRectColor, NULL, 0 );
//	    BYTE * pBuffer = (BYTE*) LockedRectColor.pBits;
//        
//        {// lock mRgb and i_ts_rgb
//            boost::mutex::scoped_lock lock( mtxColor );
//            i_ts_rgb = pImageFrameColor->liTimeStamp.QuadPart;
//	        for (int j = 0; j < nFrameHeight; j++ )
//	        {
//		        uchar* desData = mRgb.ptr<uchar>(j);
//		        for (int i = 0; i < nFrameWidth; i++) 
//		        {
//			        *desData++ = pBuffer[(j*nFrameWidth+i)*4];
//			        *desData++ = pBuffer[(j*nFrameWidth+i)*4+1];;
//			        *desData++ = pBuffer[(j*nFrameWidth+i)*4+2];;
//		        }
//	        }        
//        }
//
//        pTextureColor->UnlockRect(0);
//        m_pNuiSensor->NuiImageStreamReleaseFrame(hColorStream, pImageFrameColor);
//
//        nCurFrameNumber++;
//        //lock.unlock();
//    }
//
	return true;
}

bool MsKinect::updataDataDeviceSingle()
{
    if( !bIsOpen )
        return false;

    if( WAIT_OBJECT_0 == WaitForSingleObject(hNextFrameReady, 100) )
    {
//        boost::mutex::scoped_lock lock( mtxFrame );    

        mTestRgb = mRgb.clone();
        if( WAIT_OBJECT_0 == WaitForSingleObject(hNextSkeletonReady, 10)  
            && i_ts_skeleton != -1 && mSkeleton._3dPoint[NUI_SKELETON_POSITION_HEAD].z != 0 )
        {
            int x = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_LEFT].x;
            int y = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_LEFT].y;
            int xw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_LEFT].x;
            int yw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_LEFT].y;
            int radius = sqrt( pow((float)xw-x,2) + pow((float)yw-y,2) );
            circle(mTestRgb, Point(x,y) , radius, Scalar(0,0,255), 4);

            x = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_RIGHT].x;
            y = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_RIGHT].y;
            xw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_RIGHT].x;
            yw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_RIGHT].y;
            radius = sqrt( pow((float)xw-x,2) + pow((float)yw-y,2) );
            circle(mTestRgb, Point(x,y) , radius, Scalar(0,255,0), 4);  

            char distance[80];
            sprintf(distance, "distance: %0.2f m", mSkeleton._3dPoint[NUI_SKELETON_POSITION_HEAD].z );
            putText(mTestRgb, string(distance), Point(160, 60), FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,0),2); 
        }
        else
        {
            putText(mTestRgb, string("Searching for hand"), Point(160, 60), FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,0),2); 
        }

    }
    return true;
}


bool MsKinect::updataDataDevice()
{

    if( !bIsOpen )
        return false;

    while( 1 )
    {
        LONGLONG rgbTime = -1;
        LONGLONG skeletonTime = -1;
        LONGLONG depthTime = -1;
        {
 //           boost::mutex::scoped_lock lock( mtxColor );
            rgbTime = i_ts_rgb;
        }
        {
 //           boost::mutex::scoped_lock lock( mtxSkeleton );
            skeletonTime = i_ts_skeleton;
        }
        {
  //          boost::mutex::scoped_lock lock( mtxDepth );
            depthTime = i_ts_depth;
        }

        if( rgbTime != -1 ) //&& skeletonTime != -1 &&  abs( rgbTime - skeletonTime ) < 15
            break;
    }


    {// get test rgb image(with hand circle around)
//        boost::mutex::scoped_lock lock( mtxFrame );    

        {
  //          boost::mutex::scoped_lock lock( mtxColor );
            mTestRgb = mRgb.clone();
            stCurFrame.rgb = mRgb.clone();
        }

        {
  //          boost::mutex::scoped_lock lock( mtxSkeleton );
            if( i_ts_skeleton != -1 && mSkeleton._3dPoint[NUI_SKELETON_POSITION_HEAD].z != 0 )
            {
                int x = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_LEFT].x;
                int y = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_LEFT].y;
                int xw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_LEFT].x;
                int yw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_LEFT].y;
                int radius = sqrt( pow((float)xw-x,2) + pow((float)yw-y,2) );
                circle(mTestRgb, Point(x,y) , radius, Scalar(0,0,255), 4);

                x = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_RIGHT].x;
                y = mSkeleton._2dPoint[NUI_SKELETON_POSITION_HAND_RIGHT].y;
                xw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_RIGHT].x;
                yw = mSkeleton._2dPoint[NUI_SKELETON_POSITION_WRIST_RIGHT].y;
                radius = sqrt( pow((float)xw-x,2) + pow((float)yw-y,2) );
                circle(mTestRgb, Point(x,y) , radius, Scalar(0,255,0), 4);  

                char distance[80];
                sprintf(distance, "distance: %0.2f m", mSkeleton._3dPoint[NUI_SKELETON_POSITION_HEAD].z );
                putText(mTestRgb, string(distance), Point(160, 60), FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,0),2); 
            }
            else
            {
                putText(mTestRgb, string("Searching for hand"), Point(160, 60), FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,0),2); 
            }
        }
    }


    return true;
}

bool MsKinect::tiltToAngle(int angle)
{
	m_pNuiSensor->NuiCameraElevationSetAngle(angle);
	return true;
}

void MsKinect::_beginRecord()
{
    switch( eRecordFileType )
    {
    case E_RT_NOT_SET:
        //begin record ms build-in record function
        break;
    case E_RT_AVI:
        beginRecordAVI();
        break;
    case E_RT_FOLDER:
        beginRecordFolder();
        break;
    default:
        break;
    }
}

void MsKinect::_endRecord()
{
    switch( eRecordFileType )
    {
    case E_RT_NOT_SET:
        //begin record ms build-in record function
        break;
    case E_RT_AVI:
        endRecordAVI();
        break;
    case E_RT_FOLDER:
        endRecordFolder();
        break;
    default:
        break;
    }
}

void MsKinect::singleCaptureThread()
{
   // try{
        bIsOpen = true;

        while(WaitForSingleObject(hNextFrame, INFINITE) == WAIT_OBJECT_0)
        {
			queryFrame();
//             if(queryFrame())
//             {
//                 if(eKinectRecordStatus == E_KRS_DURATION)
//                 {
//                     writeOneFrameColor();
//                     writeOneFrameDepth(); 
//                     writeOneFrameSkeleton(); 
//                 }
//                 else if(eKinectRecordStatus == E_KRS_BEGIN)
//                 {
//                     _beginRecord();
//                     eKinectRecordStatus = E_KRS_DURATION;
//                 }
//                 else if(eKinectRecordStatus == E_KRS_END)
//                 {
//                     eKinectRecordStatus = E_KRS_DEFAULT;
//                     _endRecord(); 
//                 }            
//             }            
            //boost::this_thread::interruption_point();  
        } 
 //   }
//     catch(/*boost::thread_interrupted&*/)
//     {
//     }
}

void MsKinect::colorCaptureThread()
{
    //try{
        bIsOpen = true;

        while(queryColor())
        {
            if(eKinectRecordStatus == E_KRS_DURATION)
            {
                writeOneFrameColor();
            }
            else if(eKinectRecordStatus == E_KRS_BEGIN)
            {
                _beginRecord();
                eKinectRecordStatus = E_KRS_DURATION;
            }
            else if(eKinectRecordStatus == E_KRS_END)
            {
                eKinectRecordStatus = E_KRS_DEFAULT;
                _endRecord(); 
            }
            //boost::this_thread::interruption_point();  
        } 
//     }
//     catch(/*boost::thread_interrupted&*/)
//     {
//     }
}

void MsKinect::depthCaptureThread()
{
   // try{
        bIsOpen = true;
        while(queryDepth())
        {
            if(eKinectRecordStatus == E_KRS_DURATION)
            {
                writeOneFrameDepth(); 
            }
            //boost::this_thread::interruption_point();  
        } 
//     }
//     catch(boost::thread_interrupted&)
//     {
//     }
}


void MsKinect::skeletonCaptureThread()
{
  //  try{
        bIsOpen = true;
        while(querySkeleton())
        {
            if(eKinectRecordStatus == E_KRS_DURATION)
            {
                writeOneFrameSkeleton();
            }
           // boost::this_thread::interruption_point();  
        } 
//     }
//     catch(boost::thread_interrupted&)
//     {
//     }
}