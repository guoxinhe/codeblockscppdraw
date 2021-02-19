#ifndef __PLUGIN_H__
#define __PLUGIN_H__

class SubWindow {
    //basic info
    int x,y,w,h;//position and size
    int thisSID=(int)(size_t)this;

    //boolean type to management
    int responseUserInput;
    int showWindow;//or hide window
    int focusedWindow;//boolean

    int renderCount;
    int renderAlpha; //alpha with other window

    int processKey(UINT message, WPARAM wParam, LPARAM lParam) {
        if(!responseUserInput || !focusedWindow)
            return 0;
        return 1;//response
    }
    int updateRenderBuffer() {
        renderCount = 0;
        return renderCount;
    }
    int renderToTarget() {
        renderCount++;
        return renderCount;
    }
public:
    SubWindow() {
    }
    ~SubWindow() {
    }
    int tryShow() {
        if(showWindow) {
            renderToTarget();
            return 1;
        }
        return 0;
    }
};
class SubScene {
    int thisSID=(int)(size_t)this;
    SubWindow *statusWindows[32];
    SubWindow *responseUserWindows[128];
    SubWindow *backgroundWindow;
    SubWindow *alternateWindow;
public:
    int display() {
        int showCount=0;
        if(backgroundWindow) {
            showCount+=backgroundWindow->tryShow();
        }
        int i;
        for(i=0;i<128;i++) {
            if(responseUserWindows[i])
                showCount+=responseUserWindows[i]->tryShow();
        }
        for(i=0;i<32;i++) {
            if(statusWindows[i])
                showCount+=statusWindows[i]->tryShow();
        }
        if(showCount==0 && alternateWindow)
            showCount+=alternateWindow->tryShow();
        return showCount;
    }
};

extern SubScene *allScenes[64];
extern SubScene *currentScene;



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
