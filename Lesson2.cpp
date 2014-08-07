#include <windows.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include <gl\glut.h>
#include "NuiApi.h"
#include "opencv2/opencv.hpp"
#include "msKinect.h"
#include "F.h"

MsKinect m_pKinect;
#define HandSize 64
vector<double> HOG_palm_right;
vector<double> HOG_palm_left;
vector<double> HOG_fist_right;
vector<double> HOG_fist_left;
int statesIndicator[2];   //channel: 0: left. 1: right      
                        //value: 1: palm. 0: fist. -1: other.
vector<int> hiddenState[2];
int duration = 120;



HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default

// GLfloat	rtri;				// Angle For The Triangle ( NEW )
// GLfloat	rquad;				// Angle For The Quad ( NEW )
float  m_rotate[3][5];         //Control the shape of the hand.
float  m_rotate_left[3][5];         //Control the shape of the hand.
GLfloat Xrotate;
GLfloat Yrotate;
GLfloat Zrotate;

//The data exchanged between two threads.
USHORT*         ThreadFrameBits = new USHORT[640*480];  //raw data
SLR_ST_Skeleton ThreadSkeleton;
//Mat             ThreadRGB;
IplImage*       ThreadRGBImage = cvCreateImage(cvSize(640,480),8,3);
vector<CvPoint3D32f>    LineTrack;


HANDLE hThread;
static DWORD WINAPI RecvProc(LPVOID lpParameter);
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc
#define MAX_CHAR 128

struct RECVPARAM
{
	bool kinectStart;
	int a;
};

void drawString(const char* str) {
	static int isFirstCall = 1;
	static GLuint lists;

	if( isFirstCall ) { // 如果是第一次调用，执行初始化
		// 为每一个ASCII字符产生一个显示列表
		isFirstCall = 0;

		// 申请MAX_CHAR个连续的显示列表编号
		lists = glGenLists(MAX_CHAR);

		// 把每个字符的绘制命令都装到对应的显示列表中
		wglUseFontBitmaps(wglGetCurrentDC(), 0, MAX_CHAR, lists);
	}
	// 调用每个字符对应的显示列表，绘制每个字符
	for(; *str!='\0'; ++str)
		glCallList(lists + *str);
}

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
	GLfloat dep=5.0f;
	GLfloat edg1=3.0f;
	GLfloat edg2=4.0f;
	GLfloat i=-dep;
	GLfloat j=-edg1;
	GLfloat k=-edg2;
	//if (demoStyle == 1)
	{
		glPushMatrix();
		glLineWidth(1);
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
			i+=0.6f;
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
			j+=0.6f;
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
			k+=0.6f;
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

void DrawMiddleFinger_left(float x, float y, float z)
{
	float scale = 40;
	glTranslatef(x, y, z);//移动到指定位置

	glPushMatrix();
	glRotatef((float)m_rotate_left[0][2], 1.0, 0.0, 0.0);//控制旋转第一个关节

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
	glRotatef((GLfloat)m_rotate_left[1][2], 1.0, 0.0, 0.0);//控制旋转第二个指节
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
	glRotatef((GLfloat)m_rotate_left[2][2], 1.0, 0.0, 0.0);//控制旋转第三个指节
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

void DrawPalm_left(float x,float y, float z)
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

void HandDisplay_left(float x,float y, float z)
{
	float scale = 40;

	glTranslatef(x, y, z);
	glRotatef(0,0,1,0);
	glPushMatrix ();
		glPushMatrix();//载入手掌
			glScalef(1.3, 1.3, 0.9);
			DrawPalm_left(1.0/scale, -4.0/scale, 0.0);
		glPopMatrix();

		glScalef(0.8, 0.75, 0.8);

		glPushMatrix();//载入中指
			DrawMiddleFinger_left(0.0, -0.3/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入无名指
			//DrawRingeFinger(3.2, -0.4, 0.0);
			DrawMiddleFinger_left(3.2/scale, -0.4/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入食指
			//DrawForefinger(-3.2, -0.7, 0.0);
			DrawMiddleFinger_left(-3.2/scale, -0.7/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入小拇指
			//DrawLittleFinger(6.3, -2.5, 0.0);
			DrawMiddleFinger_left(6.3/scale, -2.5/scale, 0.0);
		glPopMatrix();

		glPushMatrix();//载入大拇指
			glScalef(1.0, 1.1, 1.0);
			//DrawThumb(-3.0, -11.0, 0.0);
			DrawMiddleFinger_left(-3.0/scale, -11.0/scale, 0.0);
		glPopMatrix();

	glPopMatrix ();

} 

void GetRotateCenter(GLfloat* x, GLfloat* y, GLfloat* z)
{
	*x = 0.0f; 
	*y = 0.0f;
	*z = 0.0f;
	for (int i=0; i<LineTrack.size(); i++)
	{
		*x += LineTrack[i].x;
		*y += LineTrack[i].y;
		*z += LineTrack[i].z;
	}

	*x /= (LineTrack.size() + 0.00001);
	*y /= (LineTrack.size() + 0.00001);
	*z /= (LineTrack.size() + 0.00001);

}

int DrawGLScene(GLvoid)									
{
	GLfloat center_x, center_y, center_z;
	center_x = 0.0f;
	center_y = 0.0f;
	center_z = 0.0f;

	float xScale = 2.5;
	float yScale = 2.5;
	float zScale = 3;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	
		//Draw background.
	glLoadIdentity();
	glTranslatef(0.0f,0.0f,-7.0f);
	DrawBackground();


	if (statesIndicator[0] == -1 && statesIndicator[1] == -1 && !LineTrack.empty())
	{
		if (ThreadSkeleton._3dPoint[11].x < ThreadSkeleton._3dPoint[7].x)
		{
			LineTrack.clear();    //This function is really simple and cool. Clear the painting.
			center_x = 0.0f;
			center_y = 0.0f;
			center_z = 0.0f;
			Yrotate = 0.0f;
		}
		Yrotate += 0.6f;
	}


	CvPoint3D32f temp;
	temp.x = ThreadSkeleton._3dPoint[11].x;
	temp.y = ThreadSkeleton._3dPoint[11].y;
	temp.z = ThreadSkeleton._3dPoint[11].z;
	if (statesIndicator[0] == 1 && statesIndicator[1] != -1)    //暂时用左手控制画笔开关，这样比较稳定。
	{
		LineTrack.push_back(temp);
	}
	
		//Get the center of the drawing.
	GetRotateCenter(&center_x, &center_y, &center_z);
	glLoadIdentity();									
	glTranslatef(0.0f,0.0f,-3.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
	glRasterPos2f(-1.0f, -1.0f);
	char tempbuf[45];
	sprintf(tempbuf, "%f, %f, %f", xScale*center_x, yScale*center_y, zScale*center_z);
	drawString(tempbuf);

		//Display the hand
	glLoadIdentity();									
	glTranslatef(0.0f,0.0f,-7.0f);	
	glColor3f(1.0f, 1.0f, 0.0f);
	HandDisplay(xScale*ThreadSkeleton._3dPoint[11].x, yScale*ThreadSkeleton._3dPoint[11].y,
		zScale*ThreadSkeleton._3dPoint[11].z);

	glLoadIdentity();									
	glTranslatef(0.0f,0.0f,-7.0f);	
	glColor3f(1.0f, 1.0f, 0.0f);
	HandDisplay_left(xScale*ThreadSkeleton._3dPoint[7].x, yScale*ThreadSkeleton._3dPoint[7].y,
		zScale*ThreadSkeleton._3dPoint[7].z);

		//Draw the painting.
	if (LineTrack.size()>0)
	{
			//Draw the original point.
		glLoadIdentity();									
		glTranslatef(0.0f,0.0f,-7.0f);
		glPointSize(25.0f);
		glBegin(GL_POINTS);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glEnd();

		glLoadIdentity();								
		glTranslatef(0.0f,0.0f,-7.0f);
		if (statesIndicator[0] == -1 && statesIndicator[1] == -1)
		{
			glTranslatef(xScale*center_x,yScale*center_y,zScale*center_z);
			glPointSize(25.0f);
			glColor3f(0.0f,0.0f,1.0f);
			glBegin(GL_POINTS);
			glVertex3f(xScale*center_x,yScale*center_y,zScale*center_z);
			glEnd();
			glRotatef(Yrotate,0,1,0);
		}
		else
		{
			center_x = 0.0f;
			center_y = 0.0f;
			center_z = 0.0f;
		}
		glPointSize(5.0f);
		glBegin(GL_POINTS);								
		glColor3f(1.0f,0.0f,0.0f);
		for (int i=0; i<LineTrack.size(); i++)
		{
			glVertex3f(xScale*LineTrack[i].x-xScale*center_x, 
				yScale*LineTrack[i].y-yScale*center_y, 
				zScale*LineTrack[i].z-zScale*center_z);
		}
		glEnd();

		glLineWidth(8);
		glBegin(GL_LINES);									
		glColor3f(1.0f,0.0f,0.0f);
		for (int i=0; i<LineTrack.size()-1; i++)
		{
			glVertex3f(xScale*LineTrack[i].x-xScale*center_x, 
				yScale*LineTrack[i].y-yScale*center_y, 
				zScale*LineTrack[i].z-zScale*center_z);
			glVertex3f(xScale*LineTrack[i+1].x-xScale*center_x, 
				yScale*LineTrack[i+1].y-yScale*center_y, 
				zScale*LineTrack[i+1].z-zScale*center_z);
		}
		glEnd();
	}

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

IplImage *Resize(IplImage *_img)
{
	IplImage *_dst=cvCreateImage(cvSize(HandSize,HandSize),_img->depth,_img->nChannels);
	cvResize(_img,_dst);
	return _dst;
}

bool GetHOGHistogram_Patch(IplImage *img,vector<double> &hog_hist)
{
	HOGDescriptor *hog=new HOGDescriptor(cvSize(HandSize,HandSize),cvSize(16,16),cvSize(8,8),cvSize(8,8),9);
	//HOGDescriptor *hog=new HOGDescriptor(cvSize(SIZE,SIZE),cvSize(32,32),cvSize(16,16),cvSize(16,16),9);
	//(cvSize(SIZE,SIZE),cvSize(16,16),cvSize(8,8),cvSize(8,8),9)
	/////////////////////window: 64*64，block: 8*8，block step:4*4，cell: 4*4
	cvNormalize(img,img,255,0,CV_MINMAX,0); //Add by Hanjie Wang. 2013-03.
	//LBP(img,img);
	Mat handMat(img);

	vector<float> *descriptors = new std::vector<float>();

	hog->compute(handMat, *descriptors,Size(0,0), Size(0,0));
	////////////////////window: 0*0
	double total=0;
	int i;
	for(i=0;i<descriptors->size();i++)
		total+=abs((*descriptors)[i]);
	//	total=sqrt(total);
	for(i=0;i<descriptors->size();i++)
		hog_hist.push_back((*descriptors)[i]/total);
	return true; 
}

double Histogram(vector<double>vec1,vector<double>vec2)
{
	double mat_score=0.0;//mat_score: similarity
	int i;
	int _Size=vec1.size();
	for(i=0;i<_Size;i++)
	{
		mat_score+=vec1[i]<vec2[i] ? vec1[i] : vec2[i];
	}
	return  mat_score;
}

void HandPostureInitial()
{
	IplImage* left_palm = cvLoadImage("left_palm.jpg", 0);
	IplImage* left_fist = cvLoadImage("left_fist.jpg", 0);
	IplImage* right_palm = cvLoadImage("right_plam.jpg", 0);
	IplImage* right_fist = cvLoadImage("right_fist.jpg", 0);

	left_palm = Resize(left_palm);
	left_fist = Resize(left_fist);
	right_palm = Resize(right_palm);
	right_fist = Resize(right_fist);

	GetHOGHistogram_Patch(left_palm, HOG_palm_left);
	GetHOGHistogram_Patch(left_fist, HOG_fist_left);
	GetHOGHistogram_Patch(right_palm, HOG_palm_right);
	GetHOGHistogram_Patch(right_fist, HOG_fist_right);

	for (int i=0; i<2; i++)
	{
		statesIndicator[i] = -1; 
		//changeDur[i] = 0;
	}
}

void HandPostureRecognition(IplImage* depthImage, IplImage* rgbImage, SLR_ST_Skeleton skeleton,
	int HandPostates[])
{
		//Background segmentation
	float aveDepthHead = 0;
	CvPoint headPosition;
	headPosition.x = skeleton._2dPoint[3].x>10?skeleton._2dPoint[3].x:10;
	headPosition.y = skeleton._2dPoint[3].y>10?skeleton._2dPoint[3].y:10;
	cvDrawCircle(depthImage,headPosition,20,cvScalar(255,0,0),2,8,0);
	for (int i=headPosition.x-10; i<headPosition.x+10; i++)
	{
		uchar* src_ptr_back = (uchar*)(depthImage->imageData + i*depthImage->widthStep);
		for (int j=headPosition.y-10; j<headPosition.y+10; j++)
		{
			aveDepthHead += (int)src_ptr_back[j];
		}
	}
	aveDepthHead = (int)(aveDepthHead/(20*20));

	for (int i=0; i<480; i++)
	{
		uchar* src_ptr_back = (uchar*)(depthImage->imageData + i*depthImage->widthStep);
		for (int j=0; j<640; j++)
		{
			if (src_ptr_back[j]< aveDepthHead+5 || src_ptr_back[j] >= 250)
			{
				src_ptr_back[j] = 0;
			}
		}
	}

	CvFont font;
	double hscale = 1.0;
	double vscale = 1.0;
	int linewidth = 2;
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC,hscale,vscale,0,linewidth);
	char tesxt[20];
	itoa(aveDepthHead,tesxt,20);
	cvPutText(depthImage, tesxt, headPosition, &font,cvScalar(255,0,0));


		//Draw a line.
	int lineHeight = 350;
	CvPoint LineP1;
	LineP1.x = 0;
	LineP1.y = lineHeight;
	CvPoint LineP2;
	LineP2.x = 640;
	LineP2.y = lineHeight;
	cvLine(depthImage, LineP1, LineP2, cvScalar(255,255,255),2,8,0);


	int handHeight = 35;
	int handWidth = 30;
		//Right hand bounding box
	CvPoint p1;
	p1.x = skeleton._2dPoint[11].x-handWidth;
	p1.y = skeleton._2dPoint[11].y-handHeight;
	CvPoint p2;
	p2.x = skeleton._2dPoint[11].x+handWidth;
	p2.y = skeleton._2dPoint[11].y+handHeight;
	cvRectangle(depthImage,p1,p2,cvScalar(255,255,255),2,8,0);
	CvRect RightHandRoi;
	RightHandRoi.x = p1.x;
	RightHandRoi.y = p1.y;
	RightHandRoi.width = 2*handWidth;
	RightHandRoi.height = 2*handHeight;
	cvSetImageROI(depthImage,RightHandRoi);
	IplImage* rightHandImage = cvCreateImage(cvSize(HandSize,HandSize),8,1);
	cvResize(depthImage,rightHandImage,1);
	//depthImage = Resize(depthImage);
	vector<double> HOG_rightHand;
	GetHOGHistogram_Patch(rightHandImage, HOG_rightHand);
	if (skeleton._2dPoint[11].y > lineHeight)
	{
		hiddenState[1].push_back(-1);
		//statesIndicator[1] = -1;
	}
	else
	{
		double score_palm = Histogram(HOG_rightHand, HOG_palm_right);
		double score_fist = Histogram(HOG_rightHand, HOG_fist_right);
		if (score_fist > score_palm)
		{
			hiddenState[1].push_back(0);
			//statesIndicator[1] = 0;
		}
		else
		{
			hiddenState[1].push_back(1);
			//statesIndicator[1] = 1;
		}

	}
	//cvSaveImage("right.jpg",rightHandImage);
	cvResetImageROI(depthImage);
	if (hiddenState[1].size()>duration)
	{
		vector<int>::iterator vi=hiddenState[1].begin();
		hiddenState[1].erase(vi);
	}
	int times_right_none = 0;
	int times_right_palm = 0;
	int times_right_fist = 0;
	for (int i=0; i<hiddenState[1].size(); i++)
	{
		if (hiddenState[1][i] == -1) times_right_none++;
		if (hiddenState[1][i] == 0) times_right_fist++;
		if (hiddenState[1][i] == 1) times_right_palm++;
	}
	if (times_right_none>times_right_fist && times_right_none>times_right_palm) statesIndicator[1] = -1;
	if (times_right_fist>times_right_none && times_right_fist>times_right_palm) statesIndicator[1] = 0;
	if (times_right_palm>times_right_fist && times_right_palm>times_right_none) statesIndicator[1] = 1;
	if (statesIndicator[1] == -1)
	{
		cvPutText(depthImage, "None", cvPoint(400, lineHeight), &font,cvScalar(255,255,255));
	}
	else if (statesIndicator[1] == 0)
	{
		cvPutText(depthImage, "Fist", cvPoint(400, lineHeight), &font,cvScalar(255,255,255));
	}
	else if (statesIndicator[1] == 1)
	{
		cvPutText(depthImage, "Palm", cvPoint(400, lineHeight), &font,cvScalar(255,255,255));
	}


		//Left hand bounding box
	CvPoint p3;
	p3.x = skeleton._2dPoint[7].x-handWidth;
	p3.y = skeleton._2dPoint[7].y-handHeight;
	CvPoint p4;
	p4.x = skeleton._2dPoint[7].x+handWidth;
	p4.y = skeleton._2dPoint[7].y+handHeight;
	cvRectangle(depthImage,p3,p4,cvScalar(255,255,255),2,8,0);
	CvRect LeftHandRoi;
	LeftHandRoi.x = p3.x;
	LeftHandRoi.y = p3.y;
	LeftHandRoi.width = 2*handWidth;
	LeftHandRoi.height = 2*handHeight;
	cvSetImageROI(depthImage,LeftHandRoi);
	//depthImage = Resize(depthImage);
	IplImage* leftHandImage = cvCreateImage(cvSize(HandSize,HandSize),8,1);
	cvResize(depthImage,leftHandImage,1);
	//cvSaveImage("left.jpg",leftHandImage);
	vector<double> HOG_leftHand;
	GetHOGHistogram_Patch(leftHandImage, HOG_leftHand);
	if (skeleton._2dPoint[7].y > lineHeight)
	{
		hiddenState[0].push_back(-1);
		//statesIndicator[0] = -1;
	}
	else
	{
		double score_palm = Histogram(HOG_leftHand, HOG_palm_left);
		double score_fist = Histogram(HOG_leftHand, HOG_fist_left);
		if (score_fist > score_palm)
		{
			hiddenState[0].push_back(0);
			//statesIndicator[0] = 0;
		}
		else
		{
			hiddenState[0].push_back(1);
			//statesIndicator[0] = 1;
		}

	}
	cvResetImageROI(depthImage);
	if (hiddenState[0].size()>duration)
	{
		vector<int>::iterator vi=hiddenState[0].begin();
		hiddenState[0].erase(vi);
	}
	int times_left_none = 0;
	int times_left_palm = 0;
	int times_left_fist = 0;
	for (int i=0; i<hiddenState[0].size(); i++)
	{
		if (hiddenState[0][i] == -1) times_left_none++;
		if (hiddenState[0][i] == 0) times_left_fist++;
		if (hiddenState[0][i] == 1) times_left_palm++;
	}
	if (times_left_none>times_left_fist && times_left_none>times_left_palm) statesIndicator[0] = -1;
	if (times_left_fist>times_left_none && times_left_fist>times_left_palm) statesIndicator[0] = 0;
	if (times_left_palm>times_left_fist && times_left_palm>times_left_none) statesIndicator[0] = 1;
	if (statesIndicator[0] == -1)
	{
		cvPutText(depthImage, "None", cvPoint(100, lineHeight), &font,cvScalar(255,255,255));
	}
	else if (statesIndicator[0] == 0)
	{
		cvPutText(depthImage, "Fist", cvPoint(100, lineHeight), &font,cvScalar(255,255,255));
	}
	else if (statesIndicator[0] == 1)
	{
		cvPutText(depthImage, "Palm", cvPoint(100, lineHeight), &font,cvScalar(255,255,255));
	}


	cvReleaseImage(&leftHandImage);
	cvReleaseImage(&rightHandImage);

}

void handshapeChange()
{
	if (statesIndicator[1] == 0)
	{
		for (int i=0; i<3; i++)
		{
			for (int j=0; j<5; j++)
			{
				m_rotate[i][j] = 30;
			}
		}
	}
	else if (statesIndicator[1] == 1)
	{
		for (int i=0; i<3; i++)
		{
			for (int j=0; j<5; j++)
			{
				m_rotate[i][j] = 0;
			}
		}
	}


	if (statesIndicator[0] == 0)
	{
		for (int i=0; i<3; i++)
		{
			for (int j=0; j<5; j++)
			{
				m_rotate_left[i][j] = 30;
			}
		}
	}
	else if (statesIndicator[0] == 1)
	{
		for (int i=0; i<3; i++)
		{
			for (int j=0; j<5; j++)
			{
				m_rotate_left[i][j] = 0;
			}
		}
	}
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
	HandPostureInitial();
	hThread = CreateThread(NULL, 0, RecvProc, (LPVOID)pRecvParam, 0, NULL);
	CloseHandle(hThread);

	for (int i=0; i<3; i++)
	{
		for (int j=0; j<5; j++)
		{
			m_rotate[i][j] = 40;
			m_rotate_left[i][j] = 40;
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
				//Check if it is an empty frame.
			if(!emptyFrameTest(ThreadFrameBits))
			{
				for (j=0; j<480*640; j++)
				{
					*(m_pFrameBits+j)=*(ThreadFrameBits+j);
				}

			}
				//Obtain the maximun depth value.
			maxDepth = 0;
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
				//Normalize the depth for showing.
			for (j=0; j<height; j++)
			{
				uchar* src_ptr_back = (uchar*)(depthImage->imageData + j*depthImage->widthStep);
				for (i=0; i<width; i++)
				{
					src_ptr_back[i] = 255 - (*(m_pFrameBits+width*j+i))*255/(maxDepth+1);
				}
			}

			int HandPoStates[4];
			for (int i=0; i<4; i++)
				HandPoStates[i] = 0;
			HandPostureRecognition(depthImage, ThreadRGBImage, ThreadSkeleton, HandPoStates);
				//Show depth or rgb images.
			cvShowImage("Capturing",depthImage);
			//cvShowImage("Capturing",ThreadRGBImage);



			//////////////////////////////////////////////////////////////////////////
			//Change the shape of hand in openGL
			handshapeChange();
			
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

			//Depth image
		int count = 0;
		for (int i=0; i<480; i++)
		{
			for (int j=0; j<640; j++)
			{
				*(ThreadFrameBits+count) = (m_pKinect.mDepth).at<USHORT>(i,j);
				count++;
			}
		}

			//Skeleton image
		ThreadSkeleton = m_pKinect.mSkeleton;

			//RGB image
		if ((m_pKinect.mRgb).data != NULL)
		{
			for (int i=0; i<480; i++)
			{
				uchar* src_rgbimage = (uchar*)(ThreadRGBImage->imageData + i*ThreadRGBImage->widthStep);
				for (int j=0; j<640; j++)
				{
					src_rgbimage[3*j+0] = (m_pKinect.mRgb).at<uchar>(i,3*j+0);
					src_rgbimage[3*j+1] = (m_pKinect.mRgb).at<uchar>(i,3*j+1);
					src_rgbimage[3*j+2] = (m_pKinect.mRgb).at<uchar>(i,3*j+2);
				}
			}
		}
		
	}
	
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
