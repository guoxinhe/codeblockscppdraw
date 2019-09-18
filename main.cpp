#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <math.h>//for sin(float);


#define WIN32_LEAN_AND_MEAN
int mainLive=1;
HWND hwndMain=NULL;
void checkDirty(HWND hwnd);

DWORD WINAPI ThreadFunc(LPVOID n)
{
    int nrLoop=0;
    while(mainLive>0) {
        Sleep(10*(DWORD)n);//in ms
        if((DWORD)n==1 && hwndMain!=NULL)
            checkDirty(hwndMain);
        //else
        //    printf("thread[%d] loop %d waiting for next loop\n", (DWORD)n, nrLoop++);
    }
    //Sleep((DWORD)n*1000*2);
    //return (DWORD)n * 10;
    return 0;
}
class ThreadObject {
    HANDLE hThread;
    public: DWORD exitCode;
    DWORD threadId;
    DWORD initSeed;
    public: int isThreadDone() {
        GetExitCodeThread(hThread, &exitCode);
        if ( exitCode == STILL_ACTIVE ) {
            printf("Thread %d is still running!\n", initSeed);
            return 0;
        }
        return 1;
    }
    public: void startThread(DWORD seed) {
        initSeed=seed;
        hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)initSeed, 0, &threadId );
        if (hThread)
            printf("Thread %d launched\n", initSeed);
    }
    public: void stopThread() {
        CloseHandle(hThread);
        printf("Thread %d returned %d\n", initSeed, exitCode);
    }
};

HANDLE hThrd1;
HANDLE hThrd2;
DWORD exitCode1 = 0;
DWORD exitCode2 = 0;
DWORD threadId1, threadId2;
ThreadObject thredObj;
void startThreads(void) {
    hThrd1 = CreateThread(NULL, 0, ThreadFunc, (LPVOID)1, 0, &threadId1 );
    if (hThrd1)
        printf("Thread 1 launched\n");

    hThrd2 = CreateThread(NULL, 0, ThreadFunc, (LPVOID)200, 0, &threadId2 );
    if(hThrd2)
        printf("Thread 2 launched\n");

    thredObj.startThread(100);
}
int isThreadAllDone(void) {
        GetExitCodeThread(hThrd1, &exitCode1);
        GetExitCodeThread(hThrd2, &exitCode2);
        if ( exitCode1 == STILL_ACTIVE )
            puts("Thread 1 is still running!");
        if ( exitCode2 == STILL_ACTIVE )
            puts("Thread 2 is still running!");
        if ( exitCode1 != STILL_ACTIVE && exitCode2 != STILL_ACTIVE &&thredObj.isThreadDone())
            return 1;
        return 0;
}
void closeThreads(void) {
    CloseHandle(hThrd1);
    CloseHandle(hThrd2);
    printf("Thread 1 returned %d\n", exitCode1);
    printf("Thread 2 returned %d\n", exitCode2);
    thredObj.stopThread();
}
int waitThreadAlldone(void)
{
    mainLive=0;
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


/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("CodeBlocksWindowsApp");

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
    printf("Press enter to continue\n");
    fflush(stdout);
    //getchar();

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
#define TWOPI	(2*3.14159)
#define TWO_PI TWOPI
int		cxClient, cyClient;//screen size
static POINT ptBegin;//mouse left down
static POINT ptEnd;//mouse left up
static POINT ptMove;//mouse move
static int guiDirty=0;//need render
static int msgCount=0;//debug counter
static HRGN	hRgnClip;

void drawRaginTest(HDC hdc) {
    //由四个椭圆形成一个区域，然后把这个区域选入设备环境，
    //接着从窗口的客户区中心发散绘制一系列直线。这些直线仅出现裁剪区内。
        double fAngle, fRadius;
		SetViewportOrgEx(hdc, cxClient/2, cyClient/2, NULL);
		SelectClipRgn(hdc, hRgnClip);

		// hypot 计算直角三角形斜边的长
		fRadius = _hypot(cxClient/2.0, cyClient/2.0);

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

		hRgnTemp[0] = CreateEllipticRgn(0,			cyClient/3,		cxClient/2,		2*cyClient/3);
		hRgnTemp[1] = CreateEllipticRgn(cxClient/2,	cyClient/3,		cxClient,		2*cyClient/3);
		hRgnTemp[2] = CreateEllipticRgn(cxClient/3,	0,				2*cxClient/3,	cyClient/2);
		hRgnTemp[3] = CreateEllipticRgn(cxClient/3, cyClient/2,		2*cxClient/3,	cyClient);
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
		Rectangle(hdc, cxClient/8, cyClient/8, 7*cxClient/8, 7*cyClient/8);
		MoveToEx(hdc, 0, 0, NULL);
		LineTo(hdc, cxClient, cyClient);
		MoveToEx(hdc, 0, cyClient, NULL);
		LineTo(hdc, cxClient, 0);
		Ellipse(hdc, cxClient/8, cyClient/8, 7*cxClient/8, 7*cyClient/8);
		RoundRect(hdc, cxClient/4, cyClient/4, 3*cxClient/4, 3*cyClient/4, cxClient/4, cyClient/4);
}

void drawSineWave(HDC hdc) {
        #define NUM		1000
        POINT			apt[NUM];
        int i;

		for (i = 0; i < NUM; i++)
		{
			apt[i].x = i*cxClient/NUM;
			apt[i].y = (int)(cyClient / 2 * (1-sin(TWOPI * i / NUM)));
		}
		Polyline(hdc, apt, NUM);
}
void FillWindowToColor(HDC hdc, int color) {
	HBRUSH		hBrush;
	RECT		rect;

	if (cxClient == 0 || cyClient == 0)
		return;

	SetRect(&rect, 0, 0, cxClient, cyClient);
 	hBrush = CreateSolidBrush(RGB(color&0xFF, (color>>8)&0xFF,(color>>16)&0xFF));
 	FillRect(hdc, &rect, hBrush);
	DeleteObject(hBrush);
}
void RandomFillRect(HDC hdc)
{
	HBRUSH		hBrush;
	RECT		rect;

	if (cxClient == 0 || cyClient == 0)
		return;

	SetRect(&rect, rand() % cxClient, rand() % cyClient, rand() % cxClient, rand() % cyClient);
 	hBrush = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
 	FillRect(hdc, &rect, hBrush);
	DeleteObject(hBrush);
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
enum DrawType_e
    {
        TYPE_LINE,
        TYPE_RECT,
        TYPE_ELLIPSE
    };
void checkDirty(HWND hwnd)
{
    if(guiDirty>0) {
        guiDirty=0;
        //force update GUI.
        HDC         hdc;
        //在处理非WM_PAINT消息时由Windows程序获取：
        hdc = GetDC(hwnd);
        //获得用于整个窗口的，而不仅仅是窗口客户区的设备环境句柄：
        //hdc = GetWindowDC(hwnd);
        //获取当前整个屏幕的设备环境句柄：
        //hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
        FillWindowToColor(hdc, 0xFF);

        RandomFillRect(hdc);
        drawRaginTest(hdc);
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
        ReleaseDC(hwnd, hdc);
    }
}

/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    guiDirty=0;
    msgCount++;
    hwndMain = hwnd;
    switch (message)                  /* handle the messages */
    {

        case WM_SIZE://窗口大小消息
            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);
            createTestRagin();
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        case WM_PAINT:
            //guiDirty++;
            //checkDirty(hwnd);
            onDraw(hwnd);
            printf("Here is a WM_PAINT 0x%03X ordered %d\n",message, msgCount++);
            break;

        //mouse event
        case WM_LBUTTONDOWN:
            ptBegin.x = LOWORD(lParam);
            ptBegin.y = HIWORD(lParam);
            guiDirty++;
            break;
        case WM_LBUTTONUP:
            ptEnd.x = LOWORD(lParam);
            ptEnd.y = HIWORD(lParam);
            guiDirty++;
            break;
        case WM_MOUSEMOVE:
            ptMove.x = LOWORD(lParam);
            ptMove.y = HIWORD(lParam);
            //printf("Here is a Mouse move message 0x%03X %d %d\n",message, ptMove.x, ptMove.y);
            break;

        case WM_NCHITTEST: //0x84,132
        case MF_MENUBARBREAK://0x32, 32
        case WM_NCMOUSEMOVE://0xA0,160, cursor on border for change size
        case 0x2A2: //cursor inter/leave window
            //return DefWindowProc (hwnd, message, wParam, lParam);
            //break;
        default:                      /* for messages that we don't deal with */
            printf("Ignore MSG 0x%03X 0x%X 0x%X\n",message, wParam, lParam);
            return DefWindowProc (hwnd, message, wParam, lParam);
    }
    //checkDirty(hwnd);

    return 0;
}
