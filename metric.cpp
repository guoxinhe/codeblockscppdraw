
//#if defined(__cplusplus)
//	extern "C" {
//#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

void matricSetInitZero(Matric *mat, int row, int col) {
    CHECK_VALID(mat!=NULL);
    CHECK_VALID(row>0 && row<=MATRIC_MAXROW && col>0 && col<=MATRIC_MAXCOL);
    memset(mat, 0, sizeof(*mat));
    mat->row=row;
    mat->col=col;
}
void matricSetInitUnit(Matric *mat, int row, int col) {
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
void matricSetZero(Matric *mat) {
    CHECK_VALID_MATRIC(mat);
    memset(mat->ma, 0, sizeof(mat->ma));
}
void matricSetUnit(Matric *mat) {
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
void matricSetCopy(Matric *mat, Matric *matsrc) {
    CHECK_VALID_MATRIC(matsrc);
    CHECK_VALID(NULL!=mat);
    memcpy(mat, matsrc, sizeof(*mat));
}
void matricSetElement(Matric *mat, int row, int col, float val) {
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(row>=0 && row<mat->row && col>=0 && col<mat->col);
    mat->ma[row][col]=val;
}
void matricSetFromArray(Matric *mat, float *ar) {
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
void matricSetShift(Matric *mat, float x, float y, float z) {
    matricSetUnit(mat);
    mat->ma[3][0]=x;
    mat->ma[3][1]=y;
    mat->ma[3][2]=z;
}
void matricSetRotatex(Matric *mat, int angleOf360) {
    matricSetUnit(mat);
    float angle=M_PI*2*angleOf360/360;
    mat->ma[1][1]=cos(angle);
    mat->ma[2][2]=mat->ma[1][1];
    mat->ma[1][2]=sin(angle);
    mat->ma[2][1]=-mat->ma[1][2];
}
void matricSetRotatey(Matric *mat, int angleOf360) {
    matricSetUnit(mat);
    float angle=M_PI*2*angleOf360/360;
    mat->ma[0][0]=cos(angle);
    mat->ma[2][2]=mat->ma[0][0];
    mat->ma[2][0]=sin(angle);
    mat->ma[0][2]=-mat->ma[2][0];
}
void matricSetRotatez(Matric *mat, int angleOf360) {
    matricSetUnit(mat);
    float angle=M_PI*2*angleOf360/360;
    mat->ma[0][0]=cos(angle);
    mat->ma[1][1]=mat->ma[0][0];
    mat->ma[0][1]=sin(angle);
    mat->ma[1][0]=-mat->ma[0][1];
}
void matricSetScale(Matric *mat, float untiRatio) {
    matricSetUnit(mat);
    mat->ma[3][3]=untiRatio;
}
void matricSetScaleDim(Matric *mat, float rx, float ry, float rz) {
    matricSetUnit(mat);
    mat->ma[0][0]=rx;
    mat->ma[1][1]=ry;
    mat->ma[2][2]=rz;
}
void matricSetScrewXy(Matric *mat, float scr) {
    matricSetUnit(mat);
    mat->ma[1][0]=scr;
}
void matricSetScrewXz(Matric *mat, float scr) {
    matricSetUnit(mat);
    mat->ma[2][0]=scr;
}
void matricSetScrewYx(Matric *mat, float scr) {
    matricSetUnit(mat);
    mat->ma[0][1]=scr;
}
void matricSetScrewYz(Matric *mat, float scr) {
    matricSetUnit(mat);
    mat->ma[2][1]=scr;
}
void matricSetScrewZx(Matric *mat, float scr) {
    matricSetUnit(mat);
    mat->ma[0][2]=scr;
}
void matricSetScrewZy(Matric *mat, float scr) {
    matricSetUnit(mat);
    mat->ma[1][2]=scr;
}
void matricSetMirrorXZ(Matric *mat) {
    matricSetUnit(mat);
    mat->ma[1][1]=-1.0f;
}
void matricSetMirrorXY(Matric *mat) {
    matricSetUnit(mat);
    mat->ma[2][2]=-1.0f;
}
void matricSetMirrorYZ(Matric *mat) {
    matricSetUnit(mat);
    mat->ma[0][0]=-1.0f;
}
void matricSetProjectXZ(Matric *mat) {
    matricSetUnit(mat);
    mat->ma[1][1]=0.0f;
}
void matricSetProjectXY(Matric *mat) {
    matricSetUnit(mat);
    mat->ma[2][2]=0.0f;
}
void matricSetProjectYZ(Matric *mat) {
    matricSetUnit(mat);
    mat->ma[0][0]=0.0f;
}
void matricCeqAxB(Matric *c, Matric *a, Matric *b) {
    CHECK_VALID(c!=NULL);
    CHECK_VALID_MATRIC(a);
    CHECK_VALID_MATRIC(b);
    CHECK_VALID(a->col==b->row);
    matricSetZero(c);
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
void matricDump(Matric *mat) {
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

void matridShift(Matric *mat, float x, float y, float z) {
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(4==mat->row && 4==mat->col);
    Matric a, b;
    matricSetInitUnit(&b, mat->row, mat->col);

    matricSetCopy(&a, mat);
    matricSetShift(&b, x, y, z);
    matricCeqAxB(mat, &a, &b);
}
void matridRotate(Matric *mat, int angx, int angy, int angz) {
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(4==mat->row && 4==mat->col);
    Matric a, b;
    matricSetInitUnit(&b, mat->row, mat->col);

    if(angx!=0) {
        matricSetRotatex(&b, angx);
        matricSetCopy(&a, mat);
        matricCeqAxB(mat, &a, &b);
    }
    if(angy!=0) {
        matricSetRotatey(&b, angy);
        matricSetCopy(&a, mat);
        matricCeqAxB(mat, &a, &b);
    }
    if(angz!=0) {
        matricSetRotatez(&b, angz);
        matricSetCopy(&a, mat);
        matricCeqAxB(mat, &a, &b);
    }
}
void matridScale(Matric *mat, float rx, float ry, float rz) {
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(4==mat->row && 4==mat->col);
    Matric a, b;
    matricSetInitUnit(&b, mat->row, mat->col);
    //if(rx==ry && ry==rz && rz!=0.0f && rz!=1.0f)
    //    matricSetScale(&b, 1.0f/rz);
    //else
        matricSetScaleDim(&b, rx, ry, rz);
    matricSetCopy(&a, mat);
    matricCeqAxB(mat, &a, &b);
}
void shapeTransCeqAxB(Shapeva *dst, Shapeva *src, Matric *mat) {
    CHECK_VALID(dst!=NULL);
    CHECK_VALID(src!=NULL);
    CHECK_VALID_MATRIC(mat);
    CHECK_VALID(src->col==mat->row);
    memset(dst, 0, sizeof(*dst));
    dst->row=src->row;
    dst->col=mat->col;
    dst->draw=src->draw;

    int row, col, rowCol;
    float cRC;
    for(row=0;row<dst->row;row++) {
        for(col=0;col<dst->col;col++) {
            cRC=0;
            for(rowCol=0;rowCol<src->col;rowCol++) {
                cRC+=src->ma[row][rowCol]*mat->ma[rowCol][col];
            }
            dst->ma[row][col]=cRC;
        }
    }
}
void shapeClone(Shapeva *dst, Shapeva *src) {
    memcpy(dst, src, sizeof(*dst));
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
void shapeCreatePreset(Shapeva *sva, int preModel) {
    if(preModel==1||preModel==0) {
        memset(sva, 0, sizeof(*sva));
        sva->col=4;
        sva->row=8;
        sva->draw=preModel;
        int row;
        for(row=0;row<sva->row;row++) {
            sva->ma[row][3]=1.0f;
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
            if(preModel==1) {
                //unit is 1, cube at (0,0,0) to (1,1,1), center (0.5, 0.5,0.5)
                shapeSetVertex(sva, 0,  0,  1,  0);  //1
                shapeSetVertex(sva, 1,  1,  1,  0);  //5
                shapeSetVertex(sva, 2,  1,  1,  1);  //7
                shapeSetVertex(sva, 3,  0,  1,  1);  //3
                shapeSetVertex(sva, 4,  0,  0,  0);  //0
                shapeSetVertex(sva, 5,  1,  0,  0);  //4
                shapeSetVertex(sva, 6,  1,  0,  1);  //6
                shapeSetVertex(sva, 7,  0,  0,  1);  //2
                //a mini cube at (0,0,0) to (0.1,0.1,0.1)
                shapeSetVertex(sva, 8,    0,  0.1,    0);  //1
                shapeSetVertex(sva, 9,  0.1,  0.1,    0);  //5
                shapeSetVertex(sva,10,  0.1,  0.1,  0.1);  //7
                shapeSetVertex(sva,11,    0,  0.1,  0.1);  //3
                shapeSetVertex(sva,12,    0,    0,    0);  //0
                shapeSetVertex(sva,13,  0.1,    0,    0);  //4
                shapeSetVertex(sva,14,  0.1,    0,  0.1);  //6
                shapeSetVertex(sva,15,    0,    0,  0.1);  //2
                //a mini cube as front door
                shapeSetVertex(sva,16, 0.25, 0.50, 0.75);  //1
                shapeSetVertex(sva,17, 0.75, 0.50, 0.75);  //5
                shapeSetVertex(sva,18, 0.75, 0.50, 1   );  //7
                shapeSetVertex(sva,19, 0.25, 0.50, 1   );  //3
                shapeSetVertex(sva,20, 0.25, 0.00, 0.75);  //0
                shapeSetVertex(sva,21, 0.75, 0.00, 0.75);  //4
                shapeSetVertex(sva,22, 0.75, 0.00, 1   );  //6
                shapeSetVertex(sva,23, 0.25, 0.00, 1   );  //2
                sva->row=24;
            }
            else {//unit is 2, center is (0,0,0)
                shapeSetVertex(sva, 0, -1, -1, -1);  //4
                shapeSetVertex(sva, 1, -1,  1, -1);  //0
                shapeSetVertex(sva, 2, -1, -1,  1);  //7
                shapeSetVertex(sva, 3, -1,  1,  1);  //3
                shapeSetVertex(sva, 4,  1, -1, -1);  //5
                shapeSetVertex(sva, 5,  1,  1, -1);  //1
                shapeSetVertex(sva, 6,  1, -1,  1);  //6
                shapeSetVertex(sva, 7,  1,  1,  1);  //2
            }
        }
    } else {
        memset(sva, 0, sizeof(*sva));
        sva->col=4;
        sva->row=1024;
        sva->draw=preModel;
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
    }
};

void projectTest(void) {
    Shapeva finalModel; //crated but not transfered
    Shapeva finalShape; //transfered but not project
    Shapeva finalCamera, finalFront, finalLeft, finalTop; //4 project view
    Matric projFront, projLeft, projTop, projCamera;
    Matric localMat, *mat=&localMat;
    Shapeva *shape = &finalModel;
    shapeCreatePreset(shape, 0);
    //shapeDump(shape);

    matricSetInitUnit(mat, 4, 4);
    matricSetUnit(mat);
    matridRotate(mat, 0, 0, 90);
    shapeTransCeqAxB(&finalShape, shape, mat);
    //shapeDump(&finalShape);

    matricSetInitUnit(&projCamera, 4, 4);
    matricSetInitUnit(&projFront, 4, 4);
    matricSetInitUnit(&projLeft, 4, 4);
    matricSetInitUnit(&projTop, 4, 4);

    matricSetProjectXY(&projFront);
    matricSetProjectYZ(&projLeft);
    matricSetProjectXZ(&projTop);

    Matric tmpCamera;
    matricSetInitUnit(&tmpCamera, 4, 4);
    matridRotate(&tmpCamera, 30, 45, 0);
    matricCeqAxB(&projCamera, &tmpCamera, &projFront);

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
    matricSetInitUnit(mat, 4, 4);
    printf("Show unit\n");
    matricDump(mat);

    printf("Show shift\n");
    matricSetUnit(mat);
    matridShift(mat, 10, 20, 30);
    matricDump(mat);

    printf("Show rotate x\n");
    matricSetUnit(mat);
    matridRotate(mat, 45, 0, 0);
    matricDump(mat);

    printf("Show rotate y\n");
    matricSetUnit(mat);
    matridRotate(mat, 0, 45, 0);
    matricDump(mat);

    printf("Show rotate z\n");
    matricSetUnit(mat);
    matridRotate(mat, 0, 0, 45);
    matricDump(mat);

    printf("Show rotate x, y, z\n");
    matricSetUnit(mat);
    matridRotate(mat, 45, 60, 75);
    matricDump(mat);

    printf("Show shift, rotate x, y, z, shit\n");
    matricSetUnit(mat);
    matridShift(mat, -10, -20, -30);
    matridRotate(mat, 45, 60, 75);
    matridShift(mat, 10, 20, 30);
    matricDump(mat);

    projectTest();

    return 0;
}
//#if defined(__cplusplus)
//	}
//#endif
