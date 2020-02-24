#include <QCoreApplication>
#include <Xlib.h>
#include <extensions/Xfixes.h>
#include <extensions/Xdamage.h>

typedef unsigned char  BYTE;
typedef unsigned short	WORD;
typedef unsigned int  DWORD;

#define PACKED __attribute__(( packed, aligned(2)))

typedef struct tagBITMAPFILEHEADER{
     WORD     bfType;        //Linux此值为固定值，0x4d42
     DWORD    bfSize;        //BMP文件的大小，包含三部分
     WORD     bfReserved1;    //置0
     WORD     bfReserved2;
     DWORD    bfOffBits;     //文件起始位置到图像像素数据的字节偏移量

}PACKED BITMAPFILEHEADER;


typedef struct tagBITMAPINFOHEADER{
     DWORD    biSize;          //文件信息头的大小，40
     DWORD    biWidth;         //图像宽度
     DWORD    biHeight;        //图像高度
     WORD     biPlanes;        //BMP存储RGB数据，总为1
     WORD     biBitCount;      //图像像素位数，笔者RGB位数使用24
     DWORD    biCompression;   //压缩 0：不压缩  1：RLE8 2：RLE4
     DWORD    biSizeImage;     //4字节对齐的图像数据大小
     DWORD    biXPelsPerMeter; //水平分辨率  像素/米
     DWORD    biYPelsPerMeter;  //垂直分辨率  像素/米
     DWORD    biClrUsed;        //实际使用的调色板索引数，0：使用所有的调色板索引
     DWORD    biClrImportant;
}BITMAPINFOHEADER;

static void dump_bmp(char* buffer, int width, int height)
{
    static int id = 0;
    char file[200];
//    if (id > 50){
//        return;
//    }
    printf("creating bmp!\n");
    sprintf(file, "/home/zhougang/x11-screenshot/%d.bmp", id++);

    FILE* f = fopen(file, "wb");
    if (!f) {
       printf("Error creating bmp!\n");
       return;
    }

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = 0;
    bi.biSizeImage = width*height*4;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    BITMAPFILEHEADER bf;
    bf.bfType = 0x4d42;
    bf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bi.biSizeImage;
    bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;

    fwrite(&bf,sizeof(BITMAPFILEHEADER),1,f);                      //写入文件头
    fwrite(&bi,sizeof(BITMAPINFOHEADER),1,f);                      //写入信息头
    fwrite(buffer,bi.biSizeImage,1,f);

    printf("width : %d height: %d\n", width, height);
    fclose(f);

}

int main(int argc, char *argv[])
{

    Display * display = XOpenDisplay(nullptr);
    int screen = XDefaultScreen(display);
    Window win = RootWindow(display, screen);
    XWindowAttributes win_info;
    XGetWindowAttributes(display, win, &win_info);
    printf("width: %d height: %d\n", win_info.width, win_info.height);

    XSelectInput(display, win, StructureNotifyMask);

    int error_base;
    int xfixes_event_base;
    if (!XFixesQueryExtension(display, &xfixes_event_base, &error_base)) {
       printf("XFixesQueryExtension failed\n");
       return 0;
    }
    XFixesSelectCursorInput(display, DefaultRootWindow(display), XFixesDisplayCursorNotifyMask);


    int xdamage_event_base;
    if (!XDamageQueryExtension(display, &xdamage_event_base, &error_base)) {
        printf("XDamageQueryExtension failed!\n");
        return 0;
    }
    Damage damage = XDamageCreate(display, win, XDamageReportRawRectangles);

    XEvent event;

    while (1){
        XNextEvent(display, &event);
        printf("xdamge event %d\n", event.type);
        if (event.type == xfixes_event_base + 1) {
            XFixesCursorImage *cursor = XFixesGetCursorImage(display);
            //printf("cursor width: %d height: %d\n", cursor->width, cursor->height);
        } else if (event.type == ConfigureNotify){
            XConfigureEvent *config = &event.xconfigure;
            printf("win width: %d height: %d\n", config->width, config->height);
        } else if (event.type == xdamage_event_base + XDamageNotify){
            XDamageNotifyEvent *damage_event = reinterpret_cast<XDamageNotifyEvent*> (&event);
            XImage *image = XGetImage(display, damage_event->drawable, damage_event->area.x, damage_event->area.y,
                      damage_event->area.width, damage_event->area.height, AllPlanes, ZPixmap);
            dump_bmp(image->data, image->width, image->height);
            image->f.destroy_image(image);
        } else {
            printf("event ignore!\n");
        }

    }

    XDamageDestroy(display, damage);
    return 0;
}
