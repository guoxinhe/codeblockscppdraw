#ifndef __MATRIC_H__
#define __MATRIC_H__

//default to 4x4, row=col set to 1, else set to 0
//type: 0: int, 1: float, 2: double
#define MATRIC_INTEGER 0
#define MATRIC_FLOAT   1
#define MATRIC_DOUBLE  2
#define MATRIC_DEFAULT 1
typedef struct  {
    int row, col, type, flags;
    int ma[32][32];
}Matric;
typedef struct  {
    int row, col, type, flags;
    float ma[32][32];
}Matrif;
typedef struct  {
    int row, col, type, flags;
    double ma[32][32];
}Matrid;

#define MatriReset(mat) //reset to unit
#define MatriResetUnit(mat) //reset to unit
#define MatriResetZero(mat) //reset to all zero
#define MatriSet(mat,matsrc) //mat=matsrc
#define MatriSetFromArray(mat,valArray) //mat=for each in array
#define MatriSetElement(mat,row,col,val)

#define MatriSetShift(mat,x,y,z)
#define MatriSetMirrorx(mat)
#define MatriSetMirrory(mat)
#define MatriSetMirrorz(mat)
#define MatriSetMirror(mat,x,y,z)
#define MatriSetRotatex(mat,angle)
#define MatriSetRotatey(mat,angle)
#define MatriSetRotatez(mat,angle)
#define MatriSetScale(mat,scale)
#define MatriSetScalex(mat,scale)
#define MatriSetScaley(mat,scale)
#define MatriSetScalez(mat,scale)
#define MatriSetScalexy(mat,scale)
#define MatriSetScalexz(mat,scale)
#define MatriSetScaleyz(mat,scale)
#define MatriSetScrewxy(mat,delta)
#define MatriSetScrewxz(mat,delta)
#define MatriSetScrewyx(mat,delta)
#define MatriSetScrewyz(mat,delta)
#define MatriSetScrewzx(mat,delta)
#define MatriSetScrewzy(mat,delta)
#define MatriSetProjx(mat)
#define MatriSetProjy(mat)
#define MatriSetProjz(mat)

#define MatriDump(mat)


#endif // __MATRIC_H__
