#include <stdio.h>
#include <unistd.h>
#include "touch.h"

int main()
{
    struct screenInfo dev;
    char path[128] = {0};

    createTouchScreen(&dev);
    printf("%dx%d\n", dev.width, dev.height);
    sleep(10);
    printf("Down\n");
    int x = dev.width / 2;
    int y= dev.height / 2;
    touchDown(&dev, 1, x, y);
    touchPosUpdate(&dev, 1, x, y - 100);
    touchPosUpdate(&dev, 1, x - 100, y - 100);
    touchPosUpdate(&dev, 1, x - 100, y);
    touchPosUpdate(&dev, 1, x, y);
    touchPosUpdate(&dev, 1, x, y + 100);
    touchUp(&dev, 1);
    destroyTouchScreen(&dev);

    return 0;
}