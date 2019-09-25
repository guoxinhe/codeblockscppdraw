/**
 * ruds branch
 * Thread 1: Render data to buffer
 * Thread 2: User control, mostly the major thread
 * Thread 3: Display buffer to device
 * Thread 4: Service
 */

#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <math.h>//for sin(float);
#include "plugin.h"

#define TESTPLUGIN pluginm3d
#define exInc(pLong) InterlockedIncrement((volatile LONG *)pLong)            //++
#define exDec(pLong) InterlockedDecrement((volatile LONG *)pLong)            //--
#define exAdd(pLong,val) InterlockedExchangeAdd((volatile LONG *)pLong, val) //+=
#define exSet(pLong,val) InterlockedExchange((volatile LONG *)pLong, val)    //=
#define exMod(pLong,to,out) InterlockedCompareExchange((volatile LONG *)pLong, to, out)

#define PI      (3.14159)
#define TWOPI   (2*3.14159)
#define TWO_PI  TWOPI

//thread used external callback function
int RenderBuffer(DWORD bufferIndex);
void DisplayBuffer(DWORD bufferIndex);

#define WIN32_LEAN_AND_MEAN
int mainThreadLive=1;
DWORD displayBusy=0;
DWORD displayedCount=0;

DWORD readerBusy=0;
DWORD readerBusyOn=0;
DWORD readeredCount=0;
DWORD renderIdentifier=0, renderIdwp, renderIdlp;
const char *displayBufferName[]={"[    -- ]","[ --    ]"};

DWORD WINAPI ThreadFuncRender(LPVOID lpParam) {
    int meetDisplayBusy=0;
    while(mainThreadLive != 0) {
        //TODO: check if need render
        exSet(&readerBusy,1);

        //check last display busy flag if need switch render buffer
        if(meetDisplayBusy && displayBusy==0) {
            exSet(&readerBusyOn,1-readerBusyOn);
            meetDisplayBusy=0;
        }

        //render something if user is dirty
        if(RenderBuffer(readerBusyOn)!=0) {
            //after render, try to switch render buffer
            if(displayBusy==0)
                exSet(&readerBusyOn,1-readerBusyOn);
            else
                meetDisplayBusy=1;
        }
        exSet(&readerBusy,0);
        Sleep((DWORD)lpParam);//in ms
    }
    return 0;
}
DWORD WINAPI ThreadFuncDisplay(LPVOID lpParam) {
    DWORD displayBusyOn=1;
    DWORD displayedOn=-1;

    while(mainThreadLive != 0) {
        //TODO: check if need render
        displayBusyOn=1-readerBusyOn;
        if(displayedOn != displayBusyOn) {
            exSet(&displayBusy,1);
            displayedOn = displayBusyOn;
            ++displayedCount;
            DisplayBuffer(displayBusyOn);
            //printf("Display buffer %s #%lu/%lu\n",
            //    displayBufferName[displayBusyOn],
            //    displayedCount, readeredCount);
            exSet(&displayBusy,0);
        } else
            Sleep((DWORD)lpParam);//in ms
    }
    return 0;
}
DWORD WINAPI ThreadFuncService(LPVOID lpParam) {
    //TODO: add thread enter once code
    while(mainThreadLive != 0) {
        //TODO: check if need render
        Sleep((DWORD)lpParam);//in ms
    }
    //TODO: add thread leave once code
    return 0;
}

HANDLE threadHandle1, threadHandle2, threadHandle3;
DWORD exitCode1, exitCode2, exitCode3;
DWORD threadId1, threadId2, threadId3;
void startThreads(void) {
    threadHandle1 = CreateThread(NULL, 0, ThreadFuncRender, (LPVOID)10, 0, &threadId1 );
    if (threadHandle1)
        printf("Thread 1 launched\n");

    threadHandle2 = CreateThread(NULL, 0, ThreadFuncDisplay, (LPVOID)2, 0, &threadId2 );
    if(threadHandle2)
        printf("Thread 2 launched\n");

    threadHandle3 = CreateThread(NULL, 0, ThreadFuncService, (LPVOID)20, 0, &threadId3 );
    if(threadHandle3)
        printf("Thread 3 launched\n");
}
void closeThreads(void) {
    CloseHandle(threadHandle1);
    CloseHandle(threadHandle2);
    CloseHandle(threadHandle3);
    printf("Thread 1 returned %d\n", exitCode1);
    printf("Thread 2 returned %d\n", exitCode2);
    printf("Thread 3 returned %d\n", exitCode3);
}
int isThreadAllDone(void) {
        GetExitCodeThread(threadHandle1, &exitCode1);
        GetExitCodeThread(threadHandle2, &exitCode2);
        GetExitCodeThread(threadHandle3, &exitCode3);
        if ( exitCode1 == STILL_ACTIVE )
            puts("Thread 1 is still running!");
        if ( exitCode2 == STILL_ACTIVE )
            puts("Thread 2 is still running!");
        if ( exitCode3 == STILL_ACTIVE )
            puts("Thread 3 is still running!");

        if (   exitCode1 != STILL_ACTIVE
            && exitCode2 != STILL_ACTIVE
            && exitCode3 != STILL_ACTIVE)
            return 1;
        return 0;
}
int waitThreadAlldone(void) {
    mainThreadLive=0;
    //startThreads();
    // Keep waiting until both calls to GetExitCodeThread succeed AND
    // neither of them returns STILL_ACTIVE.
    for (;;)
    {
        if ( isThreadAllDone() )
            break;
        Sleep(100);
    }
    closeThreads();
    return EXIT_SUCCESS;
}

//called by main
void createGlobalResource();
void destroyGlobalResource();
void createWindowResource(HWND hwnd);
void destroyWindowResource(HWND hwnd);
static HWND mainCreatedHWND=NULL;

/*  Declare Windows procedure  */
/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("CodeBlocksWindowsApp");
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           _T("Code::Blocks Template Windows App"),       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           544,                 /* The programs width */
           375,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    if(hwnd == NULL) {
        printf("Fatal error, CreateWindowEx fail, exit\n");
        return -1;
    }
    mainCreatedHWND = hwnd;
    createGlobalResource();
    createWindowResource(hwnd);
    /* Make the window visible on the screen */
    ShowWindow (hwnd, nCmdShow);
    startThreads();

    int nrLoop=0;
    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
        //printf("Main thread loop %d waiting for next message\n", nrLoop++);
    }
    waitThreadAlldone();

    destroyWindowResource(hwnd);
    destroyGlobalResource();
    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    printf("Press enter to continue\n");
    fflush(stdout);
    //getchar();
    return messages.wParam;
}

int		clientWidth, clientHeight;//screen size
RECT	clientRect;

static POINT ptBegin;//mouse left down
static POINT ptEnd;//mouse left up
static POINT ptMove;//mouse move
static int guiDirty=0;//need render
static int msgCount=0;//debug counter
static int userControlShowTitle=0;
static int userControlPause=0;
static HWND hwndMain=NULL;
static HBRUSH hBrushBG, hBrushFG;
struct RenderBuffer {
    HDC renderDC;
    HBITMAP renderBmp;
    //other attribute for this buffer
    DWORD renderCount, displayCount;
    COLORREF bgColor;
};
struct RenderBuffer renderBuffer[2];//2 buffer for ping pang render/display

void createGlobalResource() {
    hBrushBG = CreateSolidBrush(RGB(16,16,16));
    hBrushFG = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
}
void destroyGlobalResource() {
	DeleteObject(hBrushBG);
	DeleteObject(hBrushFG);
    hBrushBG = NULL;
    hBrushFG = NULL;

    extern int matridTest(void);
    //matridTest();
}
void createWindowResource(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    int i;
    DWORD renderWidth=2560*2, renderHeight=1080*2;
    RECT rect;
    SetRect(&rect, 0, 0, renderWidth, renderHeight);
    COLORREF bgColor[2]={RGB(128,0,0),RGB(0,0,128)};
    for(i=0;i<2;i++) {
        renderBuffer[i].renderCount=0;
        renderBuffer[i].displayCount=0;
        renderBuffer[i].bgColor=bgColor[i];
        // 1.创建兼容缓冲区
        renderBuffer[i].renderDC = CreateCompatibleDC(hdc);   // 创建兼容DC
        // 创建兼容位图画布
        renderBuffer[i].renderBmp = CreateCompatibleBitmap(hdc, renderWidth, renderHeight);
        if(renderBuffer[i].renderBmp==NULL) {
            printf("Fatal error, out of memory when CreateCompatibleBitmap(hdc, 2560*2, 1080*2);");
        } else {
            SelectObject(renderBuffer[i].renderDC, renderBuffer[i].renderBmp);    // 选入
            //debug optional initialize to a back ground color
            HBRUSH hBrushTmp=CreateSolidBrush(bgColor[i]);
            FillRect(renderBuffer[i].renderDC, &rect, hBrushTmp);
            DeleteObject(hBrushTmp);
        }
    }

    TESTPLUGIN.open(0);
    TESTPLUGIN.registerObj(0, hdc);
    TESTPLUGIN.user(0,0,0);
    ReleaseDC(hwnd, hdc);
}
void destroyWindowResource(HWND hwnd) {
    int i;
    TESTPLUGIN.close();
    for(i=0;i<2;i++) {
        // 4.释放缓冲区DC
        DeleteDC(renderBuffer[i].renderDC);
    }
}

//===============函数定义=====================
//图像的旋转
//围绕图片中心点逆时针旋转
#define ijToxyx(i,j,w,h) (-0.5*(w) + (i))
#define ijToxyy(i,j,w,h) ( 0.5*(h) - (j))
#define xyToijx(x,y,w,h) ( 0.5*(w) + (x))
#define xyToijy(x,y,w,h) ( 0.5*(h) - (y))
POINT ijToxy(float i, float j,int w,int h) {
	POINT c;
	c.x = -0.5*w + i;
	c.y = 0.5*h - j;
	return c;
}
POINT xyToij(float x, float y,int w,int h) {
	POINT c;
	c.x = 0.5*w + x;
	c.y = 0.5*h - y;
	return c;
}

void BitBltRotate(HDC hdc, HDC hMemDC, HBITMAP hBitmap, int Width, int Heigh,int angle)
{
	BITMAP bm;
	GetObject(hBitmap, sizeof(bm), &bm);
	int WidthRow = bm.bmWidthBytes;//原位图的字节宽(即原位图的每行字节数)

	//获取原位图的像素位
	BYTE *Pixel = new BYTE[WidthRow*Heigh];
	GetBitmapBits(hBitmap, WidthRow*Heigh, (LPVOID)Pixel);

	//绕中心旋转，新图片宽和高最大为:
	int newWidth = Width + Heigh;
	int newHeigh = newWidth;

	BYTE *newPixel = new BYTE[newWidth*newHeigh * 4];//存放新的像素位
	memset(newPixel, 255, newWidth*newHeigh * 4 * sizeof(BYTE));

	//图像旋转
	for (int j = 0; j <Heigh; j++) {//行
		for (int i = 0; i < Width; i++) {//列
			//(i,j)转化为坐标系坐标
			float x0 = ijToxyx(i, j, Width, Heigh);//.x;
			float y0 = ijToxyy(i, j, Width, Heigh);//.y;

			float r = sqrt(x0*x0 + y0*y0);
			/*
				x0=r*cos(a),y0=r*sin(a)
				旋转b角度
				x1=r*cos(a+b)=r*(cos(a)cos(b)-sin(a)sin(b))
					=r*(x0/r * cos(b)-y0/r *sin(b))
				y1=r*sin(a+b)=r*(sin(a)cos(b)+cos(a)sin(b))
					=r*(y0/r *cos(b)+x0/r * sin(b))

			*/

			//原坐标(x0,y0)->新坐标(x1,y1)
			float x1 = r*(x0/r*cos(angle*PI/180)-y0/r*sin(angle*PI/180));
			float y1 = r*(y0/r*cos(angle*PI/180)+x0/r*sin(angle*PI/180));

			//坐标系坐标转化为(i,j)
			float i1 = xyToijx(x1, y1, Width, Heigh);//.x;
			float j1 = xyToijy(x1, y1, Width, Heigh);//.y;

			float i2 = i1+ 0.5*Heigh;
			float j2 = j1+0.5*Width;
			//每4个为一个像素位
			for (int k = 0; k < 4; k++)
			{
				newPixel[int(j2)*newWidth *4 + 4 *int(i2) + k] = Pixel[j*Width * 4 + 4 * i + k];
			}
		}
	}

	//创建一个新的位图
	HBITMAP hNewBitmap = CreateCompatibleBitmap(hdc, newWidth, newHeigh);

	SelectObject(hMemDC, hNewBitmap);//选进内存DC

	SetBitmapBits(hNewBitmap, newWidth*newHeigh * 4, newPixel);
	BitBlt(hdc, 0, 0, newWidth, newHeigh, hMemDC, 0, 0, SRCCOPY);

	DeleteObject(hNewBitmap);

	delete[] Pixel;
	delete[] newPixel;
}

void drawDCGrid(HDC hdc, int rows, int cols, int fillRandomColor) {
    int row, col;
    int gridW=clientWidth/cols;
    int gridH=clientHeight/rows;
    for(row=1;row<rows;row++) {
        MoveToEx(hdc, 0, row*clientHeight/rows, NULL);
        LineTo(hdc, clientWidth, row*clientHeight/rows);
    }
    for(col=1;col<cols;col++) {
        MoveToEx(hdc, col*clientWidth/cols, 0, NULL);
        LineTo(hdc, col*clientWidth/cols, clientHeight);
    }

    if(fillRandomColor==0 || gridH<5 || gridW<5)
        return;
    RECT rect;
    for(row=0;row<rows;row++) {
        for(col=0;col<cols;col++) {
            SetRect(&rect,
                col*clientWidth/cols  +2,
                row*clientHeight/rows +2,
                (1+col)*clientWidth/cols -2,
                (1+row)*clientHeight/rows -2);
            if((ptMove.x>=rect.left-2 && ptMove.x<rect.right+2 &&
                ptMove.y>=rect.top-2 && ptMove.y<rect.bottom+2) ||
                fillRandomColor==0xFF) {
                if(userControlShowTitle) {
                HBRUSH hBrush;
                if(fillRandomColor==0xFF)
                    hBrush = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
                else
                    hBrush = CreateSolidBrush(RGB(255,255,255));
                FillRect(hdc, &rect, hBrush);
                DrawText(hdc, "A", -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
                DeleteObject(hBrush);
                }
            }
        }
    }
}

void drawOnBGDC(HWND hWindow, HDC hdc, int index) {

    int fillRandomColor=1;
    HDC memoryDC=renderBuffer[index].renderDC;
    renderBuffer[index].renderCount++;

    // 2.在缓冲区绘制
	RECT		rect;
	//draw whole background first
	SetRect(&rect, 0, 0, clientWidth, clientHeight);
	//select on of next to erase background, or left it blink
 	FillRect(memoryDC, &rect, hBrushBG);
 	//BitBlt(memoryDC, 0, 0, clientWidth, clientHeight, memoryDC, 0, 0, SRCERASE);
 	drawDCGrid(memoryDC,13, 30, fillRandomColor);

 	if(fillRandomColor==0) {
 	HBRUSH hBrushRandom = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
 	int left=0, top=0, right=clientWidth, bottom=clientHeight;
 	//random on client area
 	if(clientWidth>100) {
 	    left =rand() % (clientWidth-100);
 	    right=left+100;
 	}
    if(clientHeight>100) {
        top =rand() % (clientHeight-100);
        bottom=top+100;
    }
	SetRect(&rect, left, top, right, bottom);
 	FillRect(memoryDC, &rect, hBrushRandom);
 	DeleteObject(hBrushRandom);

    //Rectangle(memoryDC, 100, 100, 200, 200);
	SetRect(&rect,100, 100, 200, 200);
 	FillRect(memoryDC, &rect, hBrushFG);

    //Rectangle(memoryDC, 300, 300, 200, 200);
	SetRect(&rect,300, 300, 200, 200);
 	FillRect(memoryDC, &rect, hBrushFG);
 	}

    //TESTPLUGIN.render(NULL);
    TESTPLUGIN.display(memoryDC, clientWidth, clientHeight);
    if(userControlShowTitle) {
	//在兼容DC中间位置输出字符串, 相当于把hbmp这个位图加上了文字标注,
    //DrawText(memoryDC,"Center Line Text", -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
    SetRect(&rect, 0, 0, clientWidth, clientHeight);
    char title[256];
    sprintf(title, "  Center Line Text rendering %s on message %x #%lu/%lu  ",
        displayBufferName[index], renderIdentifier,
        displayedCount, readeredCount);
    DrawText(memoryDC, title, -1, &rect, DT_TOP|DT_SINGLELINE|DT_CENTER);

    rect.top += 24;
    sprintf(title, "  RenderBuffer display/render %lu/%lu msg:%x %x %x %d %d  ",
        renderBuffer[index].displayCount, renderBuffer[index].renderCount,
        renderIdentifier, renderIdwp, renderIdlp,
        LOWORD(renderIdlp), HIWORD(renderIdlp));
    DrawText(memoryDC, title, -1, &rect, DT_TOP|DT_SINGLELINE|DT_CENTER);
    }
}
void drawFromBGDC(HWND hWindow, HDC hdc, int index) {
    HDC memoryDC=renderBuffer[index].renderDC;
    renderBuffer[index].displayCount++;
    // 3.一次性复制到设备DC
    BitBlt(hdc, 0, 0, clientWidth, clientHeight, memoryDC, 0, 0, SRCCOPY);
    //BitBltRotate(hdc, memoryDC, renderBuffer[index].renderBmp, clientWidth, clientHeight, 30);
    //SelectObject(renderBuffer[index].renderDC, renderBuffer[index].renderBmp);
}
int RenderBuffer(DWORD bufferIndex) {
 	if(userControlPause==0) {
 	    TESTPLUGIN.user(1,0,0);
 	    guiDirty = 1;
 	}
    if(guiDirty) {
        HDC         hdc;
        hdc = GetDC(hwndMain);
        readeredCount++;
        drawOnBGDC(hwndMain, hdc, bufferIndex);
        ReleaseDC(hwndMain, hdc);
        guiDirty=0;
        return 1;
    }
    return 0;
}
void DisplayBuffer(DWORD bufferIndex) {
    HDC hdc = GetDC(hwndMain);
    drawFromBGDC(hwndMain, hdc, bufferIndex);
    ReleaseDC(hwndMain, hdc);
}
void DisplayBufferOnPaint(HWND hwnd) {
    HDC         hdc;
    PAINTSTRUCT ps;
    hdc = BeginPaint(hwnd, &ps);
    drawFromBGDC(hwndMain, hdc, 1-readerBusyOn);
    EndPaint(hwnd, &ps);
}
//----------------------------------------------------------------------
#if 0
static HRGN	hRgnClip;
void drawRaginTest(HDC hdc) {
    //由四个椭圆形成一个区域，然后把这个区域选入设备环境，
    //接着从窗口的客户区中心发散绘制一系列直线。这些直线仅出现裁剪区内。
        double fAngle, fRadius;
		SetViewportOrgEx(hdc, clientWidth/2, clientHeight/2, NULL);
		SelectClipRgn(hdc, hRgnClip);

		// hypot 计算直角三角形斜边的长
		fRadius = _hypot(clientWidth/2.0, clientHeight/2.0);

		for (fAngle = 0.0; fAngle < TWO_PI; fAngle += TWO_PI/360)
		{
			MoveToEx(hdc, 0, 0, NULL);
			LineTo(hdc, int(fRadius * cos(fAngle) + 0.5), int(-fRadius * sin(fAngle) + 0.5));
		}
}
void createTestRagin() {
    //InvalidRect函数使显示的矩形区域无效，可以用来擦除客户区的内容, 并产生一个WM_PAINT消息。
    //若处理区域而不是矩形可用：InvalidateRgn(hwnd, hRgn, bErase); ValidateRgn(hwnd, hRgn);

    //由四个椭圆形成一个区域
		HCURSOR	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
		ShowCursor(TRUE);

		if (hRgnClip)
			DeleteObject(hRgnClip);

        HRGN			hRgnTemp[6];

		hRgnTemp[0] = CreateEllipticRgn(0,			clientHeight/3,		clientWidth/2,		2*clientHeight/3);
		hRgnTemp[1] = CreateEllipticRgn(clientWidth/2,	clientHeight/3,		clientWidth,		2*clientHeight/3);
		hRgnTemp[2] = CreateEllipticRgn(clientWidth/3,	0,				2*clientWidth/3,	clientHeight/2);
		hRgnTemp[3] = CreateEllipticRgn(clientWidth/3, clientHeight/2,		2*clientWidth/3,	clientHeight);
		hRgnTemp[4] = CreateRectRgn(0, 0, 1, 1);
		hRgnTemp[5] = CreateRectRgn(0, 0, 1, 1);
		hRgnClip = CreateRectRgn(0, 0, 1, 1);

		CombineRgn(hRgnTemp[4], hRgnTemp[0], hRgnTemp[1], RGN_OR);
		CombineRgn(hRgnTemp[5], hRgnTemp[2], hRgnTemp[3], RGN_OR);
		CombineRgn(hRgnClip, hRgnTemp[4], hRgnTemp[5], RGN_XOR);

		int i;
		for (i = 0; i < 6; i++)
			DeleteObject(hRgnTemp[i]);

		SetCursor(hCursor);
		ShowCursor(FALSE);
}
void drawMiscShape(HDC hdc) {
		Rectangle(hdc, clientWidth/8, clientHeight/8, 7*clientWidth/8, 7*clientHeight/8);
		MoveToEx(hdc, 0, 0, NULL);
		LineTo(hdc, clientWidth, clientHeight);
		MoveToEx(hdc, 0, clientHeight, NULL);
		LineTo(hdc, clientWidth, 0);
		Ellipse(hdc, clientWidth/8, clientHeight/8, 7*clientWidth/8, 7*clientHeight/8);
		RoundRect(hdc, clientWidth/4, clientHeight/4, 3*clientWidth/4, 3*clientHeight/4, clientWidth/4, clientHeight/4);
}

void drawSineWave(HDC hdc) {
        #define NUM		1000
        POINT			apt[NUM];
        int i;

		for (i = 0; i < NUM; i++)
		{
			apt[i].x = i*clientWidth/NUM;
			apt[i].y = (int)(clientHeight / 2 * (1-sin(TWOPI * i / NUM)));
		}
		Polyline(hdc, apt, NUM);
}
void FillWindowToColor(HDC hdc, int color) {
	HBRUSH		hBrush;
	RECT		rect;

	if (clientWidth == 0 || clientHeight == 0)
		return;

	SetRect(&rect, 0, 0, clientWidth, clientHeight);
 	hBrush = CreateSolidBrush(RGB(color&0xFF, (color>>8)&0xFF,(color>>16)&0xFF));
 	FillRect(hdc, &rect, hBrush);
	DeleteObject(hBrush);
}
void RandomFillRect(HDC hdc)
{
	HBRUSH		hBrush;
	RECT		rect;

	if (clientWidth == 0 || clientHeight == 0)
		return;

	SetRect(&rect, rand() % clientWidth, rand() % clientHeight, rand() % clientWidth, rand() % clientHeight);
 	hBrush = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
 	FillRect(hdc, &rect, hBrush);
	DeleteObject(hBrush);
}


void checkDirty(HWND hwnd)
{
    enum DrawType_e
    {
        TYPE_LINE,
        TYPE_RECT,
        TYPE_ELLIPSE
    };
    if(guiDirty>0) {
        guiDirty=0;
        //force update GUI.
        HDC         hdc;
        //在处理非WM_PAINT消息时由Windows程序获取：matridTest
        hdc = GetDC(hwnd);
        //获得用于整个窗口的，而不仅仅是窗口客户区的设备环境句柄：
        //hdc = GetWindowDC(hwnd);
        //获取当前整个屏幕的设备环境句柄：
        //hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
        FillWindowToColor(hdc, 0xFF);

        //RandomFillRect(hdc);
        //drawRaginTest(hdc);
        /*
        int nTypy = TYPE_LINE;
        switch (nTypy)
        {
        case TYPE_LINE:
            MoveToEx(hdc, ptBegin.x, ptBegin.y, NULL);
            LineTo(hdc, ptEnd.x, ptEnd.y);
            break;
        case TYPE_RECT:
            Rectangle(hdc, ptBegin.x, ptBegin.y, ptEnd.x, ptEnd.y);
            break;
        case TYPE_ELLIPSE:
            Ellipse(hdc, ptBegin.x, ptBegin.y, ptEnd.x, ptEnd.y);
            break;
        default:
            break;
        }
        */
        ReleaseDC(hwnd, hdc);
    }
}
void onDraw(HWND hwnd)
{
    static int debugCount=0,onDrawCount=0;
    //document: https://blog.csdn.net/wowocpp/article/details/79299016
    HDC         hdc;
    PAINTSTRUCT ps;
    //在处理WM_PAINT消息时使用BeginPaint函数和EndPaint函数：
    hdc = BeginPaint(hwnd, &ps);                         // 1
    printf("onDraw ordered %d\n",onDrawCount++);
    /*
    drawMiscShape(hdc);
    drawSineWave(hdc);
    char msg[256];

    if(guiDirty>0)
        sprintf(msg,"    %s #%d    guiDirty=%d    ","Dirty World!", debugCount++, guiDirty);
    else
        sprintf(msg,"    %s #%d     ","Hello World!", debugCount++);
    TextOut(hdc, 0, 0, _T(msg), _tcslen(_T(msg)));
    Rectangle(hdc, 50, 50, 200, 200);                    // 2
    Rectangle(hdc, 300, 50, 500, 200);                   // 3
    Ellipse(hdc, 50, 50, 200, 200);                      // 4
    Ellipse(hdc, 300, 200, 500, 50);                     // 5
    */
    EndPaint(hwnd, &ps);                                 // 6
}
#endif // 0

/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    msgCount++;
    if(hwndMain == NULL) {
        if(hwnd != mainCreatedHWND) {
            printf("Fatal error: Window is not what has been created\n");
            return DefWindowProc (hwnd, message, wParam, lParam);
        }
        hwndMain = hwnd;
    }
    else if(hwndMain != hwnd) {
        printf("Fatal error: Window handle changed\n");
        return -1;
    }
    renderIdentifier = message;//for debug only
    renderIdwp = wParam;
    renderIdlp = lParam;
    switch (message)                  /* handle the messages */
    {
        case WM_SIZE://窗口大小消息
            clientWidth = LOWORD(lParam);
            clientHeight = HIWORD(lParam);
            SetRect(&clientRect, 0, 0, clientWidth, clientHeight);
            guiDirty++;
            //createTestRagin();
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        case WM_PAINT: //Use BeginPaint() EndPaint() to reset invalid area
            //guiDirty++;
            DisplayBufferOnPaint(hwnd);
            printf("Here is a WM_PAINT 0x%03X ordered %d\n",message, msgCount++);
            break;

        //mouse event
        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            ptBegin.x = ptMove.x = LOWORD(lParam);
            ptBegin.y = ptMove.y = HIWORD(lParam);
            guiDirty++;
            printf("Here is a Mouse LBDn message 0x%03X %d %d\n",message, ptBegin.x, ptBegin.y);
            break;
        case WM_KEYUP:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            ptEnd.x = ptMove.x = LOWORD(lParam);
            ptEnd.y = ptMove.y = HIWORD(lParam);
            guiDirty++;
            printf("Here is a Mouse LBUp message 0x%03X %d %d\n",message, ptEnd.x, ptEnd.y);
            if(message == WM_RBUTTONUP)
                userControlShowTitle = 1 - userControlShowTitle;

            if(message == WM_LBUTTONUP)
                userControlPause = 1 - userControlPause;
            break;
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
            ptEnd.x = ptMove.x = LOWORD(lParam);
            ptEnd.y = ptMove.y = HIWORD(lParam);
            guiDirty++;
            printf("Here is a Mouse LBDc message 0x%03X %d %d\n",message, ptEnd.x, ptEnd.y);
            break;

        case WM_MOUSEHOVER:
        case WM_MOUSELEAVE:
        case WM_MOUSEWHEEL:
        case WM_MOUSEMOVE:
            ptMove.x = LOWORD(lParam);
            ptMove.y = HIWORD(lParam);
            guiDirty++;
            printf("Here is a Mouse move message 0x%03X %d %d\n",message, ptMove.x, ptMove.y);
            break;

        case WM_NCHITTEST: //0x84,132
        case MF_MENUBARBREAK://0x32, 32
        case WM_NCMOUSEMOVE://0xA0,160, cursor on border for change size
        case 0x2A2: //cursor inter/leave window
            //return DefWindowProc (hwnd, message, wParam, lParam);
            //break;
        default:                      /* for messages that we don't deal with */
            //printf("Ignore MSG 0x%03X 0x%X 0x%X\n",message, wParam, lParam);
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
