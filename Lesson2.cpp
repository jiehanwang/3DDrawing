#include <windows.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include <gl\glut.h>
#include "NuiApi.h"
#include "opencv2/opencv.hpp"
#include "msKinect.h"

MsKinect m_pKinect;


HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default

// GLfloat	rtri;				// Angle For The Triangle ( NEW )
// GLfloat	rquad;				// Angle For The Quad ( NEW )
float  m_rotate[3][5];

//The data exchanged between two threads.
USHORT*         ThreadFrameBits = new USHORT[640*480];  //raw data
SLR_ST_Skeleton ThreadSkeleton;
Mat             ThreadRGB;
vector<CvPoint3D32f>    LineTrack;


HANDLE hThread;
static DWORD WINAPI RecvProc(LPVOID lpParameter);
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

struct RECVPARAM
{
	bool kinectStart;
	int a;
};

bool emptyFrameTest(USHORT* data)
{
	bool flag = TRUE;
	for (int j=0; j<480*640; j+=1000)
	{
		if (*(data+j)<52685)
		{
			flag = FALSE;
			break;
		}
	}
	return flag;
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	return TRUE;										// Initialization Went OK
}

void DrawBackground()
{
	GLfloat dep=4.0f;
	GLfloat edg1=3.0f;
	GLfloat edg2=4.0f;
	GLfloat i=-dep;
	GLfloat j=-edg1;
	GLfloat k=-edg2;
	//if (demoStyle == 1)
	{
		glPushMatrix();
		glBegin(GL_LINES);
		glColor3f(1.0,1.0,1.0);
		while(i<=dep)
		{		// Top Face1
			glVertex3f(-edg2,  edg1,  i);
			glVertex3f( edg2,  edg1,  i);	
			// Bottom Face1
			glVertex3f(-edg2,  -edg1,  i);
			glVertex3f( edg2,  -edg1,  i);	
			//Left Face2
			glVertex3f(-edg2, -edg1,  i);
			glVertex3f(-edg2,  edg1,  i);
			// Right face2
			glVertex3f(edg2, -edg1,  i);
			glVertex3f(edg2,  edg1,  i);
			i+=0.4f;
		}

		while(j<=edg1)
		{		
			// Right face1
			glVertex3f( edg2,  j, -dep);
			glVertex3f( edg2,  j,  dep);
			//Left Face1
			glVertex3f( -edg2,  j, -dep);
			glVertex3f( -edg2,  j,  dep);
			// Back Face1
			glVertex3f(-edg2,  j, -dep);
			glVertex3f( edg2,  j, -dep);	
			j+=0.4f;
		}

		while (k<=edg2)
		{
			// Top Face2
			glVertex3f( k,  edg1,  dep);
			glVertex3f( k,  edg1, -dep);
			// Bottom Face2
			glVertex3f( k,  -edg1,  dep);
			glVertex3f( k,  -edg1, -dep);
			// Back Face2
			glVertex3f(k, -edg1, -dep);
			glVertex3f(k,  edg1, -dep);
			k+=0.4f;
		}

		glEnd();
		glPopMatrix();

	}
}

void DrawMiddleFinger(float x, float y, float z)
{
	float scale = 40;
	glTranslatef(x, y, z);//移动到指定位置

	glPushMatrix();
	glRotatef((float)m_rotate[0][2], 1.0, 0.0, 0.0);//控制旋转第一个关节

	glPushMatrix();

	glPushMatrix();
	glutSolidSphere(1.5/scale, 8, 8);//绘制中指的第一个关节，球A
	glPopMatrix();
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	glPushMatrix();
	GLUquadricObj *middFinger1;
	middFinger1=gluNewQuadric();
	gluQuadricDrawStyle(middFinger1,GLU_FILL);
	gluCylinder(middFinger1, 1.25/scale, 1.2/scale, 4/scale, 8, 8);//中指的第一个指节，圆柱1
	glPopMatrix();
	glPopMatrix();//第一个指节绘制完毕

	glTranslatef(0.0,4/scale,0.0);
	glRotatef((GLfloat)m_rotate[1][2], 1.0, 0.0, 0.0);//控制旋转第二个指节
	glTranslatef(0.0, -4/scale, 0.0);
	glPushMatrix();

	glPushMatrix();
	glTranslatef(0.0, 4.1/scale, 0.0);
	glutSolidSphere(1.25/scale, 8, 8);//绘制中指的第二个关节，球B
	glTranslatef(0.0, -4.1/scale, 0.0);
	glPopMatrix();

	glTranslatef(0.0, 4.2/scale, 0.0);
	glRotatef(-90, 1.0, 0.0, 0.0);
	glPushMatrix();
	GLUquadricObj *middFinger2;
	middFinger2=gluNewQuadric();
	gluQuadricDrawStyle(middFinger2, GLU_FILL);
	gluCylinder(middFinger2, 1.2/scale, 1.2/scale, 4/scale, 8, 8);//中指的第二个指节，圆柱2
	glPopMatrix(); 
	glTranslatef(0.0, -4.2/scale, 0.0);
	glPopMatrix();//第二节手指绘制完毕

	glTranslatef(0.0, 8.2/scale, 0.0); //绘制第三指节，即最后一个指节！
	glRotatef((GLfloat)m_rotate[2][2], 1.0, 0.0, 0.0);//控制旋转第三个指节
	glTranslatef(0.0, -8.2/scale, 0.0);
	glPushMatrix();

	glTranslatef(0.0, 8.0/scale, 0.0);	 
	glPushMatrix();
	glutSolidSphere(1.25/scale, 8, 8);//绘制中指的第三个关节，球C  
	glPopMatrix();   
	glTranslatef(0.0, -8.0/scale, 0.0); 

	glTranslatef(0.0, 8.1/scale, 0.0);
	glRotatef(-90, 1.0, 0.0, 0.0);	
	glPushMatrix();	  
	GLUquadricObj *middFinger3;
	middFinger3=gluNewQuadric();
	gluQuadricDrawStyle(middFinger3, GLU_FILL);
	gluCylinder(middFinger3, 1.2/scale, 1.2/scale, 4/scale, 8, 8);//中指的第三个指节，圆柱3
	glPopMatrix();
	glRotatef(90, 1.0, 0.0, 0.0);
	glTranslatef(0.0, -8.1/scale, 0.0);

	glTranslatef(0.0, 12.0/scale, 0.0);
	glPushMatrix();//绘制指尖
	glutSolidSphere(1.2/scale, 8, 4);//绘制指尖
	glPopMatrix();
	glTranslatef(0.0, -12.0/scale, 0.0);
	glPopMatrix();

	glPopMatrix();
	gluDeleteQuadric(middFinger1);//删除二次曲面对象
	gluDeleteQuadric(middFinger2);
	gluDeleteQuadric(middFinger3);
}

void DrawPalm(float x,float y, float z)
{
	float scale = 40;
	glTranslatef(x, y, z);//将手掌模型移动到指定位置
	  
	glPushMatrix();//矩形 的手掌模型案
	glTranslatef(-0.7/scale, 0.25/scale, 0.0);
	glScalef(6.0/scale, 7.0/scale, 2.5/scale);
	glutSolidCube(1.0);
	glTranslatef(0.6/scale, -0.25/scale, 0.0);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.1/scale, -2.95/scale, 0.0);
	glScalef(5.7/scale, 1.8/scale, 2.5/scale);
	glutSolidCube(1.0);
	glTranslatef(0.1/scale, 2.95/scale, 0.0);
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(0.7/scale, 0.0, 0.0);
	glScalef(6.0/scale, 5.0/scale, 2.5/scale);
	glutSolidCube(1.0);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.45/scale, -2.6/scale, 0.0);
	glRotatef(49.0, 0.0, 0.0, 1.0);	
	glScalef(1.75/scale, 1.75/scale, 2.5/scale);
	glutSolidCube(1.0);
	glTranslatef(-2.45/scale, 2.6/scale, 0.0);
	glPopMatrix();

	/*glPushMatrix();//方案二：八边形
	glTranslatef(-0.9, 1.9, 0.0);
	glScalef(6.5, 4.0, 2.4);
	glutSolidCube(1.0);
	glTranslatef(0.9, -1.9, 0.0);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.7, 1.75, 0.0);
	glScalef(6.0, 1.8, 2.4);
	glutSolidCube(1.0);
	glPopMatrix();
	glScalef(1.0, 1.0, 0.5);
	glPushMatrix();
	glRotatef(15.0, 0.0, 0.0, 1.0);
	glTranslatef(-0.3, 0.0, 0.0);
	glutSolidSphere(4.8, 8, 8);
	glPopMatrix();*/
}

void HandDisplay(float x,float y, float z)
{
	float scale = 40;

	glTranslatef(x, y, z);
	glRotatef(0,0,1,0);
	glPushMatrix ();
		glPushMatrix();//载入手掌
			glScalef(1.3, 1.3, 0.9);
			DrawPalm(1.0/scale, -4.0/scale, 0.0);
		glPopMatrix();

		glScalef(0.8, 0.75, 0.8);

		glPushMatrix();//载入中指
			DrawMiddleFinger(0.0, -0.3/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入无名指
			//DrawRingeFinger(3.2, -0.4, 0.0);
			DrawMiddleFinger(3.2/scale, -0.4/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入食指
			//DrawForefinger(-3.2, -0.7, 0.0);
			DrawMiddleFinger(-3.2/scale, -0.7/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入小拇指
			//DrawLittleFinger(6.3, -2.5, 0.0);
			DrawMiddleFinger(6.3/scale, -2.5/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入大拇指
			glScalef(1.0, 1.1, 1.0);
			//DrawThumb(-3.0, -11.0, 0.0);
			DrawMiddleFinger(-3.0/scale, -11.0/scale, 0.0);
		glPopMatrix();

	glPopMatrix ();

} 

int DrawGLScene(GLvoid)									// Here's Where We Do All The Drawing
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	DrawBackground();
	


	CvPoint3D32f temp;
	temp.x = ThreadSkeleton._3dPoint[11].x;
	temp.y = ThreadSkeleton._3dPoint[11].y;
	temp.z = ThreadSkeleton._3dPoint[11].z;
	LineTrack.push_back(temp);



	float xScale = 2.5;
	float yScale = 2.5;
	float zScale = 3;
	glLoadIdentity();									// Reset The Current Modelview Matrix
	glTranslatef(0.0f,0.0f,-7.0f);	
	HandDisplay(xScale*ThreadSkeleton._3dPoint[11].x, yScale*ThreadSkeleton._3dPoint[11].y,
		zScale*ThreadSkeleton._3dPoint[11].z);
// 	glBegin(GL_QUADS);									// Draw A Quad
// 	glColor3f(0.0f,1.0f,0.0f);						// Set The Color To Green
// 	glVertex3f(xScale*ThreadSkeleton._3dPoint[11].x, yScale*ThreadSkeleton._3dPoint[11].y,
// 		zScale*ThreadSkeleton._3dPoint[11].z);					// Top Right Of The Quad (Top)
// 	glVertex3f(xScale*(ThreadSkeleton._3dPoint[11].x-0.1f), yScale*ThreadSkeleton._3dPoint[11].y,
// 		zScale*ThreadSkeleton._3dPoint[11].z);					// Top Left Of The Quad (Top)
// 	glVertex3f(xScale*(ThreadSkeleton._3dPoint[11].x-0.1f), yScale*(ThreadSkeleton._3dPoint[11].y-0.1f),
// 		zScale*ThreadSkeleton._3dPoint[11].z);					// Bottom Left Of The Quad (Top)
// 	glVertex3f(xScale*ThreadSkeleton._3dPoint[11].x, yScale*(ThreadSkeleton._3dPoint[11].y-0.1f),
// 		zScale*ThreadSkeleton._3dPoint[11].z);
// 	glEnd();

	glLoadIdentity();									// Reset The Current Modelview Matrix
	glTranslatef(0.0f,0.0f,-7.0f);	
	glPointSize(5.0f);
	glBegin(GL_POINTS);									// Draw A Quad
	glColor3f(1.0f,0.0f,0.0f);
	for (int i=0; i<LineTrack.size(); i++)
	{
		glVertex3f(xScale*LineTrack[i].x, yScale*LineTrack[i].y, zScale*LineTrack[i].z);
	}
	glEnd();

	glLoadIdentity();									// Reset The Current Modelview Matrix
	glTranslatef(0.0f,0.0f,-7.0f);	
	glBegin(GL_LINES);									// Draw A Quad
	glColor3f(1.0f,0.0f,0.0f);
	for (int i=0; i<LineTrack.size()-1; i++)
	{
		glVertex3f(xScale*LineTrack[i].x, yScale*LineTrack[i].y, zScale*LineTrack[i].z);
		glVertex3f(xScale*LineTrack[i+1].x, yScale*LineTrack[i+1].y, zScale*LineTrack[i+1].z);
	}
	glEnd();

// 	rtri+=0.6f;											// Increase The Rotation Variable For The Triangle ( NEW )
// 	rquad-=0.15f;										// Decrease The Rotation Variable For The Quad ( NEW )
	return TRUE;	
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,L"Release Of DC And RC Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,L"Release Rendering Context Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,L"Release Device Context Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,L"Could Not Release hWnd.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass(L"OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,L"Could Not Unregister Class.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= L"OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,L"Failed To Register The Window Class.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,L"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?",L"NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,L"Program Will Now Close.",L"ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								L"OpenGL",							// Class Name
								L"3D Drawing",								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,L"Window Creation Error.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Create A GL Device Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Find A Suitable PixelFormat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Set The PixelFormat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Create A GL Rendering Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Activate The GL Rendering Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,L"Initialization Failed.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
					return 0;
			}
			break;
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

	USHORT*   m_pFrameBits;
	int i,j;
	int height = 480;
	int width = 640;
	int maxDepth;
	IplImage* depthImage = cvCreateImage(cvSize(640,480),8,1);
	cvNamedWindow("Capturing",1);
	m_pFrameBits = new USHORT[640*480];
	RECVPARAM *pRecvParam;
	pRecvParam = new RECVPARAM;
	//pRecvParam->kinectStart = m_bStartKinect;

	hThread = CreateThread(NULL, 0, RecvProc, (LPVOID)pRecvParam, 0, NULL);
	CloseHandle(hThread);

	for (int i=0; i<3; i++)
	{
		for (int j=0; j<5; j++)
		{
			m_rotate[i][j] = 40;
		}
	}



	// Ask The User Which Screen Mode They Prefer
// 	if (MessageBox(NULL,L"Would You Like To Run In Fullscreen Mode?", L"Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
// 	{
		fullscreen=FALSE;							// Windowed Mode
	//}

	// Create Our OpenGL Window
	if (!CreateGLWindow("NeHe's First Polygon Tutorial",640,480,16,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			//////////////////////////////////////////////////////////////////////////
			maxDepth = 0;
			if(!emptyFrameTest(ThreadFrameBits))
			{
				for (j=0; j<480*640; j++)
				{
					*(m_pFrameBits+j)=*(ThreadFrameBits+j);
				}

			}
			for (j=0; j<height; j++)
			{
				for (i=0; i<width; i++)
				{
					if((*(m_pFrameBits+width*j+i))>maxDepth)
					{
						maxDepth = *(m_pFrameBits+width*j+i);
					}
				}
			}

			for (j=0; j<height; j++)
			{
				uchar* src_ptr_back = (uchar*)(depthImage->imageData + j*depthImage->widthStep);
				for (i=0; i<width; i++)
				{
					src_ptr_back[i] = (*(m_pFrameBits+width*j+i))*255/(maxDepth+1);
				}
			}
			//cvSaveImage("dfd.jpg",depthImage);
			cvShowImage("Capturing",depthImage);

			//////////////////////////////////////////////////////////////////////////

			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if ((active && !DrawGLScene()) || keys[VK_ESCAPE])	// Active?  Was There A Quit Received?
			{
				done=TRUE;							// ESC or DrawGLScene Signalled A Quit
			}
			else									// Not Time To Quit, Update Screen
			{
				SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
			}

			if (keys[VK_F1])						// Is F1 Being Pressed?
			{
				keys[VK_F1]=FALSE;					// If So Make Key FALSE
				KillGLWindow();						// Kill Our Current Window
				fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
				// Recreate Our OpenGL Window
				if (!CreateGLWindow("NeHe's First Polygon Tutorial",640,480,16,fullscreen))
				{
					return 0;						// Quit If Window Was Not Created
				}
			}
		}
	}


	// Shutdown
	KillGLWindow();									// Kill The Window
	return (msg.wParam);							// Exit The Program
}

DWORD WINAPI RecvProc(LPVOID lpParameter)
{
	m_pKinect.init(0,0,480,640);
	while (WaitForSingleObject(m_pKinect.hNextFrame, INFINITE) == WAIT_OBJECT_0)
	{
		m_pKinect.queryFrame();
		//float ahand = m_pKinect.mSkeleton._3dPoint[7].x;

		int count = 0;
		for (int i=0; i<480; i++)
		{
			for (int j=0; j<640; j++)
			{
				*(ThreadFrameBits+count) = (m_pKinect.mDepth).at<USHORT>(i,j);
				count++;
			}
		}

		ThreadSkeleton = m_pKinect.mSkeleton;
		
// 		m_pKinect.mDepth;
// 		m_pKinect.mRgb;
	}
	
	//m_pKinect.singleCaptureThread();
	return 0;
}



// DWORD WINAPI RecvProc(LPVOID lpParameter)
// {
// 
// 	//For Kinect on-line sampling 
// 	INuiSensor*             m_pNuiSensor;
// 	HANDLE                  m_pDepthStreamHandle;
// 	HANDLE                  m_pColorStreamHandle;
// 	HANDLE                  m_hNextDepthFrameEvent;
// 	HANDLE                  m_hNextColorFrameEvent;
// 	INuiFrameTexture*       m_pTexture;
// 	NUI_LOCKED_RECT         m_LockedRect;
// 	BYTE*                   m_depthRGBX;
// 	static const int        m_cDepthWidth = 640;
// 	static const int        m_cDepthHeight = 480;
// 	static const int        m_cBytesPerPixel = 4;
// 	//////////////////////////////////////////////////////////////////////////
// 	INuiSensor * pNuiSensor;
// 	HRESULT hr;
// 
// 	int iSensorCount = 0;
// 	hr = NuiGetSensorCount(&iSensorCount);
// 	if (FAILED(hr))
// 	{
// 		return hr;
// 	}
// 	// Look at each Kinect sensor
// 	for (int i = 0; i < iSensorCount; ++i)
// 	{
// 		// Create the sensor so we can check status, if we can't create it, move on to the next
// 		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
// 		if (FAILED(hr))
// 		{
// 			continue;
// 		}
// 
// 		// Get the status of the sensor, and if connected, then we can initialize it
// 		hr = pNuiSensor->NuiStatus();
// 		if (S_OK == hr)
// 		{
// 			m_pNuiSensor = pNuiSensor;
// 			break;
// 		}
// 
// 		// This sensor wasn't OK, so release it since we're not using it
// 		pNuiSensor->Release();
// 	}
// 
// 	if (NULL != m_pNuiSensor)
// 	{
// 		// Initialize the Kinect and specify that we'll be using depth
// 		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH); 
// 		if (SUCCEEDED(hr))
// 		{
// 			// Create an event that will be signaled when depth data is available
// 			m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
// 
// 			// Open a depth image stream to receive depth frames
// 			hr = m_pNuiSensor->NuiImageStreamOpen(
// 				NUI_IMAGE_TYPE_DEPTH,
// 				NUI_IMAGE_RESOLUTION_640x480,
// 				0,
// 				2,
// 				m_hNextDepthFrameEvent,
// 				&m_pDepthStreamHandle);
// 		}
// 	}
// 
// 	if (NULL == m_pNuiSensor || FAILED(hr))
// 	{
// 		//SetStatusMessage(L"No ready Kinect found!");
// 		return E_FAIL;
// 	}
// 
// 	m_depthRGBX       = new BYTE[m_cDepthWidth*m_cDepthHeight*m_cBytesPerPixel];
// 
// 	//////////////////////////////////////////////////////////////////////////
// 	//update
// 	while (1)
// 	{
// 		if (NULL == m_pNuiSensor)
// 		{
// 			continue;
// 		}
// 
// 		if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextDepthFrameEvent, 0) )
// 		{
// 			//////////////////////////////////////////////////////////////////////////
// 
// 			NUI_IMAGE_FRAME imageFrame;
// 			hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);
// 			if (FAILED(hr))
// 			{
// 				return false;
// 			}
// 			m_pTexture = imageFrame.pFrameTexture;
// 			// Lock the frame data so the Kinect knows not to modify it while we're reading it
// 			m_pTexture->LockRect(0, &m_LockedRect, NULL, 0);
// 
// 			// Make sure we've received valid data
// 			if (m_LockedRect.Pitch != 0)
// 			{
// 				BYTE * rgbrun = m_depthRGBX;
// 				const USHORT * pBufferRun = (const USHORT *)m_LockedRect.pBits;
// 				int count = 0;
// 				//read data from the kinect sensor when needed
// 				while ( count < m_cDepthWidth * m_cDepthHeight)
// 				{
// 					// discard the portion of the depth that contains only the player index
// 					USHORT depth = NuiDepthPixelToDepth(*pBufferRun);
// 					*(ThreadFrameBits+count) = depth;
// 					count++;
// 					++pBufferRun;
// 				}
// 			}
// 			// We're done with the texture so unlock it
// 			m_pTexture->UnlockRect(0);
// 
// 			// Release the frame
// 			m_pNuiSensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);
// 		}
// 	}
// 
// 
// 	// CTitaniumRemoteClientApp dfd;
// 	// dfd.InitInstance();
// 
// 	//KinectClientConnectThread m_clientConnectThread;
// 	/*	CTitaniumRemoteClientApp theApp;*/
// 	return 0;
// }
