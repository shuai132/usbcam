#include "webcam.h"

#include "mytools.h"

/*调试程序时取消注释*/
//#define USE_WEBCAM_DEBUG

/*测试帧率时置1*/
#define COUNT_FRAME_NUM 1

static void process_image(void *data, size_t length);

/**********以下修改自官方历程**********/
/**********https://linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/v4l2spec/capture.c********/
/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
} io_method;

struct buffer {
    void *                  start;
    size_t                  length;
};

static char *           dev_name        = NULL;
static io_method	io		= IO_METHOD_MMAP;
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;

static void
errno_exit                      (const char *           s)
{
    fprintf (stderr, "%s error %d, %s\n",
             s, errno, strerror (errno));

    exit (EXIT_FAILURE);
}
static void
errno_exit                      (int status)
{
    exit (status);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
    int r;

    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

static int
read_frame			(void)
{
    struct v4l2_buffer buf;
    unsigned int i;

#if COUNT_FRAME_NUM
    static int count=1;
    printf("%d\n", count++);
#endif

    switch (io) {
    case IO_METHOD_READ:
        if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit ("read");
            }
        }

        process_image (buffers[0].start, buffers[0].length);

        break;

    case IO_METHOD_MMAP:
        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit ("VIDIOC_DQBUF");
            }
        }

        assert (buf.index < n_buffers);

        process_image (buffers[buf.index].start, buffers[buf.index].length);

        if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");

        break;

    case IO_METHOD_USERPTR:
        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit ("VIDIOC_DQBUF");
            }
        }

        for (i = 0; i < n_buffers; ++i)
            if (buf.m.userptr == (unsigned long) buffers[i].start
                    && buf.length == buffers[i].length)
                break;

        assert (i < n_buffers);

        process_image ((void *) buf.m.userptr, buf.length);

        if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");

        break;
    }

    return 1;
}

static void
frame                        (void)
{
    for (;;) {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO (&fds);
        FD_SET (fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select (fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;

            errno_exit ("select");
        }

        if (0 == r) {
            fprintf (stderr, "select timeout\n");
            errno_exit (EXIT_FAILURE);
        }

        if (read_frame ())
            break;

        /* EAGAIN - continue select loop. */
    }
}

static void
stop_capturing                  (void)
{
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
            errno_exit ("VIDIOC_STREAMOFF");

        break;
    }
}

static void
start_capturing                 (void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR (buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = i;

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                errno_exit ("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
            errno_exit ("VIDIOC_STREAMON");

        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR (buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_USERPTR;
            buf.index       = i;
            buf.m.userptr	= (unsigned long) buffers[i].start;
            buf.length      = buffers[i].length;

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                errno_exit ("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
            errno_exit ("VIDIOC_STREAMON");

        break;
    }
}

static void
uninit_device                   (void)
{
    unsigned int i;

    switch (io) {
    case IO_METHOD_READ:
        free (buffers[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap (buffers[i].start, buffers[i].length))
                errno_exit ("munmap");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
            free (buffers[i].start);
        break;
    }

    free (buffers);
}

static void
init_read			(unsigned int		buffer_size)
{
    buffers = (buffer *)calloc (1, sizeof (*buffers));

    if (!buffers) {
        fprintf (stderr, "Out of memory\n");
        errno_exit (EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc (buffer_size);

    if (!buffers[0].start) {
        fprintf (stderr, "Out of memory\n");
        errno_exit (EXIT_FAILURE);
    }
}

static void
init_mmap			(void)
{
    struct v4l2_requestbuffers req;

    CLEAR (req);

    req.count               = 4;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s does not support "
                             "memory mapping\n", dev_name);
            errno_exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf (stderr, "Insufficient buffer memory on %s\n",
                 dev_name);
        errno_exit (EXIT_FAILURE);
    }

    buffers = (buffer *)calloc (req.count, sizeof (*buffers));

    if (!buffers) {
        fprintf (stderr, "Out of memory\n");
        errno_exit (EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
            errno_exit ("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
                mmap (NULL /* start anywhere */,
                      buf.length,
                      PROT_READ | PROT_WRITE /* required */,
                      MAP_SHARED /* recommended */,
                      fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit ("mmap");
    }
}

static void
init_userp			(unsigned int		buffer_size)
{
    struct v4l2_requestbuffers req;
    unsigned int page_size;

    page_size = getpagesize ();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    CLEAR (req);

    req.count               = 4;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s does not support "
                             "user pointer i/o\n", dev_name);
            errno_exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_REQBUFS");
        }
    }

    buffers = (buffer *)calloc (4, sizeof (*buffers));

    if (!buffers) {
        fprintf (stderr, "Out of memory\n");
        errno_exit (EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = memalign (/* boundary */ page_size,
                                             buffer_size);

        if (!buffers[n_buffers].start) {
            fprintf (stderr, "Out of memory\n");
            errno_exit (EXIT_FAILURE);
        }
    }
}

static void
init_device                     (__u32 &width,
                                 __u32 &height)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s is no V4L2 device\n",
                     dev_name);
            errno_exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf (stderr, "%s is no video capture device\n",
                 dev_name);
        errno_exit (EXIT_FAILURE);
    }

    switch (io) {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf (stderr, "%s does not support read i/o\n",
                     dev_name);
            errno_exit (EXIT_FAILURE);
        }

        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf (stderr, "%s does not support streaming i/o\n",
                     dev_name);
            errno_exit (EXIT_FAILURE);
        }

        break;
    }


    /* Select video input, video standard and tune here. */


    CLEAR (cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }


    CLEAR (fmt);

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
        errno_exit ("VIDIOC_S_FMT");

    /* Note VIDIOC_S_FMT may change width and height. */
    width  = fmt.fmt.pix.width;
    height = fmt.fmt.pix.height;

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (io) {
    case IO_METHOD_READ:
        init_read (fmt.fmt.pix.sizeimage);
        break;

    case IO_METHOD_MMAP:
        init_mmap ();
        break;

    case IO_METHOD_USERPTR:
        init_userp (fmt.fmt.pix.sizeimage);
        break;
    }
}

static void
close_device                    (void)
{
    if (-1 == close (fd))
        errno_exit ("close");

    fd = -1;
}

static bool
open_device                     (void)
{
    struct stat st;

    if (-1 == stat (dev_name, &st)) {
        fprintf (stderr, "Cannot identify '%s': %d, %s\n",
                 dev_name, errno, strerror (errno));
        return false;
    }

    if (!S_ISCHR (st.st_mode)) {
        fprintf (stderr, "%s is no device\n", dev_name);
        return false;
    }

    fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        fprintf (stderr, "Cannot open '%s': %d, %s\n",
                 dev_name, errno, strerror (errno));
        return false;
    }
    return true;
}
/**********https://linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/v4l2spec/capture.c********/



/*********************************************C++封装部分*******************************************/
/**
 * @brief  C++封装部分
 * @date   2016-12-15
 * @update 2017-02-10
 * @author LiuShuai
 */
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
using namespace std;

static thread frameThread;
static atomic_bool isRun(false);
static JPGImage imgBuffer[2];
static volatile int readyImgPos = 0;
static mutex imgLock;
/**
 * @brief  当从V4L2收到一帧数据时会被调用
 * @date   2016-12-15
 * @update 2017-02-10
 * @author LiuShuai
 */
static void process_image(void *data, size_t length)
{
#ifdef USE_WEBCAM_DEBUG
    printf("received data length:%d\n", length);
    cout<<"readyImgPos:"<<readyImgPos<<endl;
#endif
    if(Webcam::isStreamMode)
    {
        imgLock.lock();
        imgBuffer[1-readyImgPos].data = (unsigned char *)data;
        imgBuffer[1-readyImgPos].size = length;
        readyImgPos = 1-readyImgPos;
        imgLock.unlock();
    }
    else
    {
        imgBuffer[0].data = (unsigned char *)data;
    }
}

static void autoframe()
{
    while(isRun)
    {
        frame();
    }
}

Webcam::Webcam(const char *devName, unsigned int width, unsigned int height)
{
    dev_name = (char *)devName;
    if(!open_device()) {
        isOpen = false;
        return;
    }
    isOpen = true;
    init_device (width, height);    //此处 若不适配的分辨率会自动被更正
    start_capturing ();

    for(int i=0; i<2; i++) {
        imgBuffer[i].size = buffers[0].length;
        imgBuffer[i].width  = width;
        imgBuffer[i].height = height;
    }
}

Webcam::~Webcam()
{
    if(isOpen) {
        stop();
        stop_capturing ();
        uninit_device ();
        close_device ();
    }
}

/**
 * @brief  单帧拍摄
 * @date   2017-01-23
 * @author LiuShuai
 */
ImgClass Webcam::snap()
{
    isStreamMode = false;
    frame();
    imgBuffer[0].size = jpgClean(imgBuffer[0].data, imgBuffer[0].size);
    return ImgClass(imgBuffer[0]);
}

/**
 * @brief  建立后台线程并开始拍摄
 * @date   2017-01-23
 * @author LiuShuai
 */
void Webcam::start()
{
    isRun = true;
    isStreamMode = true;
    frameThread  = thread(autoframe);
}

/**
 * @brief  获取流中最新的jpg图片
 * @date   2017-01-23
 * @update 2017-02-04
 * @update 2017-03-06
 * @author LiuShuai
 */
ImgClass Webcam::getImg()
{
#ifdef USE_WEBCAM_DEBUG
    cout<<"Webcam::getImg"<<endl;
    cout<<"imgBuffer[readyImgPos].size = "<<imgBuffer[readyImgPos].size<<endl;
#endif
    imgLock.lock();
    imgBuffer[readyImgPos].size = jpgClean(imgBuffer[readyImgPos].data, imgBuffer[readyImgPos].size);
    auto img = ImgClass(imgBuffer[readyImgPos]);
    imgLock.unlock();
    return img;
}

/**
 * @brief  暂停后台拍摄
 * @date   2017-01-23
 * @author LiuShuai
 */
void Webcam::pause()
{
    if(Webcam::isStreamMode) {
        imgLock.lock();
    }
}

/**
 * @brief  继续后台拍摄
 * @date   2017-01-23
 * @author LiuShuai
 */
void Webcam::goon()
{
    if(Webcam::isStreamMode) {
        imgLock.unlock();
    }
}

/**
 * @brief  停止拍摄并销毁后台线程
 * @date   2017-01-23
 * @author LiuShuai
 */
void Webcam::stop()
{
    if(isRun)
    {
        isRun = false;
        frameThread.join();
    }
}

bool Webcam::isStreamMode;

/*********************************************C++封装部分*******************************************/
