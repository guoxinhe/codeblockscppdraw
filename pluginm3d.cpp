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
#include "metric.h"

#define PI      (3.14159)
#define TWOPI   (2*3.14159)
#define TWO_PI  TWOPI

int rotateX=0, rotateY=0, rotateZ=0;
int shiftX=200, shiftY=160, shiftZ=0;
Shapeva finalModel; //crated but not transfered
Shapeva finalShape; //crated but not transfered
Shapeva finalCamera;


static HDC hdcRegisted=NULL;
static     HDC renderDC;
static     HBITMAP renderBmp;
static const DWORD renderWidth=400, renderHeight=400;
static HBRUSH hBrushBG, hBrushFG;
static HPEN hNullPen;
static RECT renderRect;
static COLORREF colorBG=RGB(0, 64, 64), colorFG=RGB(128, 128, 128);
static void initLocal(void) {
    Matric localMat, *mat=&localMat;
    Shapeva *shape = &finalModel;

    shapeCreatePreset(shape, 1);

    HDC hdc = hdcRegisted;
    renderDC = CreateCompatibleDC(hdc);
    renderBmp = CreateCompatibleBitmap(hdc, renderWidth, renderHeight);
    SelectObject(renderDC, renderBmp);
    hBrushBG=CreateSolidBrush(colorBG);
    hBrushFG=CreateSolidBrush(colorFG);
    SetRect(&renderRect, 0, 0, renderWidth, renderHeight);
    FillRect(renderDC, &renderRect, hBrushBG);
    hNullPen = CreatePen(PS_NULL, 1, 0); //for not draw border
}

static int plugina_open(int mode) {
    return 0;
}
static int plugina_close(void) {
    DeleteObject(hBrushBG);
    DeleteObject(hBrushFG);
    DeleteObject(hNullPen);
    DeleteDC(renderDC);
    return 0;
}
static int plugina_user(int msg, DWORD wParam, void *lParam) {
    if(msg==0)
        initLocal();
    if(msg==1) {//auto daemon
        rotateX = (rotateX+1)%360;
        rotateY = (rotateY+1)%360;
        rotateZ = (rotateY+1)%360;
    }
    return 0;
}
static int plugina_render(void *target) {
    HDC hdc=(HDC)target;
    if(hdc==NULL) {
        hdc=renderDC;
    }

    if(hdc==renderDC) {
        SetRect(&renderRect, 0, 0, renderWidth, renderHeight);
        FillRect(hdc, &renderRect, hBrushBG);
    }

    //rotateX = (rotateX+1)%360;
    //rotateY = (rotateY+1)%360;
    //rotateZ = (rotateY+1)%360;

    Matric localMat, *mat=&localMat;
    matricSetInitUnit(mat,4,4);

    //adjust model to center if its center is not at (0,0,0)
    matricSetShift(mat, -0.5f, -0.5f, -0.5f);
    shapeTransCeqAxB(&finalShape, &finalModel, mat);

    //scale, and rotate model to final shape, and put to final position
    matricSetUnit(mat);
    matridScale(mat, 100, 100, 100);
    matridRotate(mat, rotateX, rotateY, rotateZ);
    matridShift(mat, shiftX, shiftY, shiftZ);
    shapeTransCeqAxB(&finalCamera, &finalShape, mat);

    Shapeva *sp = &finalCamera;
            /*   model 1: 1x1x1            model 2: 2x2x2
                      0 .------------. 1      1 .------------. 5
                       /.           /|         /.           /|
                      / .          / |        / .          / |
                   3 .--+---------. 2|     3 .--+---------. 7|
                     |  .         |  |       |  .         |  |
                     |4 .---------+--. 5     |0 .---------+--. 4
                     | /          | /        | /          | /
                     |/           |/         |/           |/
                   7 .------------. 6      2 .------------. 6
            */

    #define shapeLineDraw(hdc, sp, v0, v1) \
        MoveToEx(hdc, sp->ma[v0][0], sp->ma[v0][1], NULL);\
        LineTo(hdc, sp->ma[v1][0], sp->ma[v1][1])
    if(sp->draw==0) {
        shapeLineDraw(hdc, sp, 0, 2);
        shapeLineDraw(hdc, sp, 2, 3);
        shapeLineDraw(hdc, sp, 3, 1);
        shapeLineDraw(hdc, sp, 1, 0);
        shapeLineDraw(hdc, sp, 0, 4);
        shapeLineDraw(hdc, sp, 4, 6);
        shapeLineDraw(hdc, sp, 6, 7);
        shapeLineDraw(hdc, sp, 7, 5);
        shapeLineDraw(hdc, sp, 5, 4);
        shapeLineDraw(hdc, sp, 2, 6);
        shapeLineDraw(hdc, sp, 3, 7);
        shapeLineDraw(hdc, sp, 1, 5);
    } else if(sp->draw==1) {
        shapeLineDraw(hdc, sp, 4, 7);
        shapeLineDraw(hdc, sp, 7, 3);
        shapeLineDraw(hdc, sp, 3, 0);
        shapeLineDraw(hdc, sp, 0, 4);
        shapeLineDraw(hdc, sp, 4, 5);
        shapeLineDraw(hdc, sp, 5, 6);
        shapeLineDraw(hdc, sp, 6, 2);
        shapeLineDraw(hdc, sp, 2, 1);
        shapeLineDraw(hdc, sp, 1, 5);
        shapeLineDraw(hdc, sp, 7, 6);
        shapeLineDraw(hdc, sp, 3, 2);
        shapeLineDraw(hdc, sp, 0, 1);
    } else {
        int i;
        MoveToEx(hdc, sp->ma[0][0], sp->ma[0][1], NULL);
        for(i=0;i<sp->row;i++) {
            LineTo(hdc, sp->ma[i][0], sp->ma[i][1]);
        }
        LineTo(hdc, sp->ma[0][0], sp->ma[0][1]);
    }
    return 0;
}
static int plugina_display(void *target, int winW, int winH) {
    HDC hdc=(HDC)target;
    //BitBlt(hdc, 0, 0, renderWidth, renderHeight, renderDC, 0, 0, SRCCOPY);
    plugina_render(hdc);
    return 0;
}
static int plugina_registerObj(int category, void *obj) {
    if(category==0) {
        hdcRegisted=(HDC)obj;
    }
    return 0;
}

struct pluginObject pluginm3d= {
    0,0,
    NULL,NULL,
    plugina_open,
    plugina_close,
    plugina_user,
    plugina_render,
    plugina_display,
    plugina_registerObj,
};
