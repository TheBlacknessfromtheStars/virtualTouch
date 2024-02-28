//
// Created by 32319 on 2024/2/26.
//

#include "touch.h"
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>
#include <linux/uinput.h>

int findTouchScreenFD(struct screenInfo *info)
{
    DIR *input = opendir("/dev/input");
    struct dirent *child = NULL;

    if(!input)
        return -1;

    while( (child = readdir(input)) != NULL )
    {
        if(child->d_name[0] == '.')
            continue;

        char eventPath[64] = {0};
        snprintf(eventPath, 63, "/dev/input/%s", child->d_name);

        int fd = open(eventPath, O_RDONLY);
        if(fd < 0)
        {
            perror("open");
            continue;
        }
        struct input_absinfo absinfo;
        memset(&absinfo, 0, sizeof(absinfo));
        ioctl(fd, EVIOCGABS(ABS_MT_SLOT), &absinfo);
        if(absinfo.maximum == 9)
        {
            info->fd = fd;
            break;
        }
        close(fd);
    }
    closedir(input);

    return 0;
}

int getScreenResolution(struct screenInfo *info)
{
    FILE *fp = popen("wm size", "r");
    if(!fp)
        return -1;

    char cmdResult[64] = {0};
    fread(cmdResult, 4, 16, fp);
    pclose(fp);
    sscanf(cmdResult, "Physical size: %dx%d", &info->width, &info->height);

    return 0;
}

int initTouchscreenInfo(struct screenInfo *info)
{
    memset(info, 0, sizeof(struct screenInfo));
    info->fd = -1;

    if(findTouchScreenFD(info) < 0)
    {
        printf("findTouchScreenFD fail\n");
        return -1;
    }
    if(getScreenResolution(info) < 0)
    {
        printf("getScreenResolution fail\n");
        return -1;
    }

    return 0;
}

void deleteInfo(struct screenInfo *info)
{
    close(info->fd);
    info->fd = -1;
}

int createTouchScreen(struct screenInfo *dev)
{
    struct uinput_user_dev usetup;
    int uinputFd = open("/dev/uinput", O_RDWR);

    if(uinputFd < 0)
    {
        printf("open /dev/uinput fail\n");
        return -1;
    }
    getScreenResolution(dev);

    ioctl(uinputFd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
    ioctl(uinputFd, UI_SET_EVBIT, EV_ABS);
    ioctl(uinputFd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinputFd, UI_SET_EVBIT, EV_SYN);

    ioctl(uinputFd, UI_SET_ABSBIT, ABS_MT_TOUCH_MINOR);
    ioctl(uinputFd, UI_SET_ABSBIT, ABS_X);
    ioctl(uinputFd, UI_SET_ABSBIT, ABS_Y);
    ioctl(uinputFd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
    ioctl(uinputFd, UI_SET_ABSBIT, ABS_MT_WIDTH_MAJOR);
    ioctl(uinputFd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(uinputFd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(uinputFd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);

    ioctl(uinputFd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(uinputFd, UI_SET_KEYBIT, BTN_TOOL_FINGER);

    memset(&usetup, 0, sizeof(struct uinput_user_dev));
    strcpy(usetup.name, "Virtual Touch Screen");
    usetup.id.bustype = BUS_SPI;
    usetup.id.vendor = 0x6c90;
    usetup.id.product = 0x8fb0;

    usetup.absmin[ABS_X] = 0;
    usetup.absmax[ABS_X] = dev->width - 1;
    usetup.absmin[ABS_Y] = 0;
    usetup.absmax[ABS_Y] = dev->height - 1;
    usetup.absmin[ABS_MT_POSITION_X] = 0;
    usetup.absmax[ABS_MT_POSITION_X] = dev->width - 1;
    usetup.absfuzz[ABS_MT_POSITION_X] = 0;
    usetup.absflat[ABS_MT_POSITION_X] = 0;
    usetup.absmin[ABS_MT_POSITION_Y] = 0;
    usetup.absmax[ABS_MT_POSITION_Y] = dev->height - 1;
    usetup.absfuzz[ABS_MT_POSITION_Y] = 0;
    usetup.absflat[ABS_MT_POSITION_Y] = 0;
    usetup.absmin[ABS_MT_PRESSURE] = 0;
    usetup.absmax[ABS_MT_PRESSURE] = 1000;
    usetup.absfuzz[ABS_MT_PRESSURE] = 0;
    usetup.absflat[ABS_MT_PRESSURE] = 0;
    usetup.absmax[ABS_MT_TOUCH_MAJOR] = 255;
    usetup.absmin[ABS_MT_TRACKING_ID] = 0;
    usetup.absmax[ABS_MT_TRACKING_ID] = 65535;

    if(write(uinputFd, &usetup, sizeof(usetup)) < 0)
    {
        perror("write");
        close(uinputFd);
        return -1;
    }
    if(ioctl(uinputFd, UI_DEV_CREATE) < 0)
    {
        printf("create fail\n");
        perror("ioctl");
        close(uinputFd);
        return -1;
    }

    dev->fd = uinputFd;
    memset(dev->fingers, 0, sizeof(struct finger)*10);
    for(int i = 0; i < 10; i++)
        dev->fingers[i].id = -1;

    return 0;
}

void destroyTouchScreen(struct screenInfo *dev)
{
    ioctl(dev->fd, UI_DEV_DESTROY);
    close(dev->fd);
}

int emit(int fd, int type, int code, int value)
{
    struct input_event event;

    event.type = type;
    event.code = code;
    event.value = value;
    event.time.tv_sec = 0;
    event.time.tv_usec = 0;

    if(0 > write(fd, &event, sizeof(event)))
    {
        perror("emit write");
        return -1;
    }

    return 0;
}

int getIndexById(struct screenInfo *dev)
{
    for(int i = 0; i < 10; i++)
        if(dev->fingers[i].isDown == 0 && dev->fingers[i].id == -1)
            return i;
    return -1;
}

struct finger *getFinger(struct screenInfo *dev, unsigned short id)
{
    for(int i = 0; i < 10; i++)
    {
        if(dev->fingers[i].id == id)
            return &dev->fingers[i];
    }

    return NULL;
}

int touchDown(struct screenInfo *dev, unsigned short id, int x, int y)
{
    int index = getIndexById(dev);
    if(index < 0)
    {
        return -1;
    }
    struct finger *finger = &dev->fingers[index];
    emit(dev->fd, EV_ABS, ABS_MT_TRACKING_ID, id);
    emit(dev->fd, EV_KEY, BTN_TOUCH, 1);
    emit(dev->fd, EV_ABS, ABS_MT_POSITION_X, x);
    emit(dev->fd, EV_ABS, ABS_MT_POSITION_Y, y);
    emit(dev->fd, EV_SYN, SYN_REPORT, 0);
    finger->isDown = 1;
    finger->x = x;
    finger->y = y;
    finger->id = id;

    return 0;
}

int touchUp(struct screenInfo *dev, unsigned short id)
{
    struct finger *finger = getFinger(dev, id);
    if(!finger)
        return -1;
    emit(dev->fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    emit(dev->fd, EV_KEY, BTN_TOUCH, 0);
    emit(dev->fd, EV_SYN, SYN_REPORT, 0);
    finger->isDown = 0;
    finger->id = -1;
    
    return 0;
}

int touchPosUpdate(struct screenInfo *dev, unsigned short id, int x, int y)
{
    struct finger *finger = getFinger(dev, id);
    if(!finger)
        return -1;
    emit(dev->fd, EV_ABS, ABS_MT_TRACKING_ID, id);
    emit(dev->fd, EV_ABS, ABS_MT_POSITION_X, x);
    emit(dev->fd, EV_ABS, ABS_MT_POSITION_Y, y);
    emit(dev->fd, EV_SYN, SYN_REPORT, 0);
    finger->x = x;
    finger->y = y;

    return 0;
}

int touchTap(struct screenInfo *dev, unsigned short id, int x, int y)
{
    touchDown(dev, id, x, y);
    touchUp(dev, id);

    return 0;
}

int touchMove(struct screenInfo *dev, unsigned short id, int x1, int y1, int x2, int y2)
{
    touchDown(dev, id, x1, y1);
    //sleep(2);
    touchPosUpdate(dev, id, x2, y2);
    touchUp(dev, id);

    return 0;
}