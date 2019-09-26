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

float rotateX=0, rotateY=0, rotateZ=0;
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
static Matric shapeAdjust;
static void initLocal(void) {
    Shapeva *shape = &finalModel;

    shapeCreatePreset(shape, 1);
    int i;
    Matric *mat=&shapeAdjust;
    memset(&shapeAdjust, 0, sizeof(shapeAdjust));
    for(i=0;i<shape->row;i++) {
        if(shape->ma[i][0]<mat->ma[0][0]) mat->ma[0][0]=shape->ma[i][0];
        if(shape->ma[i][1]<mat->ma[0][1]) mat->ma[0][1]=shape->ma[i][1];
        if(shape->ma[i][2]<mat->ma[0][2]) mat->ma[0][2]=shape->ma[i][2];

        if(shape->ma[i][0]>mat->ma[1][0]) mat->ma[1][0]=shape->ma[i][0];
        if(shape->ma[i][1]>mat->ma[1][1]) mat->ma[1][1]=shape->ma[i][1];
        if(shape->ma[i][2]>mat->ma[1][2]) mat->ma[1][2]=shape->ma[i][2];
    }
    //the proper center for scale and rotate
    mat->ma[2][0]=(mat->ma[0][0]+mat->ma[1][0])/2;
    mat->ma[2][1]=(mat->ma[0][1]+mat->ma[1][1])/2;
    mat->ma[2][2]=(mat->ma[0][2]+mat->ma[1][2])/2;

    //the size of 3 dimension
    mat->ma[3][0]=(mat->ma[1][0]-mat->ma[0][0]);
    mat->ma[3][1]=(mat->ma[1][1]-mat->ma[0][1]);
    mat->ma[3][2]=(mat->ma[1][2]-mat->ma[0][2]);
    //a proper scale ratio for render to 200x200 window
    float length;
    length=mat->ma[3][0];
    if(mat->ma[3][1]>length) length=mat->ma[3][1];
    if(mat->ma[3][2]>length) length=mat->ma[3][2];

    float minofWin=shiftX;
    if(minofWin<shiftY) minofWin=shiftY;
    mat->ma[3][3] = minofWin/length;

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
        rotateX = (rotateX+1);
        rotateY = (rotateY+0.719);
        rotateZ = (rotateY+0.53);
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

    Matric localMat, *mat=&localMat;
    matricSetInitUnit(mat,4,4);

    //adjust model to center if its center is not at (0,0,0)
    if(1) {
        matricSetShift(mat, -shapeAdjust.ma[2][0], -shapeAdjust.ma[2][1], -shapeAdjust.ma[2][2]);
        shapeTransCeqAxB(&finalShape, &finalModel, mat);
    } else {
        shapeClone(&finalShape, &finalModel);
    }

    //scale, and rotate model to final shape, and put to final position
    matricSetUnit(mat);
    matricSetMirrorXZ(mat); //y is upside down in display

    float length;
    length=shapeAdjust.ma[3][0];
    if(shapeAdjust.ma[3][1]>length) length=shapeAdjust.ma[3][1];
    if(shapeAdjust.ma[3][2]>length) length=shapeAdjust.ma[3][2];

    float minofWin=shiftX;
    if(minofWin>shiftY) minofWin=shiftY;
    float adjRatio = minofWin/length;//shapeAdjust.ma[3][3]

    matridScale(mat, adjRatio, adjRatio, adjRatio);
    //matridRotate(mat, rotateX, rotateY, rotateZ);
    matridRotate(mat, 0, rotateY, 0);
    matridRotate(mat, 15,0,0);
    matridShift(mat, shiftX, shiftY, shiftZ);
    //matridAway(mat, 0, -0.58, 0);
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

    HPEN linePen = CreatePen(PS_SOLID,1,RGB(0,128,0)); //set draw line's color
    HPEN oldPen = (HPEN)SelectObject(hdc,linePen);
    #define shapeLineDraw(hdc, sp, v0, v1) \
        MoveToEx(hdc, sp->ma[v0][0], sp->ma[v0][1], NULL);\
        LineTo(hdc, sp->ma[v1][0], sp->ma[v1][1])
    int vid;
    if(sp->draw==0) {
        for(vid=0;vid<sp->row;vid+=8) {
            shapeLineDraw(hdc, sp, vid+0, vid+2);
            shapeLineDraw(hdc, sp, vid+2, vid+3);
            shapeLineDraw(hdc, sp, vid+3, vid+1);
            shapeLineDraw(hdc, sp, vid+1, vid+0);
            shapeLineDraw(hdc, sp, vid+0, vid+4);
            shapeLineDraw(hdc, sp, vid+4, vid+6);
            shapeLineDraw(hdc, sp, vid+6, vid+7);
            shapeLineDraw(hdc, sp, vid+7, vid+5);
            shapeLineDraw(hdc, sp, vid+5, vid+4);
            shapeLineDraw(hdc, sp, vid+2, vid+6);
            shapeLineDraw(hdc, sp, vid+3, vid+7);
            shapeLineDraw(hdc, sp, vid+1, vid+5);
        }
    } else if(sp->draw==1) {
        for(vid=0;vid<sp->row;vid+=8) {
            shapeLineDraw(hdc, sp, vid+4, vid+7);
            shapeLineDraw(hdc, sp, vid+7, vid+3);
            shapeLineDraw(hdc, sp, vid+3, vid+0);
            shapeLineDraw(hdc, sp, vid+0, vid+4);
            shapeLineDraw(hdc, sp, vid+4, vid+5);
            shapeLineDraw(hdc, sp, vid+5, vid+6);
            shapeLineDraw(hdc, sp, vid+6, vid+2);
            shapeLineDraw(hdc, sp, vid+2, vid+1);
            shapeLineDraw(hdc, sp, vid+1, vid+5);
            shapeLineDraw(hdc, sp, vid+7, vid+6);
            shapeLineDraw(hdc, sp, vid+3, vid+2);
            shapeLineDraw(hdc, sp, vid+0, vid+1);
        }
    } else {
        MoveToEx(hdc, sp->ma[0][0], sp->ma[0][1], NULL);
        for(vid=0;vid<sp->row;vid++) {
            LineTo(hdc, sp->ma[vid][0], sp->ma[vid][1]);
        }
        LineTo(hdc, sp->ma[0][0], sp->ma[0][1]);
    }


	HBRUSH newBrush = CreateSolidBrush(RGB(255,0,0));//set fill range color
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, newBrush);
	HBRUSH newBrush2 = CreateSolidBrush(RGB(0, 255, 255));
    POINT pt[4];
    pt[0].x=sp->ma[0][0];pt[0].y=sp->ma[0][1];
    pt[1].x=sp->ma[1][0];pt[1].y=sp->ma[1][1];
    pt[2].x=sp->ma[2][0];pt[2].y=sp->ma[2][1];
    pt[3].x=sp->ma[3][0];pt[3].y=sp->ma[3][1];
    HRGN hRgn = CreatePolygonRgn( pt, 4, WINDING );
    FillRgn( hdc, hRgn, newBrush2 );
    DeleteObject( hRgn );

    pt[0].x=sp->ma[8][0];pt[0].y=sp->ma[8][1];
    pt[1].x=sp->ma[9][0];pt[1].y=sp->ma[9][1];
    pt[2].x=sp->ma[10][0];pt[2].y=sp->ma[10][1];
    pt[3].x=sp->ma[11][0];pt[3].y=sp->ma[11][1];
    hRgn = CreatePolygonRgn( pt, 4, WINDING );
    PaintRgn( hdc, hRgn );

	//this group fill whole range with selected brush
	Rectangle(hdc,40,40,100,100);
	SelectObject(hdc, newBrush2);
	Ellipse(hdc,40,40,100,100);

	//this group direct use brush as parameter
	RECT r;
	SetRect (&r, 250, 250,400, 400);
	FrameRect(hdc, &r, newBrush2);//draw border only with color in brush
	SetRect (&r, 250+20, 250+20,400-20, 400-20);
	HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);//for not draw border
	HPEN redPen = CreatePen(PS_SOLID,1,RGB(255,0,0)); //set draw line's color
	HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);//for not fill center
	SelectObject(hdc, redPen); //for draw border
	SelectObject(hdc, newBrush2);//for fill center
	FillRect(hdc, &r, newBrush);
	Ellipse(hdc, 300, 300, 400, 400);


	SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
	DeleteObject(newBrush);
	DeleteObject(newBrush2);

    return 0;
}
static int plugina_display(void *target, int winW, int winH) {
    HDC hdc=(HDC)target;
    //BitBlt(hdc, 0, 0, renderWidth, renderHeight, renderDC, 0, 0, SRCCOPY);
    shiftX = winW/2;
    shiftY = winH/2;
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
