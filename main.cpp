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

#define exInc(pLong) InterlockedIncrement((volatile LONG *)pLong)            //++
#define exDec(pLong) InterlockedDecrement((volatile LONG *)pLong)            //--
#define exAdd(pLong,val) InterlockedExchangeAdd((volatile LONG *)pLong, val) //+=
#define exSet(pLong,val) InterlockedExchange((volatile LONG *)pLong, val)    //=
#define exMod(pLong,to,out) InterlockedCompareExchange((volatile LONG *)pLong, to, out)

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
DWORD renderIdentifier=0;
const char *displayBufferName[]={"[    -- ]","[ --    ]"};

DWORD WINAPI ThreadFuncRender(LPVOID n) {
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
        Sleep(15);//in ms
    }
    return 0;
}
DWORD WINAPI ThreadFuncDisplay(LPVOID n) {
    DWORD displayBusyOn=1;
    DWORD displayedOn=-1;

    while(mainThreadLive != 0) {
        //TODO: check if need render
        exSet(&displayBusy,1);
        displayBusyOn=1-readerBusyOn;
        //display it
        if(displayedOn != displayBusyOn) {
            ++displayedCount;
            DisplayBuffer(displayBusyOn);
            displayedOn = displayBusyOn;
            printf("Display buffer %s #%lu/%lu\n",
                displayBufferName[displayBusyOn],
                displayedCount, readeredCount);
        }
        exSet(&displayBusy,0);
        Sleep(16);//in ms
    }
    return 0;
}
DWORD WINAPI ThreadFuncService(LPVOID n) {
    while(mainThreadLive != 0) {
        //TODO: check if need render
        Sleep(20);//in ms
    }
    return 0;
}

HANDLE threadHandle1, threadHandle2, threadHandle3;
DWORD exitCode1, exitCode2, exitCode3;
DWORD threadId1, threadId2, threadId3;
void startThreads(void) {
    threadHandle1 = CreateThread(NULL, 0, ThreadFuncRender, (LPVOID)10, 0, &threadId1 );
    if (threadHandle1)
        printf("Thread 1 launched\n");

    threadHandle2 = CreateThread(NULL, 0, ThreadFuncDisplay, (LPVOID)16, 0, &threadId2 );
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

#define TWOPI	(2*3.14159)
#define TWO_PI TWOPI
int		clientWidth, clientHeight;//screen size
RECT	clientRect;

static POINT ptBegin;//mouse left down
static POINT ptEnd;//mouse left up
static POINT ptMove;//mouse move
static int guiDirty=0;//need render
static int msgCount=0;//debug counter
static HRGN	hRgnClip;
static HWND hwndMain=NULL;

HBRUSH hBrushBG, hBrushFG;
HDC renderDC[2]; //2 buffer for ping pang render/display
HBITMAP renderBmp[2];

void createGlobalResource() {
    hBrushBG = CreateSolidBrush(RGB(0,128,0));
    hBrushFG = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
}
void destroyGlobalResource() {
	DeleteObject(hBrushBG);
	DeleteObject(hBrushFG);
    hBrushBG = NULL;
    hBrushFG = NULL;
}
void createWindowResource(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    int i;
    for(i=0;i<2;i++) {
        // 1.�������ݻ�����
        renderDC[i] = CreateCompatibleDC(hdc);   // ��������DC
        renderBmp[i] = CreateCompatibleBitmap(hdc, 2560*2, 1080*2);   // ��������λͼ����
        if(renderBmp[i]==NULL)
            printf("Fatal error, out of memory when CreateCompatibleBitmap(hdc, 2560*2, 1080*2);");
        SelectObject(renderDC[i], renderBmp[i]);    // ѡ��
    }

    ReleaseDC(hwnd, hdc);
}
void destroyWindowResource(HWND hwnd) {
    int i;
    for(i=0;i<2;i++) {
        // 4.�ͷŻ�����DC
        DeleteDC(renderDC[i]);
    }
}
void drawOnBGDC(HWND hWindow, HDC hdc, int index)
{
    HDC memoryDC=renderDC[index];

    // 2.�ڻ���������
	RECT		rect;
	//draw whole background first
	SetRect(&rect, 0, 0, clientWidth, clientHeight);
 	//FillRect(memoryDC, &rect, hBrushBG);
 	//BitBlt(memoryDC, 0, 0, clientWidth, clientHeight, memoryDC, 0, 0, SRCERASE);

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

	//�ڼ���DC�м�λ������ַ���, �൱�ڰ�hbmp���λͼ���������ֱ�ע,
    //DrawText(memoryDC,"Center Line Text", -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
    SetRect(&rect, 0, 0, clientWidth, clientHeight);
    char title[256];
    sprintf(title, "  Center Line Text rendering %s on message %x #%lu/%lu  ",
        displayBufferName[index], renderIdentifier,
        displayedCount, readeredCount);
    DrawText(memoryDC, title, -1, &rect, DT_TOP|DT_SINGLELINE|DT_CENTER);
}
void drawFromBGDC(HWND hWindow, HDC hdc, int index)
{
    HDC memoryDC=renderDC[index];
    // 3.һ���Ը��Ƶ��豸DC
    BitBlt(hdc, 0, 0, clientWidth, clientHeight, memoryDC, 0, 0, SRCCOPY);
}

int RenderBuffer(DWORD bufferIndex) {
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
void drawRaginTest(HDC hdc) {
    //���ĸ���Բ�γ�һ������Ȼ����������ѡ���豸������
    //���ŴӴ��ڵĿͻ������ķ�ɢ����һϵ��ֱ�ߡ���Щֱ�߽����ֲü����ڡ�
        double fAngle, fRadius;
		SetViewportOrgEx(hdc, clientWidth/2, clientHeight/2, NULL);
		SelectClipRgn(hdc, hRgnClip);

		// hypot ����ֱ��������б�ߵĳ�
		fRadius = _hypot(clientWidth/2.0, clientHeight/2.0);

		for (fAngle = 0.0; fAngle < TWO_PI; fAngle += TWO_PI/360)
		{
			MoveToEx(hdc, 0, 0, NULL);
			LineTo(hdc, int(fRadius * cos(fAngle) + 0.5), int(-fRadius * sin(fAngle) + 0.5));
		}
}
void createTestRagin() {
    //InvalidRect����ʹ��ʾ�ľ���������Ч���������������ͻ���������, ������һ��WM_PAINT��Ϣ��
    //��������������Ǿ��ο��ã�InvalidateRgn(hwnd, hRgn, bErase); ValidateRgn(hwnd, hRgn);

    //���ĸ���Բ�γ�һ������
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
        //�ڴ����WM_PAINT��Ϣʱ��Windows�����ȡ��
        hdc = GetDC(hwnd);
        //��������������ڵģ����������Ǵ��ڿͻ������豸���������
        //hdc = GetWindowDC(hwnd);
        //��ȡ��ǰ������Ļ���豸���������
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
    //�ڴ���WM_PAINT��Ϣʱʹ��BeginPaint������EndPaint������
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
    switch (message)                  /* handle the messages */
    {
        case WM_SIZE://���ڴ�С��Ϣ
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
        case WM_LBUTTONDOWN:
            ptBegin.x = LOWORD(lParam);
            ptBegin.y = HIWORD(lParam);
            guiDirty++;
            printf("Here is a Mouse LBDn message 0x%03X %d %d\n",message, ptBegin.x, ptBegin.y);
            break;
        case WM_LBUTTONUP:
            ptEnd.x = LOWORD(lParam);
            ptEnd.y = HIWORD(lParam);
            guiDirty++;
            printf("Here is a Mouse LBUp message 0x%03X %d %d\n",message, ptEnd.x, ptEnd.y);
            break;
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
