#ifndef __PLUGIN_H__
#define __PLUGIN_H__

struct pluginObject {
    int id;
    int version;
    void *param;
    void *priva;
    int (*open)(int mode);
    int (*close)(void);
    int (*user)(int msg, DWORD wParam, void *lParam);
    int (*render)(void *target);
    int (*display)(void *target, int winW, int winH);
    int (*registerObj)(int category, void *obj);
};
extern struct pluginObject plugina;
extern struct pluginObject pluginm3d;

#endif // __PLUGIN_H__
