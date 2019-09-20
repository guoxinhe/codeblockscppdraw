
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//default to 4x4, row=col set to 1, else set to 0
//type: 0: int, 1: float, 2: double
#define MATRIC_INTEGER 0
#define MATRIC_FLOAT   1
#define MATRIC_DOUBLE  2
#define MATRIC_DEFAULT 1
#define MATRIC_MAXROW 4
#define MATRIC_MAXCOL 4
#define Matric Matrif

typedef struct  {
    int row, col, type, flags;
    float ma[MATRIC_MAXROW][MATRIC_MAXCOL];
}Matrif;
typedef struct  {
    int row, col, type, flags;
    float ma[1024][MATRIC_MAXCOL];
}Shapeva;

#define CHECK_VALID(ex) do{if(!(ex)){printf("Invalid at source code %s:%d\n",__FILE__,__LINE__);}}while(0)
#define CHECK_VALID_MATRIC(mat) \
    CHECK_VALID(mat!=NULL && mat->row>0 && mat->row<=MATRIC_MAXROW && mat->col>0 &&mat->col<=MATRIC_MAXCOL)

void matriSetInitZero(Matric *mat, int row, int col) {
    CHECK_VALID(mat!=NULL);
    CHECK_VALID(row>0 && row<=MATRIC_MAXROW && col>0 && col<=MATRIC_MAXCOL);
    memset(mat, 0, sizeof(*mat));
    mat->row=row;
    mat->col=col;
}
void matriSetInitUnit(Matric *mat, int row, int col) {
    CHECK_VALID(mat!=NULL);
    CHECK_VALID(row>0 && row<=MATRIC_MAXROW && col>0 && col<=MATRIC_MAXCOL);
    memset(mat, 0, sizeof(*mat));
    mat->row=row;
    mat->col=col;
    int minVal = ((row<col)?row:col);
    for(row=0;row<minVal;row++) {
        mat->ma[row][row]=1.0f;
    }
}
void matriSetZero(Matric *mat) {
    CHECK_VALID_MATRIC(mat);
    memset(mat->ma, 0, sizeof(mat->ma));
}
void matriSetUnit(Matric *mat) {
    int row, col;
    CHECK_VALID_MATRIC(mat);
    memset(mat->ma, 0, sizeof(mat->ma));
    row=mat->row;
    col=mat->col;
    int minVal = ((row<col)?row:col);
    for(row=0;row<minVal;row++) {
        mat->ma[row][row]=1.0f;
    }
}
void matriSetCopy(Matric *mat, Matric *matsrc) {
    CHECK_VALID_MATRIC(matsrc);
    CHECK_VALID(NULL!=mat);
    memcpy(mat, matsrc, sizeof(*mat));
}
void matriSetElement(Matric *mat, int row, int col, float val) {
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(row>=0 && row<mat->row && col>=0 && col<mat->col);
    mat->ma[row][col]=val;
}
void matriSetFromArray(Matric *mat, float *ar) {
    int row, col;
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(ar!=NULL);
    //memset(mat->ma, 0, sizeof(mat->ma));
    for(row=0;row<mat->row;row++) {
        for(col=0;col<mat->col;col++) {
            mat->ma[row][col]=ar[row*mat->col + col];
        }
    }
}
void matriSetShift(Matric *mat, int x, int y, int z) {
    matriSetUnit(mat);
    mat->ma[3][0]=x;
    mat->ma[3][1]=y;
    mat->ma[3][2]=z;
}
void matriSetRotatex(Matric *mat, int angleOf360) {
    matriSetUnit(mat);
    float angle=M_PI*2*angleOf360/360;
    mat->ma[1][1]=cos(angle);
    mat->ma[2][2]=mat->ma[1][1];
    mat->ma[1][2]=sin(angle);
    mat->ma[2][1]=-mat->ma[1][2];
}
void matriSetRotatey(Matric *mat, int angleOf360) {
    matriSetUnit(mat);
    float angle=M_PI*2*angleOf360/360;
    mat->ma[0][0]=cos(angle);
    mat->ma[2][2]=mat->ma[0][0];
    mat->ma[2][0]=sin(angle);
    mat->ma[0][2]=-mat->ma[2][0];
}
void matriSetRotatez(Matric *mat, int angleOf360) {
    matriSetUnit(mat);
    float angle=M_PI*2*angleOf360/360;
    mat->ma[0][0]=cos(angle);
    mat->ma[1][1]=mat->ma[0][0];
    mat->ma[0][1]=sin(angle);
    mat->ma[1][0]=-mat->ma[0][1];
}
void matriSetScale(Matric *mat, float untiRatio) {
    matriSetUnit(mat);
    mat->ma[3][3]=untiRatio;
}
void matriSetScaleDim(Matric *mat, float rx, float ry, float rz) {
    matriSetUnit(mat);
    mat->ma[0][0]=rx;
    mat->ma[1][1]=ry;
    mat->ma[2][2]=rz;
}
void matriSetScrewXy(Matric *mat, float scr) {
    matriSetUnit(mat);
    mat->ma[1][0]=scr;
}
void matriSetScrewXz(Matric *mat, float scr) {
    matriSetUnit(mat);
    mat->ma[2][0]=scr;
}
void matriSetScrewYx(Matric *mat, float scr) {
    matriSetUnit(mat);
    mat->ma[0][1]=scr;
}
void matriSetScrewYz(Matric *mat, float scr) {
    matriSetUnit(mat);
    mat->ma[2][1]=scr;
}
void matriSetScrewZx(Matric *mat, float scr) {
    matriSetUnit(mat);
    mat->ma[0][2]=scr;
}
void matriSetScrewZy(Matric *mat, float scr) {
    matriSetUnit(mat);
    mat->ma[1][2]=scr;
}
void matriSetMirrorXZ(Matric *mat) {
    matriSetUnit(mat);
    mat->ma[1][1]=-1.0f;
}
void matriSetMirrorXY(Matric *mat) {
    matriSetUnit(mat);
    mat->ma[2][2]=-1.0f;
}
void matriSetMirrorYZ(Matric *mat) {
    matriSetUnit(mat);
    mat->ma[0][0]=-1.0f;
}
void matriSetProjectXZ(Matric *mat) {
    matriSetUnit(mat);
    mat->ma[1][1]=0.0f;
}
void matriSetProjectXY(Matric *mat) {
    matriSetUnit(mat);
    mat->ma[2][2]=0.0f;
}
void matriSetProjectYZ(Matric *mat) {
    matriSetUnit(mat);
    mat->ma[0][0]=0.0f;
}
void matriCeqAxB(Matric *c, Matric *a, Matric *b) {
    CHECK_VALID(c!=NULL);
    CHECK_VALID_MATRIC(a);
    CHECK_VALID_MATRIC(b);
    CHECK_VALID(a->col==b->row);
    matriSetZero(c);
    c->row=a->row;
    c->col=b->col;

    int row, col, rowCol;
    float cRC;
    for(row=0;row<c->row;row++) {
        for(col=0;col<c->col;col++) {
            cRC=0;
            for(rowCol=0;rowCol<a->col;rowCol++) {
                cRC+=a->ma[row][rowCol]*b->ma[rowCol][col];
            }
            c->ma[row][col]=cRC;
        }
    }
}
void matriDump(Matric *mat) {
    int row, col;
    CHECK_VALID_MATRIC(mat);
    printf("Dump matrix row x col: %d x %d ={\n", mat->row, mat->col);
    for(row=0;row<mat->row;row++) {
        for(col=0;col<mat->col;col++) {
            printf("    %4.4f,", mat->ma[row][col]);
        }
        printf("\n");
    }
    printf("}\n");
}
void matridShift(Matric *mat, int x, int y, int z) {
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(4==mat->row && 4==mat->col);
    Matric a, b;
    matriSetInitZero(&b, mat->row, mat->col);

    matriSetCopy(&a, mat);
    matriSetShift(&b, x, y, z);
    matriCeqAxB(mat, &a, &b);
}
void matridRotate(Matric *mat, int angx, int angy, int angz) {
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(4==mat->row && 4==mat->col);
    Matric a, b;
    matriSetInitZero(&b, mat->row, mat->col);

    if(angx!=0) {
        matriSetCopy(&a, mat);
        matriSetRotatex(&b, angx);
        matriCeqAxB(mat, &a, &b);
    }
    if(angy!=0) {
        matriSetCopy(&a, mat);
        matriSetRotatey(&b, angy);
        matriCeqAxB(mat, &a, &b);
    }
    if(angz!=0) {
        matriSetCopy(&a, mat);
        matriSetRotatez(&b, angz);
        matriCeqAxB(mat, &a, &b);
    }
}

void shapeTransCeqAxB(Shapeva *c, Shapeva *a, Matric *b) {
    CHECK_VALID(c!=NULL);
    CHECK_VALID(a!=NULL);
    CHECK_VALID_MATRIC(b);
    CHECK_VALID(a->col==b->row);
    memset(c, 0, sizeof(*c));
    c->row=a->row;
    c->col=b->col;

    int row, col, rowCol;
    float cRC;
    for(row=0;row<c->row;row++) {
        for(col=0;col<c->col;col++) {
            cRC=0;
            for(rowCol=0;rowCol<a->col;rowCol++) {
                cRC+=a->ma[row][rowCol]*b->ma[rowCol][col];
            }
            c->ma[row][col]=cRC;
        }
    }
}
void shapeDump(Shapeva *sva) {
    int row, col;
    CHECK_VALID(NULL!=sva);
    printf("Dump shape row x col: %d x %d ={\n", sva->row, sva->col);
    for(row=0;row<sva->row;row++) {
        for(col=0;col<sva->col;col++) {
            printf("    %4.4f,", sva->ma[row][col]);
        }
        printf("\n");
    }
    printf("}\n");
}
void shapeCreatePreset( Shapeva *sva, int preModel) {
    if(preModel!=0)
        return ;


    memset(sva, 0, sizeof(*sva));
    sva->row=1024;
    sva->col=4;
    int row;
    for(row=0;row<sva->row;row++) {
        sva->ma[row][3]=1.0f;
    }
    float x,y,z,r,angle;
    for(row=0;row<sva->row;row++) {
        y = 16.0f - row*1.0f/32.0f;
        r = row*1.0f/32.0f;
        angle= M_PI*2*(row%32)/32.0f;
        x = r * cos(angle);
        z = r * sin(angle);
        sva->ma[row][0]=x;
        sva->ma[row][1]=y;
        sva->ma[row][2]=z;
    }
};

Shapeva finalModel; //crated but not transfered
Shapeva finalShape; //transfered but not project
Shapeva finalCamera, finalFront, finalLeft, finalTop; //4 project view

Matric projFront, projLeft, projTop, projCamera;
void initProject(void) {
    matriSetInitUnit(&projCamera, 4, 4);
    matriSetInitUnit(&projFront, 4, 4);
    matriSetInitUnit(&projLeft, 4, 4);
    matriSetInitUnit(&projTop, 4, 4);

    matriSetProjectXY(&projFront);
    matriSetProjectYZ(&projLeft);
    matriSetProjectXZ(&projTop);

    Matric tmpCamera;
    matriSetInitUnit(&tmpCamera, 4, 4);
    matridRotate(&tmpCamera, 30, 45, 0);
    matriCeqAxB(&projCamera, &tmpCamera, &projFront);

    shapeTransCeqAxB(&finalCamera, &finalShape, &projCamera);
    shapeTransCeqAxB(&finalFront, &finalShape, &projFront);
    shapeTransCeqAxB(&finalLeft, &finalShape, &projLeft);
    shapeTransCeqAxB(&finalTop, &finalShape, &projTop);

    printf("Show shape finalModel \n"); shapeDump(&finalModel );
    printf("Show shape finalShape \n"); shapeDump(&finalShape );
    printf("Show shape finalFront \n"); shapeDump(&finalFront );
    printf("Show shape finalLeft  \n"); shapeDump(&finalLeft  );
    printf("Show shape finalTop   \n"); shapeDump(&finalTop   );
    printf("Show shape finalCamera\n"); shapeDump(&finalCamera);
}

int matridTest(void) {
    Matric localMat, *mat=&localMat;
    Shapeva *shape = &finalModel;
    matriSetInitUnit(mat, 4, 4);
    printf("Show unit\n");
    matriDump(mat);

    printf("Show shift\n");
    matriSetUnit(mat);
    matridShift(mat, 10, 20, 30);
    matriDump(mat);

    printf("Show rotate x\n");
    matriSetUnit(mat);
    matridRotate(mat, 45, 0, 0);
    matriDump(mat);

    printf("Show rotate y\n");
    matriSetUnit(mat);
    matridRotate(mat, 0, 45, 0);
    matriDump(mat);

    printf("Show rotate z\n");
    matriSetUnit(mat);
    matridRotate(mat, 0, 0, 45);
    matriDump(mat);

    printf("Show rotate x, y, z\n");
    matriSetUnit(mat);
    matridRotate(mat, 45, 60, 75);
    matriDump(mat);

    printf("Show shift, rotate x, y, z, shit\n");
    matriSetUnit(mat);
    matridShift(mat, -10, -20, -30);
    matridRotate(mat, 45, 60, 75);
    matridShift(mat, 10, 20, 30);
    matriDump(mat);

    shapeCreatePreset(shape, 0);
    //shapeDump(shape);

    matriSetUnit(mat);
    matridRotate(mat, 0, 0, 90);
    shapeTransCeqAxB(&finalShape, shape, mat);
    //shapeDump(&finalShape);

    initProject();

    return 0;
}
