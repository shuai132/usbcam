/**
 * @brief  使用V4L2框架的USB摄像头测试程序（摄像头需要支持MJPEG编码方式）
 * @date   2017-02-10
 * @author LiuShuai
 */
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "usbcam.h"
#include "jpge.h"
#include "jpgd.h"

using namespace std;
using namespace jpge;
using namespace jpgd;

int main(int argc, char** argv)
{
    Webcam usbcam("/dev/video0", 3264, 2448);
    if(!usbcam.isOpen) {
        return 0;
    }

    //测试单帧拍摄模式
    //ImgClass img = usbcam.snap();

    //测试流模式
    usbcam.start();     //开启后台线程处理
    sleep(10);          //后台线程运行10s(用于测试帧率)
    ImgClass img = usbcam.getImg(); //获取流中最新的jpg图片
    usbcam.stop();      //关闭并停止拍摄线程

    cout<<"getImg:"<<endl;
    cout<<"img.width:  "<<img.width<<endl;
    cout<<"img.height: "<<img.height<<endl;
    cout<<"img.size:   "<<img.size<<endl;

    //原图保存到文件"1.jpg"
    ofstream JPG_file;
    const char *jpgFile = "1.jpg";
    JPG_file.open(jpgFile);
    JPG_file.write((char *)img.data, img.size);
    JPG_file.close();

    //jpg解码 img.data -->> imgData;
    int actual_comps;   //通道数
    cout<<"开始解码..."<<endl;
    unsigned char *imgData = decompress_jpeg_image_from_memory(img.data, img.size, &img.width, &img.height, &actual_comps, 3);
    cout<<"actual_comps:"<<actual_comps<<endl;
    cout<<"解码结束！"<<endl;
    cout<<"width  = "<<img.width<<endl;
    cout<<"height = "<<img.height<<endl;
    cout<<"size   = "<<img.size<<endl;

    //jpg压缩编码
    ImgClass jpge(img.width, img.height, img.size); //初始化图像信息 for compress_image_to_jpeg_file_in_memory()
    cout<<"开始编码..."<<endl;
    compress_image_to_jpeg_file_in_memory(jpge.data, jpge.size, jpge.width, jpge.height, actual_comps, imgData);
    cout<<"编码结束!"<<endl;
    cout<<"width  = "<<jpge.width<<endl;
    cout<<"height = "<<jpge.height<<endl;
    cout<<"size   = "<<jpge.size<<endl;

    //重新压缩后图像保存到文件"2.jpg"
    ofstream image2;
    image2.open("2.jpg");
    image2.write((char *)jpge.data, jpge.size);
    image2.close();

    free(imgData);

    cout<<"end"<<endl;
    return 0;
}
