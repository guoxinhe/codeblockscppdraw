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

#define exInc(pLong) InterlockedIncrement((volatile LONG *)pLong)            //++
#define exDec(pLong) InterlockedDecrement((volatile LONG *)pLong)            //--
#define exAdd(pLong,val) InterlockedExchangeAdd((volatile LONG *)pLong, val) //+=
#define exSet(pLong,val) InterlockedExchange((volatile LONG *)pLong, val)    //=
#define exMod(pLong,to,out) InterlockedCompareExchange((volatile LONG *)pLong, to, out)

#define PI      (3.14159)
#define TWOPI   (2*3.14159)
#define TWO_PI  TWOPI

static HDC hdcRegisted=NULL;
static     HDC renderDC;
static     HBITMAP renderBmp;
static const DWORD renderWidth=256, renderHeight=192;
static HBRUSH hBrushTmp;
static RECT renderRect;
static void initLocal(void) {
    HDC hdc = hdcRegisted;
    renderDC = CreateCompatibleDC(hdc);   // 创建兼容DC
    renderBmp = CreateCompatibleBitmap(hdc, renderWidth, renderHeight);
    SelectObject(renderDC, renderBmp);    // 选入
    hBrushTmp=CreateSolidBrush(RGB(0, 0, 128));
    SetRect(&renderRect, 0, 0, renderWidth, renderHeight);
    FillRect(renderDC, &renderRect, hBrushTmp);
}

static int plugina_open(int mode) {
    return 0;
}
static int plugina_close(void) {
    DeleteObject(hBrushTmp);
    DeleteDC(renderDC);
    return 0;
}
static int plugina_user(int msg, DWORD wParam, void *lParam) {
    if(msg==0)
        initLocal();
    return 0;
}
static int plugina_render(void *target) {
    return 0;
}
/**           .<---------7
 *    4 .     0-------> 1  ^
 *    |  ^              |  |
 *    |  |              |  |
 *    |  |              v  |
 *    |  3  <----------2   6
 *    v     5------------->
 */
static int walkAlong=0;
static int walkLeft=0, walkTop=0;
static void walkNextPosition(int winW, int winH) {
    //change x
    int requireNextWalk=0;
    if(walkAlong==0 || walkAlong==5) {
        if(walkLeft<winW)
            walkLeft++;
        else
            requireNextWalk=walkAlong+1;
    }
    if(walkAlong==2 || walkAlong==7) {
        if(walkLeft>0)
            walkLeft--;
        else
            requireNextWalk=walkAlong+1;
    }
    if(walkAlong==1 || walkAlong==4) {
        if(walkTop<winH)
            walkTop++;
        else
            requireNextWalk=walkAlong+1;
    }
    if(walkAlong==3 || walkAlong==6) {
        if(walkTop>0)
            walkTop--;
        else
            requireNextWalk=walkAlong+1;
    }
    //adjust if window size become smaller
    if(walkLeft>winW) walkLeft=winW;
    if(walkTop>winH) walkTop=winH;

    if(requireNextWalk!=0)
        walkAlong=(walkAlong+1)&7;
}

static int plugina_display(void *target, int winW, int winH) {
    int blockW=winW/2;
    int blockH=winH/2;
    if(blockW>renderWidth) blockW = renderWidth;
    if(blockH>renderHeight) blockH = renderHeight;

    HDC hdc=(HDC)target;
    BitBlt(hdc, walkLeft, walkTop, blockW, blockH, renderDC, 0, 0, SRCCOPY);
    char title[256];
    RECT rect;
    sprintf(title, "  Client %lu x %lu  ",winW, winH);
    SetRect(&rect, walkLeft, walkTop, walkLeft+blockW, walkTop+blockH);
    DrawText(hdc, title, -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
    sprintf(title, "  Block %lu x %lu  ",blockW, blockH);
    SetRect(&rect, walkLeft, walkTop+32, walkLeft+blockW, walkTop+blockH);
    DrawText(hdc, title, -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
    sprintf(title, "  Walk %lu x %lu  ",walkLeft, walkTop);
    SetRect(&rect, walkLeft, walkTop+64, walkLeft+blockW, walkTop+blockH);
    DrawText(hdc, title, -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
    walkNextPosition(winW-blockW, winH-blockH);
    return 0;
}
static int plugina_registerObj(int category, void *obj) {
    if(category==0) {
        hdcRegisted=(HDC)obj;
    }
    return 0;
}

struct pluginObject plugina= {
    0,0,
    NULL,NULL,
    plugina_open,
    plugina_close,
    plugina_user,
    plugina_render,
    plugina_display,
    plugina_registerObj,
};
