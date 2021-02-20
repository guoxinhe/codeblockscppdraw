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

static int threadRunFlag=1;
static int wastTimeCount=0;
#define ThreadSpinLockWait(a) do{int _to=(a); while(_to-->0) {wastTimeCount++;}} while(0)
/**
 * @return if success, used wait counter, for statistics, else assert fail
 */
void spinLock(volatile long *pLock) {
    long timeout=0x10000000;
    long retVal;
    //QWORD now=spinLockWaitCycleCounter;
    do {
        if(*pLock == 0) {
            retVal = exInc(pLock);
            if(retVal == 1) //success locked
                break;
            exDec(pLock);//avoid dead lock by other threads
        }

        //wait a few CPU cycle here
        ThreadSpinLockWait(1);
    }while(threadRunFlag && timeout-->0);

    //if(timeout<=0)
    //    AssertThrow(timeout>0);
    //return (TWORD)(spinLockWaitCycleCounter-now);
}
/**
 * a timeout version of spinLock(pLock)
 * @return if success, return 0, else return -1
 */
int spinLockTry(volatile long *pLock, long timeout) {
    long retVal;
    while(threadRunFlag) {
        if(*pLock == 0) {
            retVal = exInc(pLock);
            if(retVal == 1) //success locked
                return 0;
            exDec(pLock);//avoid dead lock by other threads
        }
        if(timeout<=0)
            break;
        timeout--;
        //wait a few CPU cycle here
        ThreadSpinLockWait(1);
    }

    return -1;
}

void spinUnlock(volatile long *pLock) {
    exDec(pLock);
}

//thread used external callback function
int RenderBuffer(DWORD bufferIndex);
void DisplayBuffer(DWORD bufferIndex);

struct RenderBufferDevice {
    HDC renderDC;
    HBITMAP renderBmp;
    //other attribute for this buffer
    DWORD renderCount, displayCount;
    COLORREF bgColor;
};

class DisplayManager {
private:
    long statusLock;
    long renderDirty;

    long renderCount;
    long renderBusy;
    long renderBusyIndex;
    long renderFreshIndex;
    long renderNext;

    long displayDirty;
    long displayBusy;
    long displayBusyIndex;
    long displayCount;

    int renderStatisCount[2];

    //------------------------------------------------------
    HWND hwndHost;
    //2 buffer for ping pang render/display
    struct RenderBufferDevice renderBuffer[2];
    HBRUSH hBrushBG, hBrushFG;
public:
    DisplayManager() {
        statusLock=0;

        renderDirty=1;
        renderCount=0;
        renderBusy=0;
        renderFreshIndex=0;
        renderBusyIndex=0;

        displayDirty=0;
        displayBusy=0;
        displayBusyIndex=0;
        displayCount=0;

        renderStatisCount[0]=0;
        renderStatisCount[1]=0;
    };
    void onCreate(HWND hwnd) {
        hwndHost = hwnd;
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

        ReleaseDC(hwnd, hdc);

        hBrushBG = CreateSolidBrush(RGB(16,16,16));
        hBrushFG = CreateSolidBrush(RGB(116,116,116));
    }
    void onDestroy() {
        int i;
        for(i=0;i<2;i++) {
            // 4.释放缓冲区DC
            if(renderBuffer[i].renderDC) {
                DeleteDC(renderBuffer[i].renderDC);
                renderBuffer[i].renderDC=NULL;
            }
        }
        if(hBrushBG) {
            DeleteObject(hBrushBG);
            hBrushBG = NULL;
        }
        if(hBrushFG) {
            DeleteObject(hBrushFG);
            hBrushFG = NULL;
        }

        printf("DisplayManager: renderCount=%d(%d,%d), displayCount=%d\n",
            renderCount, renderStatisCount[0], renderStatisCount[1],
            displayCount);
    }
    int render() {
        if(renderDirty==0)
            return 0;
        exSet(&renderDirty,0);
        spinLock((volatile long *)&statusLock);
        exSet(&renderBusy,1);
        if(displayBusy) {
            renderBusyIndex = 1-displayBusyIndex;
        } else {
            renderBusyIndex = 1-renderFreshIndex;
        }
        spinUnlock((volatile long *)&statusLock);

        //------------------------------- render action
        //HDC         hdc;
        //hdc = GetDC(hwndHost);
        //drawOnBGDC(hwndHost, hdc, bufferIndex);
        HDC memoryDC=renderBuffer[renderBusyIndex].renderDC;
        RECT		rect;
        //draw whole background first
        SetRect(&rect, 0, 0, 640, 480);
        //select on of next to erase background, or left it blink
        FillRect(memoryDC, &rect, hBrushBG);
        char title[256];
        sprintf(title, "  Render #%d #%d %d  ",
            renderBusyIndex, renderCount, renderStatisCount[renderBusyIndex]);
        DrawText(memoryDC, title, -1, &rect, DT_TOP|DT_SINGLELINE|DT_CENTER);
        //ReleaseDC(hwndHost, hdc);

        //after render:
        renderCount++;
        renderStatisCount[renderBusyIndex]++;
        exSet(&renderFreshIndex,renderBusyIndex);
        exSet(&renderBusy,0);
        exSet(&displayDirty,1);
        return 0;
    }
    int display() {
        if(displayDirty==0)
            return 0;
        exSet(&displayDirty,0);
        spinLock((volatile long *)&statusLock);
        exSet(&displayBusy,1);
        exSet(&displayBusyIndex,renderFreshIndex);
        spinUnlock((volatile long *)&statusLock);

        //------------------------------- render action
        HDC         hdc;
        hdc = GetDC(hwndHost);
        HDC memoryDC=renderBuffer[displayBusyIndex].renderDC;
        BitBlt(hdc, 0, 0, 640, 480, memoryDC, 0, 0, SRCCOPY);
        ReleaseDC(hwndHost, hdc);

        //after display
        displayCount++;
        exSet(&displayBusy,0);
        exSet(&renderDirty,1);
        return 0;
    }
};

DisplayManager *displayManager=new DisplayManager();

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
        displayManager->render();
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
        //Sleep((DWORD)lpParam);//in ms
        Sleep((DWORD)15);//in ms
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
        } else {
            //Sleep((DWORD)lpParam);//in ms
            Sleep((DWORD)1);//in ms
        }
        displayManager->display();
    }
    return 0;
}
DWORD WINAPI ThreadFuncService(LPVOID lpParam) {
    //TODO: add thread enter once code
    while(mainThreadLive != 0) {
        //TODO: check if need render
        //Sleep((DWORD)lpParam);//in ms
        Sleep((DWORD)20);//in ms
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
           0,                   /* Extended possibilities for variation */
           szClassName,         /* Class name */
           _T("Code::Blocks Template Windows App"),       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           640,                 /* The window client area width */
           400,                 /* and height in pixels */
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
    displayManager->onCreate(hwnd);
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
struct RenderBufferDevice renderBuffer[2];//2 buffer for ping pang render/display

void createGlobalResource() {
    hBrushBG = CreateSolidBrush(RGB(16,16,16));
    hBrushFG = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
}
void destroyGlobalResource() {
	DeleteObject(hBrushBG);
	DeleteObject(hBrushFG);
    hBrushBG = NULL;
    hBrushFG = NULL;
    displayManager->onDestroy();

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

    const char *systemBusName[]={"?","8","16","?","32","?","?","?","64"};
    const char *ptr=(const char *)systemBusName[sizeof(void *)];
    printf("sizeof(void *)=%d, is %s bits system, ptr=%p, =%llx\n",
        sizeof(void *), ptr, ptr, (unsigned long long)ptr);
    size_t mallocSize=3UL<<30;
    ptr=(const char *)malloc(mallocSize);

    if(ptr) {
        void *ptr_end=(void *)ptr+mallocSize-1;
        unsigned long long bus_end=(unsigned long long)ptr_end;
        printf("malloc() ptr=%p, ptr_end=%p\n", ptr, ptr_end);
        printf("Bus address of ptr_end is 0x%llx, hi=0x%lx, low=0x%lx\n",
            bus_end, (unsigned long)(bus_end>>32), (unsigned long)bus_end);
        unsigned long bus_hi=(unsigned long)((unsigned long long)ptr>>32);
        unsigned long bus_lo=(unsigned long)((unsigned long long)ptr);
        printf("Bus address of ptr is 0x%llx, hi=0x%lx, low=0x%lx\n",
            (unsigned long long)ptr, bus_hi, bus_lo);

        unsigned long long bus_ptr=((unsigned long long)bus_hi<<32) + bus_lo;
        free((void *)bus_ptr);
        printf("freed memory of %d GB\n", mallocSize>>30);
    }
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
		RoundRect(hdc, clientWidth/4, cliedisplayManagerntHeight/4, 3*clientWidth/4, 3*clientHeight/4, clientWidth/4, clientHeight/4);
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
char keyDown[0x1000];
int capsLock=0, shiftDown=0, altDown=0, ctrlDown=0;
enum keyboardScanCode {
    SCAN_SPACE=0x39,
    SCAN_CAPSLOCK=0x3a,
    SCAN_LALT=0xFF0, SCAN_RALT=0xFF1,
    SCAN_LCTRL=0x1d, SCAN_RCTRL=0x11d,
    SCAN_LSHIFT=0x2a, SCAN_RSHIFT=0x36,
    SCAN_WIN=0x15b, SCAN_ATTR=0x15d,
    //top line
    SCAN_ESC=0x1,
    SCAN_F1=0x3b,SCAN_F2=0x3c,SCAN_F3=0x3d,SCAN_F4=0x3e,
    SCAN_F5=0x3f,SCAN_F6=0x40,SCAN_F7=0x41,SCAN_F8=0x42,
    SCAN_F9=0x43,SCAN_F10_UNKNOWN=0xFF2,SCAN_F11=0x57,SCAN_F12=0x58,
    //control area
    SCAN_PRTSC=0x137,SCAN_SCRLK=0x46,SCAN_PAUSE=0x45,
    SCAN_INS=0x152,SCAN_DEL=0x153,
    SCAN_HOME=0x147,SCAN_END=0x14f,SCAN_PAGEUP=0x149, SCAN_PAGEDOWN=0x151,
    SCAN_LEFT=0x14b,SCAN_RIGHT=0x14d,SCAN_UP=0x148,SCAN_DOWN=0x150,
    //keyboard numeric line
    SCAN_WAVE=0X29,
    SCAN_N1=2,SCAN_N2=3,SCAN_N3=4,SCAN_N4=5,SCAN_N5=6,SCAN_N6=7,SCAN_N7=8,
    SCAN_N8=9,SCAN_N9=10,SCAN_N0=11,SCAN_MINUS=12,SCAN_EQU=13,SCAN_BACK=0x0e,
    SCAN_TAB=0x0f,
    SCAN_Q=0x10,SCAN_W=0x11,SCAN_E=0x12,SCAN_R=0x13,SCAN_T=0x14,SCAN_Y=0x15,SCAN_U=0x16,SCAN_I=0x17,SCAN_O=0x18,SCAN_P=0x19,SCAN_LB=0x1A,SCAN_RB=0x1B,SCAN_VLINE=0x2b,
    SCAN_A=0x1e,SCAN_S=0x1F,SCAN_D=0x20,SCAN_F=0x21,SCAN_G=0x22,SCAN_H=0x23,SCAN_J=0x24,SCAN_K=0x25,SCAN_L=0x26,SCAN_SEMI=0x27,SCAN_STRING=0x28,SCAN_ENTER=0x1c,
    SCAN_Z=0x2c,SCAN_X=0x2d,SCAN_C=0x2e,SCAN_V=0x2f,SCAN_B=0x30,SCAN_N=0x31,SCAN_M=0x32,SCAN_COMMA=0x33,SCAN_DOT=0x34,SCAN_ASK=0x35,
};
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WORD scanCode;
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

        case WM_KEYDOWN:
            //check LOWORD(lParam) == 1
            //check key scan code = HIWORD(lParam) & 0xFFF
            scanCode=HIWORD(lParam) & 0xFFF;
            keyDown[scanCode] = 1;
            if(scanCode==SCAN_LALT   || scanCode==SCAN_RALT  ) altDown=1;
            if(scanCode==SCAN_LCTRL  || scanCode==SCAN_RCTRL ) ctrlDown=1;
            if(scanCode==SCAN_LSHIFT || scanCode==SCAN_RSHIFT) shiftDown=1;
            printf("WM_KEYDOWN 0x%03X 0x%x 0x%x\n",message, LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_KEYUP:
            scanCode=HIWORD(lParam) & 0xFFF;
            keyDown[scanCode] = 0;
            if(scanCode==SCAN_CAPSLOCK) capsLock=1-capsLock;
            if((scanCode==SCAN_LALT || scanCode==SCAN_RALT) && keyDown[SCAN_LALT]==0 && keyDown[SCAN_RALT]==0) altDown=0;
            if((scanCode==SCAN_LCTRL || scanCode==SCAN_RCTRL) && keyDown[SCAN_LCTRL]==0 && keyDown[SCAN_RCTRL]==0) ctrlDown=0;
            if((scanCode==SCAN_LSHIFT || scanCode==SCAN_RSHIFT) && keyDown[SCAN_LSHIFT]==0 && keyDown[SCAN_RSHIFT]==0) shiftDown=0;
            printf("WM_KEYUP   0x%03X 0x%x 0x%x\n",message, LOWORD(lParam), HIWORD(lParam));
            break;

        //mouse event
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            ptBegin.x = ptMove.x = LOWORD(lParam);
            ptBegin.y = ptMove.y = HIWORD(lParam);
            guiDirty++;
            printf("Here is a Mouse LBDn message 0x%03X %d %d\n",message, ptBegin.x, ptBegin.y);
            break;
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
