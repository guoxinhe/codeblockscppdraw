#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <thread.h>

struct SimpleThread_t {
    s_thread_t* thread;//OS will create object for it.
    int statusFlags;//for thread modify, main read
    int controlFlags;//for main modify, thread read
    int lifeCycle;//for thread modify
    int idleSleepMs;//for idle sleep in milliseconds.
    void *(*procedure)(void* apParam);
    void *privateData;//parameter for the procedure, NULL for 'this'.
    char name[32];
};

void SimpleThreadOpen(struct SimpleThread_t *st) {
    if(st==NULL || st->statusFlags!=0)
        return;
    if(st->privateData==NULL)
        st->privateData=(void *)st;//point to itself.
    if(st->idleSleepMs<1)
        st->idleSleepMs=1000;//default to 1000
    st->lifeCycle=0;
    st->statusFlags=0;
    st->controlFlags=1;
    st->thread=s_thread_create(st->procedure, st->privateData);
    if(st->thread!=NULL)
        printf("Thread open  %s on #%d        [    OK    ]\n", st->name, st->lifeCycle);
    else
        printf("Thread open  %s on #%d        [   FAIL   ]\n", st->name, st->lifeCycle);
}
void SimpleThreadClose(struct SimpleThread_t *st) {
    if(st==NULL || st->statusFlags==0 || st->thread==NULL)
        return;

    st->controlFlags=0;
    while((st->statusFlags & 1) !=0) {//wait thread exit.
        //sleep 1 second
        printf("Thread wait %s quit #%d\n", st->name, st->lifeCycle);
        SleepMS(100);
    }
    s_thread_join(st->thread, NULL);
    st->thread=NULL;
    st->statusFlags=0;
    printf("Thread close %s on #%d        [    OK    ]\n", st->name, st->lifeCycle);
}

static void *simpleThreadProcedure(void* apParam) {
    if(apParam==NULL)
        return NULL;
    struct SimpleThread_t *me=(struct SimpleThread_t *)apParam;

    while(me->controlFlags!=0) {
        me->statusFlags |= 1;
        //sleep 1 second.
        me->lifeCycle++;
        printf("Thread %s life cycle #%d\n", me->name, me->lifeCycle);
        SleepMS(me->idleSleepMs);
    }
    me->statusFlags &= ~1;
    return apParam;
}
struct SimpleThread_t printThread={
    .statusFlags=0,
    .procedure=simpleThreadProcedure,
    .privateData=NULL,
    .idleSleepMs=1000,
    .name="Print Thread",
};
struct SimpleThread_t logThread={
    .statusFlags=0,
    .procedure=simpleThreadProcedure,
    .privateData=NULL,
    .idleSleepMs=2000,
    .name="Log Thread",
};

static void printThreadOpen() {
    SimpleThreadOpen(&logThread);
    SimpleThreadOpen(&printThread);
}
static void printThreadClose() {
    SimpleThreadClose(&logThread);
    SimpleThreadClose(&printThread);
}
