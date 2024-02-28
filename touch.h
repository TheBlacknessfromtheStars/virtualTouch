//
// Created by 32319 on 2024/2/26.
//

#ifndef ANDROIDTOUCH_TOUCH_H
#define ANDROIDTOUCH_TOUCH_H

struct finger
{
    int id;
    int isDown;
    int x;
    int y;
};

struct screenInfo
{
    int fd;
    int width;
    int height;
    struct finger fingers[10];
};

int initTouchscreenInfo(struct screenInfo *info);
void deleteInfo(struct screenInfo *info);
int createTouchScreen(struct screenInfo *dev);
void destroyTouchScreen(struct screenInfo *dev);
int touchTap(struct screenInfo *dev, unsigned short id, int x, int y);
int touchMove(struct screenInfo *dev, unsigned short id, int x1, int y1, int x2, int y2);
int touchDown(struct screenInfo *dev, unsigned short id, int x, int y);
int touchUp(struct screenInfo *dev, unsigned short id);
int touchPosUpdate(struct screenInfo *dev, unsigned short id, int x, int y);

#endif //ANDROIDTOUCH_TOUCH_H
