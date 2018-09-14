/**********************************************************
 *
 * Proprietary & confidential, All right reserved, 2013
 *         Greenliant System, Inc.
 *
 *     Module Name : wintime.c
 *     Description : Get windows's system time
***********************************************************/


/***********************************************************
 *     Guo Xinhe @2018.5.3
 *         Description : Wrapper for get system's time
************************************************************/


/****************************************************
 *  Include section
 * for GCC windows version, do not include any file!
 *
****************************************************/

/***************************************************
 *  Defines section
 * Add all #defines here
 *
****************************************************/
#ifndef WINBASEAPI
#ifdef __W32API_USE_DLLIMPORT__
#define DECLSPEC_IMPORT __declspec(dllimport)
#define WINBASEAPI DECLSPEC_IMPORT
#else
#define WINBASEAPI
#endif
#endif
#define WINAPI __stdcall
#ifndef VOID
#define VOID void
#endif

#define DAYMILLSECONDS 86400000UL//how many milliseconds of a day.

/***************************************************
 * Structure and Union section
 * Add all structures and unions here
 * here hack the SYSTEMTIME, and make it easy portable
****************************************************/
typedef unsigned short WORD;
typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME,*LPSYSTEMTIME;

/***************************************************
 *  Variables section
 * Add all variable declaration here
 *
****************************************************/
static SYSTEMTIME sysNow;
static unsigned long toDay=0;//the day number from 1970/1/1
static unsigned long initDay=0;//the initialized day number from 1970/1/1
static unsigned long deltaMilliSeconds=0;//millisecond from initDay
static unsigned long todayMilliSeconds=0;//millisecond of current day.
static unsigned long long systemMilliSeconds=0;//millisecond from 1970/1/1

/***************************************************
 *  Function Prototype Section
 * Add prototypes for all functions called by this
 * module, with the exception of runtime routines.
 *
****************************************************/

WINBASEAPI VOID WINAPI GetSystemTime(LPSYSTEMTIME);
unsigned long HostRandom();
unsigned long HostGetBootMilliSeconds(void);
unsigned long long currentTimeMillis();//simulate android's method.
void dumpSystemTime(void);//debug

/*****************************************************************************
 *  Function Implementation Section
 * Add prototypes for all functions called by this
 * module, with the exception of runtime routines.
 *
*****************************************************************************/
static unsigned long const monthOffsetFlat[]={
  //1 2  3  4  5   6   7   8   9   10  11  12  1   2
    0,31,59,90,120,151,181,212,243,273,304,334,365,396};
static unsigned long const monthOffsetLeap[]={
  //1 2  3  4  5   6   7   8   9   10  11  12  1   2
    0,31,60,91,121,152,182,213,244,274,305,335,366,397};
static char *  const WeekName[]={"Sunday","Monday","Tuesday","Wednesday",
    "Thursday","Friday","Saturday","Sunday","Monday","Tuesday"};
static char *  const WeekBrnm[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat",
    "Sun","Mon","Tue"};

/**
 * get the total days number from 2000/01/01.
 */
static unsigned long getYearDays(SYSTEMTIME * st) {
    if(st==(void *)0 || st->wYear<2000)
        return 0;
    unsigned long offset;
    if((st->wYear%4)!=0) // not a leap year
        offset=monthOffsetFlat[st->wMonth-1]+st->wDay-1;
    else//leap year
        offset=monthOffsetLeap[st->wMonth-1]+st->wDay-1;
    offset+=(st->wYear-2000)*365;
    offset+=(st->wYear-2000+3)/4;//add /4 leap year's day.
    offset-=(st->wYear-2001)/100;//remove /100 not leap year's day.
    offset+=(st->wYear-2001)/400;//add /400 leap year's day.
    return offset;
}
/**
 * get the milliseconds of that day.
 */
static unsigned long getDayMillseconds(SYSTEMTIME * st) {
    if(st==(void *)0)
        return 0;
   unsigned long mills=1000 * (st->wHour*3600 + st->wMinute*60
            + st->wSecond) + st->wMilliseconds;
    return mills;
}
/**
 * dump current OS support date time value.
 */
static void dumpSystemTimeLow(SYSTEMTIME * st) {
    int printf(const char *format, ...);//do not include <stdio.h>
    //year/month/day hour:min:sec:mills 2018/5/3 9:18:35:151, #4 Thursday
    printf("year/month/day hour:min:sec:mills %d/%d/%d %d:%d:%d:%d, #%d %s\n",
           st->wYear,st->wMonth,st->wDay,st->wHour,st->wMinute,st->wSecond,
           st->wMilliseconds,st->wDayOfWeek,WeekName[st->wDayOfWeek&7]);
    printf("Day info: init=%d, today=%d\n",initDay,toDay);
    printf("Millisecond info: today=%d, delta to init=%d\n",
           todayMilliSeconds, deltaMilliSeconds);
    printf("System Milliseconds: %ull\n",systemMilliSeconds);
}
static char toStringBuffer[256];
static char *toStringSystemTime(SYSTEMTIME * st) {
    extern int sprintf(char *buf, const char *fmt,...);
    //year/month/day hour:min:sec:mills 2018/5/3 9:18:35:151, #4 Thursday
    sprintf(toStringBuffer, "year/month/day hour:min:sec:mills %d/%d/%d %d:%d:%d:%d, #%d %s",
           st->wYear,st->wMonth,st->wDay,st->wHour,st->wMinute,st->wSecond,
           st->wMilliseconds,st->wDayOfWeek,WeekName[st->wDayOfWeek&7]);
    return toStringBuffer;
}
static char *toStringDateTime(SYSTEMTIME * st) {
    extern int sprintf(char *buf, const char *fmt,...);
    //year/month/day hour:min:sec:mills 2018/5/3 9:18:35:151, #4 Thursday
    sprintf(toStringBuffer, "%d/%d/%d %d:%d:%d:%d %s",
           st->wYear,st->wMonth,st->wDay,st->wHour,st->wMinute,st->wSecond,
           st->wMilliseconds,WeekName[st->wDayOfWeek&7]);
    return toStringBuffer;
}

char *msToDate(unsigned long ms) {
    SYSTEMTIME stObj, *st=&stObj;
    st->wYear=0;
    st->wMonth=0;
    st->wDayOfWeek=0;
    st->wDay=ms/86400000; ms%=86400000;
    st->wHour=ms/3600000; ms%=3600000;
    st->wMinute=ms/60000; ms%=60000;
    st->wSecond=ms/1000;  ms%=1000;
    st->wMilliseconds=ms;
    st->wYear=st->wDay/365;
    st->wMonth=st->wDay/30;
    st->wDayOfWeek=st->wDay%7;
    return toStringDateTime(st);
}
/**
 * Get system hardware time, and update local variables
 */
static void updateWallClock(void) {
    GetSystemTime(&sysNow);
    //dumpSystemTime(&sysNow);
    toDay=getYearDays(&sysNow);
    todayMilliSeconds=getDayMillseconds(&sysNow);
    if(initDay == 0) {
        initDay=toDay;
    }
    //the past days. 32bit max is 24 days.
    deltaMilliSeconds = (toDay-initDay)*DAYMILLSECONDS + todayMilliSeconds;
    systemMilliSeconds = (unsigned long long)toDay*DAYMILLSECONDS +
        (unsigned long long)todayMilliSeconds;
}
/**
 * this module's debug code.
 */
void dumpSystemTime(void) {
    updateWallClock();
    dumpSystemTimeLow(&sysNow);
}
/**
 * debug utility
 */
char *toStringNow(void) {
    updateWallClock();
    return toStringDateTime(&sysNow);
}

/**
 * this implement Android System.currentTimeMillis().
 */
unsigned long long currentTimeMillis(void) {
    updateWallClock();
    return systemMilliSeconds;
}
/**
 * get the million seconds from this application start or system boot
 * valid when system boot <49.71 days. (49 days, 17 hours, 167 seconds)
 * overflow when boot > that limit.
 */
unsigned long HostGetBootMilliSeconds(void) {
    updateWallClock();
    return deltaMilliSeconds;
}
/**
 * some API has no random(), here is an alt implement
 */
static unsigned long hostRandomSeed=0;
unsigned long HostRandom(void) {
    updateWallClock();
    if(hostRandomSeed==0) {
    	//updateWallClock();
        hostRandomSeed = deltaMilliSeconds *11/17;
    }
    hostRandomSeed = (hostRandomSeed + deltaMilliSeconds + 49999) * 19/13;
    //hostRandomSeed = hostRandomSeed + hostRandomSeed/9;
    return hostRandomSeed;
}
