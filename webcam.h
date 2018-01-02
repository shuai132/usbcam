#ifndef USBCAM_H
#define USBCAM_H

/**
 * @brief  Small C++ wrapper around V4L
 * @date   2016-12-15
 * @update 2017-02-04
 * @author LiuShuai
 */
#include <string>
#include <cstring>

struct JPGImage {
    unsigned char *data;
    int width;
    int height;
    int size;
};

/**
 * @brief  ImgClass
 * @date   2016-12-20
 * @update 2017-02-04
 * @author LiuShuai
 */
class ImgClass
{
public:
    unsigned char *data = NULL;
    int width   = 0;
    int height  = 0;
    int size    = 0;
    std::string name;

    ImgClass(int width = 0, int height = 0, int size = 0):
        width(width),
        height(height),
        size(size)
    {
        if(size != 0) {
            data = new unsigned char[size];
        }
    }

    explicit ImgClass(const JPGImage &img) {
        data = new unsigned char[img.size];
        memcpy(data, img.data, img.size);
        width  = img.width;
        height = img.height;
        size   = img.size;
    }

    //拷贝构造
    ImgClass(const ImgClass &img) {
        data = new unsigned char[img.size];
        memcpy(data, img.data, img.size);
        width  = img.width;
        height = img.height;
        size   = img.size;
        name   = img.name;
    }

    ImgClass &operator=(const ImgClass &img) {
        if(this == &img) {
            return *this;
        }
        if(size != img.size) {
            delete []data;
            data = new unsigned char[img.size];
        }
        memcpy(data, img.data, img.size);
        width  = img.width;
        height = img.height;
        size   = img.size;
        name   = img.name;
        return *this;
    }

    ~ImgClass() {
        delete []data;
        data = NULL;
    }
};

class Webcam
{
public:
    explicit Webcam(const char *devName = "/dev/video0", unsigned int width = 640, unsigned int height = 480);
    ~Webcam();

    //单帧模式
    ImgClass snap();                //获取一张图片

    //流模式
    void start();                   //开启流模式
    ImgClass getImg();              //获取最新的一张jpg图片
    void stop();
    void pause();
    void goon();

    bool isOpen = false;
    static bool isStreamMode;
};

#endif // USBCAM_H
