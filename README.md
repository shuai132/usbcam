# usbcam 
 
## Brief 
Linux下使用V4L2框架的USB摄像头测试程序以及C++封装，并在用例中提供jpeg重新解/压缩算法实现。  

注：当前捕获模式使用V4L2_PIX_FMT_MJPEG，所以摄像头需要支持MJPEG编码方式。

## Require 
`gcc` `g++`
 
## Usage 
测试用例
``` 
make
./usbcam
``` 
Have Fun ~
