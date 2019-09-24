#ifndef __MATRIC_H__
#define __MATRIC_H__

//#if defined(__cplusplus)
	//extern "C" {
//#endif

//default to 4x4, row=col set to 1, else set to 0
//type: 0: int, 1: float, 2: double
#define MATRIC_MAXROW 4
#define MATRIC_MAXCOL 4
#define CHECK_VALID(ex) do{if(!(ex)){\
    printf("Invalid at source code %s:%d\n",__FILE__,__LINE__);}}while(0)

typedef struct  {
    int row, col, draw, rsvd;
    float ma[MATRIC_MAXROW][MATRIC_MAXCOL];
}Matric;
#define CHECK_VALID_MATRIC(mat) \
    CHECK_VALID(mat!=NULL && mat->row>0 && mat->row<=MATRIC_MAXROW \
                          && mat->col>0 && mat->col<=MATRIC_MAXCOL)

typedef struct  {
    int row, col, type, draw;
    float ma[1024][MATRIC_MAXCOL];
}Shapeva;
#define shapeSetVertex(sva, row, x,y,z) do{\
    sva->ma[row][0]=(x);sva->ma[row][1]=(y);sva->ma[row][2]=(z);}while(0)

void matricSetInitZero(Matric *mat, int row, int col);
void matricSetInitUnit(Matric *mat, int row, int col);
void matricSetZero(Matric *mat);
void matricSetUnit(Matric *mat);
void matricSetCopy(Matric *mat, Matric *matsrc);
void matricSetElement(Matric *mat, int row, int col, float val);
void matricSetFromArray(Matric *mat, float *ar);
void matricSetShift(Matric *mat, float x, float y, float z);
void matricSetRotatex(Matric *mat, int angleOf360);
void matricSetRotatey(Matric *mat, int angleOf360);
void matricSetRotatez(Matric *mat, int angleOf360);
void matricSetScale(Matric *mat, float untiRatio);
void matricSetScaleDim(Matric *mat, float rx, float ry, float rz);
void matricSetScrewXy(Matric *mat, float scr);
void matricSetScrewXz(Matric *mat, float scr);
void matricSetScrewYx(Matric *mat, float scr);
void matricSetScrewYz(Matric *mat, float scr);
void matricSetScrewZx(Matric *mat, float scr);
void matricSetScrewZy(Matric *mat, float scr);
void matricSetMirrorXZ(Matric *mat);
void matricSetMirrorXY(Matric *mat);
void matricSetMirrorYZ(Matric *mat);
void matricSetProjectXZ(Matric *mat);
void matricSetProjectXY(Matric *mat);
void matricSetProjectYZ(Matric *mat);
void matricCeqAxB(Matric *c, Matric *a, Matric *b);
void matricDump(Matric *mat);

void matridShift(Matric *mat, float x, float y, float z);
void matridRotate(Matric *mat, int angx, int angy, int angz);
void matridScale(Matric *mat, float rx, float ry, float rz);

void shapeTransCeqAxB(Shapeva *c, Shapeva *a, Matric *b);
void shapeClone(Shapeva *dst, Shapeva *src);
void shapeDump(Shapeva *sva);
void shapeCreatePreset(Shapeva *sva, int preModel);

//#if defined(__cplusplus)
	//}
//#endif

#endif // __MATRIC_H__
