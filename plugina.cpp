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

#define PI      (3.14159)
#define TWOPI   (2*3.14159)
#define TWO_PI  TWOPI

static HDC hdcRegisted=NULL;
static     HDC renderDC;
static     HBITMAP renderBmp;
static const DWORD renderWidth=256, renderHeight=128;
static HBRUSH hBrushBG, hBrushFG, hBrushFontBG, hBrushFont,
    hBrushTitleBG, hBrushTitle, hBrushBorder, hBrushFill;
static HPEN hNullPen;
static RECT renderRect;
static COLORREF colorBG=RGB(0, 64, 64), colorFG=RGB(128, 128, 128),
    colorFontBG=RGB(64, 64, 64), colorFont=RGB(128, 128, 128),
    colorTitleBG=RGB(64, 64, 64), colorTitle=RGB(0, 128, 128),
    colorBorder=RGB(0, 200, 128), colorFill=RGB(16, 32, 32);
static void initLocal(void) {
    HDC hdc = hdcRegisted;
    renderDC = CreateCompatibleDC(hdc);
    renderBmp = CreateCompatibleBitmap(hdc, renderWidth, renderHeight);
    SelectObject(renderDC, renderBmp);
    hBrushBG=CreateSolidBrush(colorBG);
    hBrushFG=CreateSolidBrush(colorFG);
    hBrushFontBG=CreateSolidBrush(colorFontBG);
    hBrushFont=CreateSolidBrush(colorFont);
    hBrushTitleBG=CreateSolidBrush(colorTitleBG);
    hBrushTitle=CreateSolidBrush(colorTitle);
    hBrushBorder=CreateSolidBrush(colorBorder);
    hBrushFill=CreateSolidBrush(colorFill);
    hNullPen = CreatePen(PS_NULL, 1, 0); //for not draw border
    SetRect(&renderRect, 0, 0, renderWidth, renderHeight);
    FillRect(renderDC, &renderRect, hBrushBG);

    SetBkMode(hdc, TRANSPARENT);//text no BG color
    Rectangle(renderDC, 4, 4, renderWidth-4, renderHeight-4);

    HGDIOBJ hPenOld = SelectObject(renderDC, hNullPen);
    HBRUSH hBurshOld = (HBRUSH)SelectObject(renderDC, hBrushFill);
    Ellipse(renderDC, renderWidth/8, renderHeight/8, 7*renderWidth/8, 7*renderHeight/8);
    RoundRect(renderDC, renderWidth/4, renderHeight/4, 3*renderWidth/4, 3*renderHeight/4, renderWidth/4, renderHeight/4);
    SelectObject(renderDC, hPenOld);
    SelectObject(renderDC, hBurshOld);
}

static int plugina_open(int mode) {
    return 0;
}
static int plugina_close(void) {
    DeleteObject(hBrushBG);
    DeleteObject(hBrushFG);
    DeleteObject(hBrushFontBG);
    DeleteObject(hBrushFont);
    DeleteObject(hBrushTitleBG);
    DeleteObject(hBrushTitle);
    DeleteObject(hBrushBorder);
    DeleteObject(hBrushFill);
    DeleteObject(hNullPen);
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

    SetRect(&rect, walkLeft+4, walkTop+4, walkLeft+blockW-4, walkTop+32);
    FillRect(hdc, &rect, hBrushTitle);

    SetTextColor(hdc, RGB(0xFF,0xFF,0xFF));
    SetBkMode(hdc, TRANSPARENT);//text no BG color
    TextOut(hdc, rect.left, rect.top+4, title, strlen(title));

    SetBkMode(hdc, OPAQUE); //restore text BG color
    SetTextColor(hdc, RGB(0,0,0));//restore font color


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
