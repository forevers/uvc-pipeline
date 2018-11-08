// dk2
// -fRGB24 -s120x720 --buffer-size 245600 -c1000 -n4 /dev/video2
// webcam
// ??? test and support this

#include <iostream>
#include <fcntl.h> 
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

extern "C"
{
    #include <linux/videodev2.h>
}

#include "frame_queue_ifc.h"
#include "render_ui.h"

static inline unsigned char sat(int i) {
  return (unsigned char)( i >= 255 ? 255 : (i < 0 ? 0 : i));
}
#define YUYV2RGB_2(pyuv, prgb) { \
    float r = 1.402f * ((pyuv)[3]-128); \
    float g = -0.34414f * ((pyuv)[1]-128) - 0.71414f * ((pyuv)[3]-128); \
    float b = 1.772f * ((pyuv)[1]-128); \
    (prgb)[0] = sat(pyuv[0] + r); \
    (prgb)[1] = sat(pyuv[0] + g); \
    (prgb)[2] = sat(pyuv[0] + b); \
    (prgb)[3] = sat(pyuv[2] + r); \
    (prgb)[4] = sat(pyuv[2] + g); \
    (prgb)[5] = sat(pyuv[2] + b); \
    }
#define IYUYV2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (prgb)[0] = sat(*(pyuv) + r); \
    (prgb)[1] = sat(*(pyuv) + g); \
    (prgb)[2] = sat(*(pyuv) + b); \
    (prgb)[3] = sat((pyuv)[2] + r); \
    (prgb)[4] = sat((pyuv)[2] + g); \
    (prgb)[5] = sat((pyuv)[2] + b); \
    }
#define IYUYV2RGB_8(pyuv, prgb) IYUYV2RGB_4(pyuv, prgb); IYUYV2RGB_4(pyuv + 8, prgb + 12);
#define IYUYV2RGB_4(pyuv, prgb) IYUYV2RGB_2(pyuv, prgb); IYUYV2RGB_2(pyuv + 4, prgb + 6);

static IFrameQueue* frame_queue_ifc_ = nullptr;

static int g_shift = 4;

extern CameraConfig camera_config;

//#define DEBUG
#if defined(DEBUG) 
#define DEBUG_LOG(ARGV, ...) printf(ARGV,);
#else
#define DEBUG_LOG(ARGV, ...) 
#endif

#define ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))

enum buffer_fill_mode
{
    BUFFER_FILL_NONE = 0,
    BUFFER_FILL_FRAME = 1 << 0,
    BUFFER_FILL_PADDING = 1 << 1,
};

struct buffer
{
    unsigned int idx;
    unsigned int padding[VIDEO_MAX_PLANES];
    unsigned int size[VIDEO_MAX_PLANES];
    uint8_t *mem[VIDEO_MAX_PLANES];
};

struct device
{
    int fd;
    int opened;

    enum v4l2_buf_type type;
    enum v4l2_memory memtype;
    unsigned int nbufs;
    struct buffer *buffers;

    unsigned int width;
    unsigned int height;
    uint32_t buffer_output_flags;
    uint32_t timestamp_type;

    unsigned char num_planes;
    struct v4l2_plane_pix_format plane_fmt[VIDEO_MAX_PLANES];

    void *pattern[VIDEO_MAX_PLANES];
    unsigned int patternsize[VIDEO_MAX_PLANES];
};

static bool video_is_mplane(struct device *dev)
{
    return dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
           dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
}

static bool video_is_capture(struct device *dev)
{
    return dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
           dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

static bool video_is_output(struct device *dev)
{
    return dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
           dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT;
}

static struct v4l2_format_info {
    const char *name;
    unsigned int fourcc;
    unsigned char n_planes;
} pixel_formats[] = {
    { "RGB332", V4L2_PIX_FMT_RGB332, 1 },
    { "RGB555", V4L2_PIX_FMT_RGB555, 1 },
    { "RGB565", V4L2_PIX_FMT_RGB565, 1 },
    { "RGB555X", V4L2_PIX_FMT_RGB555X, 1 },
    { "RGB565X", V4L2_PIX_FMT_RGB565X, 1 },
    { "BGR24", V4L2_PIX_FMT_BGR24, 1 },
    { "RGB24", V4L2_PIX_FMT_RGB24, 1 },
    { "BGR32", V4L2_PIX_FMT_BGR32, 1 },
    { "RGB32", V4L2_PIX_FMT_RGB32, 1 },
    { "Y8", V4L2_PIX_FMT_GREY, 1 },
    { "Y10", V4L2_PIX_FMT_Y10, 1 },
    { "Y12", V4L2_PIX_FMT_Y12, 1 },
    { "Y16", V4L2_PIX_FMT_Y16, 1 },
    { "UYVY", V4L2_PIX_FMT_UYVY, 1 },
    { "VYUY", V4L2_PIX_FMT_VYUY, 1 },
    { "YUYV", V4L2_PIX_FMT_YUYV, 1 },
    { "YVYU", V4L2_PIX_FMT_YVYU, 1 },
    { "NV12", V4L2_PIX_FMT_NV12, 1 },
    { "NV12M", V4L2_PIX_FMT_NV12M, 2 },
    { "NV21", V4L2_PIX_FMT_NV21, 1 },
    { "NV21M", V4L2_PIX_FMT_NV21M, 2 },
    { "NV16", V4L2_PIX_FMT_NV16, 1 },
    { "NV16M", V4L2_PIX_FMT_NV16M, 2 },
    { "NV61", V4L2_PIX_FMT_NV61, 1 },
    { "NV61M", V4L2_PIX_FMT_NV61M, 2 },
    { "NV24", V4L2_PIX_FMT_NV24, 1 },
    { "NV42", V4L2_PIX_FMT_NV42, 1 },
    { "YUV420M", V4L2_PIX_FMT_YUV420M, 3 },
    { "SBGGR8", V4L2_PIX_FMT_SBGGR8, 1 },
    { "SGBRG8", V4L2_PIX_FMT_SGBRG8, 1 },
    { "SGRBG8", V4L2_PIX_FMT_SGRBG8, 1 },
    { "SRGGB8", V4L2_PIX_FMT_SRGGB8, 1 },
    { "SBGGR10_DPCM8", V4L2_PIX_FMT_SBGGR10DPCM8, 1 },
    { "SGBRG10_DPCM8", V4L2_PIX_FMT_SGBRG10DPCM8, 1 },
    { "SGRBG10_DPCM8", V4L2_PIX_FMT_SGRBG10DPCM8, 1 },
    { "SRGGB10_DPCM8", V4L2_PIX_FMT_SRGGB10DPCM8, 1 },
    { "SBGGR10", V4L2_PIX_FMT_SBGGR10, 1 },
    { "SGBRG10", V4L2_PIX_FMT_SGBRG10, 1 },
    { "SGRBG10", V4L2_PIX_FMT_SGRBG10, 1 },
    { "SRGGB10", V4L2_PIX_FMT_SRGGB10, 1 },
    { "SBGGR12", V4L2_PIX_FMT_SBGGR12, 1 },
    { "SGBRG12", V4L2_PIX_FMT_SGBRG12, 1 },
    { "SGRBG12", V4L2_PIX_FMT_SGRBG12, 1 },
    { "SRGGB12", V4L2_PIX_FMT_SRGGB12, 1 },
    { "DV", V4L2_PIX_FMT_DV, 1 },
    { "MJPEG", V4L2_PIX_FMT_MJPEG, 1 },
    { "MPEG", V4L2_PIX_FMT_MPEG, 1 },
};

static const struct v4l2_format_info *v4l2_format_by_fourcc(unsigned int fourcc)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pixel_formats); ++i) {
        if (pixel_formats[i].fourcc == fourcc)
            return &pixel_formats[i];
    }

    return NULL;
}

static const struct v4l2_format_info *v4l2_format_by_name(const char *name)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pixel_formats); ++i) {
        if (strcasecmp(pixel_formats[i].name, name) == 0)
            return &pixel_formats[i];
    }

    return NULL;
}

static const char *v4l2_format_name(unsigned int fourcc)
{
    const struct v4l2_format_info *info;
    static char name[5];
    unsigned int i;

    info = v4l2_format_by_fourcc(fourcc);
    if (info)
        return info->name;

    for (i = 0; i < 4; ++i) {
        name[i] = fourcc & 0xff;
        fourcc >>= 8;
    }

    name[4] = '\0';
    return name;
}

static const struct {
    const char *name;
    enum v4l2_field field;
} fields[] = {
    { "any", V4L2_FIELD_ANY },
    { "none", V4L2_FIELD_NONE },
    { "top", V4L2_FIELD_TOP },
    { "bottom", V4L2_FIELD_BOTTOM },
    { "interlaced", V4L2_FIELD_INTERLACED },
    { "seq-tb", V4L2_FIELD_SEQ_TB },
    { "seq-bt", V4L2_FIELD_SEQ_BT },
    { "alternate", V4L2_FIELD_ALTERNATE },
    { "interlaced-tb", V4L2_FIELD_INTERLACED_TB },
    { "interlaced-bt", V4L2_FIELD_INTERLACED_BT },
};

static const char *v4l2_field_name(enum v4l2_field field)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(fields); ++i) {
        if (fields[i].field == field)
            return fields[i].name;
    }

    return "unknown";
}

static void video_set_buf_type(struct device *dev, enum v4l2_buf_type type)
{
    DEBUG_LOG("enter %s\n", __FUNCTION__);

    LOG("%d > dev->type = %x\n", __LINE__, type);
    
    // type is V4LS_BUF_TYPE_VIDEO_CAPTURE_PLANE for mvis camera
    dev->type = type;

    LOG("exit %s\n", __FUNCTION__);
}

static bool video_has_valid_buf_type(struct device *dev)
{
    return (int)dev->type != -1;
}

static void video_init(struct device *dev)
{
    LOG("enter %s\n", __FUNCTION__);

    memset(dev, 0, sizeof(device));
    dev->fd = -1;

    dev->memtype = V4L2_MEMORY_MMAP;
    dev->buffers = NULL;
    dev->type = (enum v4l2_buf_type)-1;
    LOG("%d > dev->type = %d\n", __LINE__, dev->type);

    LOG("exit %s\n", __FUNCTION__);
}

static bool video_has_fd(struct device *dev)
{
    return dev->fd != -1;
}

static int video_open(struct device *dev, const char *devname)
{
    LOG("enter %s\n", __FUNCTION__);

    if (video_has_fd(dev)) {
        LOG("Can't open device (already open).\n");
        return -1;
    }

    // obtain camera device node file descriptor
    dev->fd = open(devname, O_RDWR);
    if (dev->fd < 0) {
        LOG("Error opening device %s: %s (%d).\n", devname, strerror(errno), errno);
        return dev->fd;
    }

    LOG("Device %s opened.\n", devname);

    dev->opened = 1;

    LOG("exit %s\n", __FUNCTION__);

    return 0;
}

static int video_querycap(struct device *dev, unsigned int *capabilities)
{
    struct v4l2_capability cap;
    int ret;

    LOG("enter %s\n", __FUNCTION__);
    LOG("%d > ***** VIDIOC_QUERYCAP *****\n", __LINE__);

    memset(&cap, 0, sizeof cap);
    ret = ioctl(dev->fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        LOG("exit %s\n", __FUNCTION__);
        return 0;
    }

    LOG("%d > cap.capabilities : %x\n", __LINE__, cap.capabilities);
    LOG("%d > cap.device_caps : %x\n", __LINE__, cap.device_caps);

    *capabilities = cap.capabilities & V4L2_CAP_DEVICE_CAPS ? cap.device_caps : cap.capabilities;

    LOG("%d > *capabilities : %x\n", __LINE__, *capabilities);

    LOG("Device `%s' on `%s' is a video %s (%s mplanes) device.\n",
        cap.card, cap.bus_info,
        video_is_capture(dev) ? "capture" : "output",
        video_is_mplane(dev) ? "with" : "without");

    LOG("exit\n");

    return 0;
}


static int cap_get_buf_type(unsigned int capabilities)
{
    LOG("enter %s\n", __FUNCTION__);

    // select the capability to use ... V4L2_CAP_VIDEO_CAPTURE_MPLANE for mvis camera
    if (capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
        LOG("V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE\n");
        LOG("exit\n");
        return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    } else if (capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE) {
        LOG("V4L2_CAP_VIDEO_OUTPUT_MPLANE\n");
        LOG("exit\n");
        return V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    } else if (capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        LOG("V4L2_BUF_TYPE_VIDEO_CAPTURE\n");
        LOG("exit\n");
        return  V4L2_BUF_TYPE_VIDEO_CAPTURE;
    } else if (capabilities & V4L2_CAP_VIDEO_OUTPUT) {
        LOG("V4L2_CAP_VIDEO_OUTPUT\n");
        LOG("exit %s\n", __FUNCTION__);
        return V4L2_BUF_TYPE_VIDEO_OUTPUT;
    } else {
        LOG("Device supports neither capture nor output.\n");
        LOG("exit\n");
        return -EINVAL;
    }

    LOG("exit\n");

    return 0;
}


static void video_close(struct device *dev)
{
    unsigned int i;

    LOG("enter\n");

    for (i = 0; i < dev->num_planes; i++)
        free(dev->pattern[i]);

    free(dev->buffers);
    if (dev->opened)
        close(dev->fd);

    LOG("exit\n");
}


static int video_get_format(struct device *dev)
{
    struct v4l2_format fmt;
    unsigned int i;
    int ret;

    LOG("enter\n");

    memset(&fmt, 0, sizeof fmt);
    fmt.type = dev->type;

    LOG("***** VIDIOC_G_FMT *****\n");

    ret = ioctl(dev->fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        LOG("Unable to get format: %s (%d).\n", strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    if (video_is_mplane(dev)) {
        dev->width = fmt.fmt.pix_mp.width;
        dev->height = fmt.fmt.pix_mp.height;
        dev->num_planes = fmt.fmt.pix_mp.num_planes;

        LOG("Video format: %s (%08x) %ux%u field %s, %u planes: \n",
            v4l2_format_name(fmt.fmt.pix_mp.pixelformat), 
            fmt.fmt.pix_mp.pixelformat,
            fmt.fmt.pix_mp.width, 
            fmt.fmt.pix_mp.height,
            v4l2_field_name((v4l2_field)fmt.fmt.pix_mp.field),
            fmt.fmt.pix_mp.num_planes);

        for (i = 0; i < fmt.fmt.pix_mp.num_planes; i++) {
            dev->plane_fmt[i].bytesperline =
                    fmt.fmt.pix_mp.plane_fmt[i].bytesperline;
            dev->plane_fmt[i].sizeimage =
                    fmt.fmt.pix_mp.plane_fmt[i].bytesperline ?
                        fmt.fmt.pix_mp.plane_fmt[i].sizeimage : 0;

            LOG(" * Stride %u, buffer size %u\n",
                fmt.fmt.pix_mp.plane_fmt[i].bytesperline,
                fmt.fmt.pix_mp.plane_fmt[i].sizeimage);
        }
    } else {
        dev->width = fmt.fmt.pix.width;
        dev->height = fmt.fmt.pix.height;
        dev->num_planes = 1;

        dev->plane_fmt[0].bytesperline = fmt.fmt.pix.bytesperline;
        dev->plane_fmt[0].sizeimage = fmt.fmt.pix.bytesperline ? fmt.fmt.pix.sizeimage : 0;

        LOG("Video format: %s (%08x) %ux%u (stride %u) field %s buffer size %u\n",
            v4l2_format_name(fmt.fmt.pix.pixelformat), fmt.fmt.pix.pixelformat,
            fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.bytesperline,
            v4l2_field_name((v4l2_field)fmt.fmt.pix_mp.field),
            fmt.fmt.pix.sizeimage);
    }

    LOG("exit\n");

    return 0;
}


static int video_set_format(struct device *dev, unsigned int w, unsigned int h,
                unsigned int format, unsigned int stride,
                enum v4l2_field field)
{
    struct v4l2_format fmt;
    unsigned int i;
    int ret;

    LOG("enter\n");

    memset(&fmt, 0, sizeof fmt);
    fmt.type = dev->type;

    if (video_is_mplane(dev)) {
        const struct v4l2_format_info *info = v4l2_format_by_fourcc(format);

        fmt.fmt.pix_mp.width = w;
        fmt.fmt.pix_mp.height = h;
        fmt.fmt.pix_mp.pixelformat = format;
        fmt.fmt.pix_mp.field = field;
        // even though we are mplane format we will use just 1 plane
        fmt.fmt.pix_mp.num_planes = info->n_planes;

        for (i = 0; i < fmt.fmt.pix_mp.num_planes; i++)
            fmt.fmt.pix_mp.plane_fmt[i].bytesperline = stride;
    } else {
        fmt.fmt.pix.width = w;
        fmt.fmt.pix.height = h;
        fmt.fmt.pix.pixelformat = format;
        fmt.fmt.pix.field = field;
        fmt.fmt.pix.bytesperline = stride;
    }

    LOG("***** VIDIOC_S_FMT *****\n");
    
    ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        LOG("Unable to set format: %s (%d).\n", strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    if (video_is_mplane(dev)) {

        LOG("Video format set: %s (%08x) %ux%u field %s, colorspace : %x, %u planes: \n",
            v4l2_format_name(fmt.fmt.pix_mp.pixelformat), 
            fmt.fmt.pix_mp.pixelformat,
            fmt.fmt.pix_mp.width, 
            fmt.fmt.pix_mp.height,
            v4l2_field_name((v4l2_field)fmt.fmt.pix_mp.field),
            fmt.fmt.pix_mp.colorspace,
            fmt.fmt.pix_mp.num_planes);

        for (i = 0; i < fmt.fmt.pix_mp.num_planes; i++) {

            LOG(" * Stride %u, buffer size %u\n",
                fmt.fmt.pix_mp.plane_fmt[i].bytesperline,
                fmt.fmt.pix_mp.plane_fmt[i].sizeimage);
        }

    } else {

        LOG("Video format set: %s (%08x) %ux%u (stride %u) field %s buffer size %u\n",
            v4l2_format_name(fmt.fmt.pix.pixelformat), fmt.fmt.pix.pixelformat,
            fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.bytesperline,
            v4l2_field_name((v4l2_field)fmt.fmt.pix.field),
            fmt.fmt.pix.sizeimage);
    }

    LOG("exit\n");

    return 0;
}


static int video_set_framerate(struct device *dev, struct v4l2_fract *time_per_frame)
{
    struct v4l2_streamparm parm;
    int ret;

    LOG("enter\n");

    memset(&parm, 0, sizeof parm);
    parm.type = dev->type;

    ret = ioctl(dev->fd, VIDIOC_G_PARM, &parm);
    if (ret < 0) {
        LOG("Unable to get frame rate: %s (%d).\n", strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    LOG("Current frame rate: %u/%u\n",
        parm.parm.capture.timeperframe.numerator,
        parm.parm.capture.timeperframe.denominator);

    LOG("Setting frame rate to: %u/%u\n",
        time_per_frame->numerator,
        time_per_frame->denominator);

    parm.parm.capture.timeperframe.numerator = time_per_frame->numerator;
    parm.parm.capture.timeperframe.denominator = time_per_frame->denominator;

    ret = ioctl(dev->fd, VIDIOC_S_PARM, &parm);
    if (ret < 0) {
        LOG("Unable to set frame rate: %s (%d).\n", strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    ret = ioctl(dev->fd, VIDIOC_G_PARM, &parm);
    if (ret < 0) {
        LOG("Unable to get frame rate: %s (%d).\n", strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    LOG("Frame rate set: %u/%u\n",
        parm.parm.capture.timeperframe.numerator,
        parm.parm.capture.timeperframe.denominator);
    LOG("exit\n");
    return 0;
}


static int video_buffer_mmap(struct device *dev, struct buffer *buffer, struct v4l2_buffer *v4l2buf)
{
    unsigned int length;
    unsigned int offset;
    unsigned int i;

    LOG("enter\n");

    for (i = 0; i < dev->num_planes; i++) {
        if (video_is_mplane(dev)) {
            length = v4l2buf->m.planes[i].length;
            offset = v4l2buf->m.planes[i].m.mem_offset;
        } else {
            length = v4l2buf->length;
            offset = v4l2buf->m.offset;
        }

        LOG("***** mmap *****\n");

        buffer->mem[i] = (uint8_t*)mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED,
                      dev->fd, offset);
        if (buffer->mem[i] == MAP_FAILED) {
            LOG("Unable to map buffer %u/%u: %s (%d)\n", buffer->idx, i, strerror(errno), errno);
            LOG("exit\n");
            return -1;
        }

        buffer->size[i] = length;
        buffer->padding[i] = 0;

        LOG("Buffer %u/%u mapped at address %p.\n", buffer->idx, i, buffer->mem[i]);
    }

    LOG("exit\n");

    return 0;
}


static int video_buffer_munmap(struct device *dev, struct buffer *buffer)
{
    unsigned int i;
    int ret;

    LOG("enter\n");

    for (i = 0; i < dev->num_planes; i++) {
        ret = munmap(buffer->mem[i], buffer->size[i]);
        if (ret < 0) {
            LOG("Unable to unmap buffer %u/%u: %s (%d)\n", buffer->idx, i, strerror(errno), errno);
        }

        buffer->mem[i] = NULL;
    }

    LOG("exit\n");

    return 0;
}


static int video_buffer_alloc_userptr(struct device *dev, struct buffer *buffer,
                      struct v4l2_buffer *v4l2buf,
                      unsigned int offset, unsigned int padding)
{
    int page_size = getpagesize();
    unsigned int length;
    unsigned int i;
    int ret;

    LOG("enter\n");

    for (i = 0; i < dev->num_planes; i++) {
        if (video_is_mplane(dev))
            length = v4l2buf->m.planes[i].length;
        else
            length = v4l2buf->length;

        ret = posix_memalign((void**)(&buffer->mem[i]), page_size,
                     length + offset + padding);
        if (ret < 0) {
            LOG("Unable to allocate buffer %u/%u (%d)\n", buffer->idx, i, ret);
            LOG("exit\n");
            return -ENOMEM;
        }

        buffer->mem[i] += offset;
        buffer->size[i] = length;
        buffer->padding[i] = padding;

        LOG("Buffer %u/%u allocated at address %p.\n", buffer->idx, i, buffer->mem[i]);
    }

    LOG("exit\n");
    return 0;
}


static void video_buffer_free_userptr(struct device *dev, struct buffer *buffer)
{
    unsigned int i;

    for (i = 0; i < dev->num_planes; i++) {
        free(buffer->mem[i]);
        buffer->mem[i] = NULL;
    }
}


static void get_ts_flags(uint32_t flags, const char **ts_type, const char **ts_source)
{
    switch (flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) {
    case V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN:
        *ts_type = "unk";
        break;
    case V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC:
        *ts_type = "mono";
        break;
    case V4L2_BUF_FLAG_TIMESTAMP_COPY:
        *ts_type = "copy";
        break;
    default:
        *ts_type = "inv";
    }
    switch (flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK) {
    case V4L2_BUF_FLAG_TSTAMP_SRC_EOF:
        *ts_source = "EoF";
        break;
    case V4L2_BUF_FLAG_TSTAMP_SRC_SOE:
        *ts_source = "SoE";
        break;
    default:
        *ts_source = "inv";
    }
}


static int video_alloc_buffers(struct device *dev, int nbufs,
    unsigned int offset, unsigned int padding)
{
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    struct v4l2_requestbuffers rb;
    struct v4l2_buffer buf;
    struct buffer *buffers;
    unsigned int i;
    int ret;

    LOG("enter\n");

    memset(&rb, 0, sizeof rb);
    rb.count = nbufs;
    rb.type = dev->type;

    rb.memory = dev->memtype;

    ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        LOG("Unable to request buffers: %s (%d).\n", strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    LOG("%u buffers requested.\n", rb.count);

    buffers = (buffer*)malloc(rb.count * sizeof buffers[0]);
    if (buffers == NULL) {
        LOG("exit\n");
        return -ENOMEM;
    }

    /* Map the buffers. */
    for (i = 0; i < rb.count; ++i) {
        const char *ts_type, *ts_source;

        memset(&buf, 0, sizeof buf);
        memset(planes, 0, sizeof planes);

        buf.index = i;
        buf.type = dev->type;
        buf.memory = dev->memtype;
        buf.length = VIDEO_MAX_PLANES;
        buf.m.planes = planes;

        LOG("%d > ***** VIDIOC_QUERYBUF *****\n", __LINE__);

        ret = ioctl(dev->fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            LOG("Unable to query buffer %u: %s (%d).\n", i, strerror(errno), errno);
            LOG("exit\n");
            return ret;
        }
        get_ts_flags(buf.flags, &ts_type, &ts_source);
        LOG("length: %u offset: %u timestamp type/source: %s/%s\n", buf.length, buf.m.offset, ts_type, ts_source);

        buffers[i].idx = i;

        switch (dev->memtype) {
        case V4L2_MEMORY_MMAP:
            ret = video_buffer_mmap(dev, &buffers[i], &buf);
            break;

        case V4L2_MEMORY_USERPTR:
            ret = video_buffer_alloc_userptr(dev, &buffers[i], &buf, offset, padding);
            break;

        default:
            break;
        }

        if (ret < 0)
            return ret;
    }

    dev->timestamp_type = buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK;
    dev->buffers = buffers;
    dev->nbufs = rb.count;

    LOG("exit\n");

    return 0;
}


static int video_free_buffers(struct device *dev)
{
    struct v4l2_requestbuffers rb;
    unsigned int i;
    int ret;

    LOG("enter\n");

    if (dev->nbufs == 0)
        return 0;

    for (i = 0; i < dev->nbufs; ++i) {
        switch (dev->memtype) {
        case V4L2_MEMORY_MMAP:
            ret = video_buffer_munmap(dev, &dev->buffers[i]);
            if (ret < 0)
                return ret;
            break;
        case V4L2_MEMORY_USERPTR:
            video_buffer_free_userptr(dev, &dev->buffers[i]);
            break;
        default:
            break;
        }
    }

    memset(&rb, 0, sizeof rb);
    rb.count = 0;
    rb.type = dev->type;
    rb.memory = dev->memtype;

    ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        LOG("Unable to release buffers: %s (%d).\n", strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    LOG("%u buffers released.\n", dev->nbufs);

    free(dev->buffers);
    dev->nbufs = 0;
    dev->buffers = NULL;

    LOG("exit\n");

    return 0;
}


static int video_queue_buffer(struct device *dev, int index, enum buffer_fill_mode fill)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;
    unsigned int i;

    memset(&buf, 0, sizeof buf);
    buf.index = index;
    buf.type = dev->type;
    buf.memory = dev->memtype;

    if (video_is_output(dev)) {
        buf.flags = dev->buffer_output_flags;
        if (dev->timestamp_type == V4L2_BUF_FLAG_TIMESTAMP_COPY) {
            struct timespec ts;

            clock_gettime(CLOCK_MONOTONIC, &ts);
            buf.timestamp.tv_sec = ts.tv_sec;
            buf.timestamp.tv_usec = ts.tv_nsec / 1000;
        }
    }

    if (video_is_mplane(dev)) {
        buf.m.planes = planes;
        buf.length = dev->num_planes;
    } else {
        buf.length = dev->buffers[index].size[0];
    }

    if (dev->memtype == V4L2_MEMORY_USERPTR) {
        if (video_is_mplane(dev)) {
            for (i = 0; i < dev->num_planes; i++)
                buf.m.planes[i].m.userptr = (unsigned long)dev->buffers[index].mem[i];
        } else {
            buf.m.userptr = (unsigned long)dev->buffers[index].mem[0];
        }
    }

    for (i = 0; i < dev->num_planes; i++) {
        if (video_is_output(dev)) {
            if (video_is_mplane(dev))
                buf.m.planes[i].bytesused = dev->patternsize[i];
            else
                buf.bytesused = dev->patternsize[i];

            memcpy(dev->buffers[buf.index].mem[i], dev->pattern[i],
                   dev->patternsize[i]);
        } else {
            if (fill & BUFFER_FILL_FRAME)
                memset(dev->buffers[buf.index].mem[i], 0x55,
                       dev->buffers[index].size[i]);
            if (fill & BUFFER_FILL_PADDING)
                memset(dev->buffers[buf.index].mem[i] +
                    dev->buffers[index].size[i],
                       0x55, dev->buffers[index].padding[i]);
        }
    }

    DEBUG_LOG("%d > ***** VIDIOC_QBUF *****\n", __LINE__);

    ret = ioctl(dev->fd, VIDIOC_QBUF, &buf);
    if (ret < 0)
        LOG("Unable to queue buffer: %s (%d).\n", strerror(errno), errno);

    return ret;
}


static int video_enable(struct device *dev, int enable)
{
    int type = dev->type;
    int ret;

    LOG("enter\n");

    LOG("***** %s *****\n", enable ? "VIDIOC_STREAMON" : "VIDIOC_STREAMOFF");

    ret = ioctl(dev->fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOG("Unable to %s streaming: %s (%d).\n", enable ? "start" : "stop", strerror(errno), errno);
        return ret;
    }

    LOG("exit\n");

    return 0;
}


static int video_set_quality(struct device *dev, unsigned int quality)
{
    struct v4l2_jpegcompression jpeg;
    int ret;

    LOG("enter\n");

    if (quality == (unsigned int)-1) {
        LOG("%d > invalid quality setting : %d\n", __LINE__, quality);
        LOG("exit\n");
        return 0;
    }

    memset(&jpeg, 0, sizeof jpeg);
    jpeg.quality = quality;

/*
    https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/vidioc-g-jpegcomp.html?highlight=vidioc_s_jpegcomp

        These ioctls are deprecated. New drivers and applications should use JPEG class controls for image quality and JPEG markers control.
*/
    LOG("***** VIDIOC_S_JPEGCOMP *****\n");

    ret = ioctl(dev->fd, VIDIOC_S_JPEGCOMP, &jpeg);
    if (ret < 0) {
        LOG("Unable to set quality to %u: %s (%d).\n", quality, strerror(errno), errno);
        LOG("exit\n");
        return ret;
    }

    LOG("***** VIDIOC_G_JPEGCOMP *****\n");

    ret = ioctl(dev->fd, VIDIOC_G_JPEGCOMP, &jpeg);
    if (ret >= 0)
        LOG("Quality set to %u\n", jpeg.quality);

    LOG("exit\n");
    return 0;
}


static int video_load_test_pattern(struct device *dev, const char *filename)
{
    unsigned int plane;
    unsigned int size;
    int fd = -1;
    int ret;

    LOG("enter\n");

    if (filename != NULL) {
        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            LOG("Unable to open test pattern file '%s': %s (%d).\n", filename, strerror(errno), errno);
            LOG("exit\n");
            return -errno;
        }
    }

    /* Load or generate the test pattern */
    for (plane = 0; plane < dev->num_planes; plane++) {
        size = dev->buffers[0].size[plane];
        dev->pattern[plane] = malloc(size);
        if (dev->pattern[plane] == NULL) {
            ret = -ENOMEM;
            goto done;
        }

        if (filename != NULL) {
            ret = read(fd, dev->pattern[plane], size);
            if (ret != (int)size && dev->plane_fmt[plane].bytesperline != 0) {
                LOG("Test pattern file size %u doesn't match image size %u\n", ret, size);
                ret = -EINVAL;
                goto done;
            }
        } else {
            // uint8_t *data = dev->pattern[plane];
            uint8_t *data = (uint8_t*)dev->pattern[plane];
            unsigned int i;

            if (dev->plane_fmt[plane].bytesperline == 0) {
                LOG("Compressed format detected for plane %u and no test pattern filename given.\n"
                    "The test pattern can't be generated automatically.\n", plane);
                ret = -EINVAL;
                goto done;
            }

            for (i = 0; i < dev->plane_fmt[plane].sizeimage; ++i)
                *data++ = i;
        }

        dev->patternsize[plane] = size;
    }

    ret = 0;

done:
    if (fd != -1)
        close(fd);

    LOG("exit\n");
    return ret;
}


static int video_prepare_capture(struct device *dev, int nbufs, unsigned int offset,
                 const char *filename, enum buffer_fill_mode fill)
{
    unsigned int padding;
    unsigned int i;
    int ret;

    LOG("enter\n");

    /* Allocate and map buffers. */
    padding = (fill & BUFFER_FILL_PADDING) ? 4096 : 0;
    if ((ret = video_alloc_buffers(dev, nbufs, offset, padding)) < 0) {
        LOG("video_alloc_buffers() failure\n");
        LOG("exit\n");
        return ret;
    }

    if (video_is_output(dev)) {
        ret = video_load_test_pattern(dev, filename);
        if (ret < 0) {
            LOG("video_load_test_pattern() failure\n");
            LOG("exit\n");
            return ret;
        }
    }

    /* Queue the buffers. */
    for (i = 0; i < dev->nbufs; ++i) {
        ret = video_queue_buffer(dev, i, fill);
        if (ret < 0) {
            LOG("video_queue_buffer() failure\n");
            LOG("exit\n");
            return ret;
        }
    }

    LOG("exit\n");
    return 0;
}

#define V4L_BUFFERS_DEFAULT 8
#define V4L_BUFFERS_MAX     32

static struct device dev;

int uvc_v4l2_init(IFrameQueue* frame_queue_ifc, std::string device_node, int enumerated_width, int enumerated_height, int actual_width, int actual_height) {

    // struct device dev;
    int ret;

    /* Options parsings */
    unsigned int capabilities = V4L2_CAP_VIDEO_CAPTURE;
    int do_set_time_per_frame = 1;
    int do_capture = 0;
    const struct v4l2_format_info *info;
    int do_set_format = 0;
    int no_query = 0;
    // char *endptr;

    /* Video buffers */
    enum v4l2_memory memtype = V4L2_MEMORY_MMAP;
    unsigned int pixelformat = V4L2_PIX_FMT_YUYV;
    unsigned int stride = 0;
    unsigned int quality = (unsigned int)-1;
    unsigned int userptr_offset = 0;
    unsigned int nbufs = V4L_BUFFERS_DEFAULT;
    //unsigned int buffer_size = 0;
    enum v4l2_field field = V4L2_FIELD_ANY;
    struct v4l2_fract time_per_frame = {1, 30};
    // struct v4l2_fract time_per_frame = {1, 5};


    /* Capture loop */
    enum buffer_fill_mode fill_mode = BUFFER_FILL_NONE;
    const char *filename = "frame-#.bin";

    frame_queue_ifc_ = frame_queue_ifc;

    video_init(&dev);

    do_set_format = 1;

    std::string f_optarg;
    // fourcc
    f_optarg = "YUYV";

    info = v4l2_format_by_name(f_optarg.c_str());
    if (info == NULL) {
        LOG("Unsupported video format '%s'\n", f_optarg.c_str());
        return 1;
    }
    pixelformat = info->fourcc;
    LOG("%d > fourcc : %s\n", __LINE__, f_optarg.c_str());

    LOG("%d > enumerated_height : %d, enumerated_width : %d\n", __LINE__, enumerated_height, enumerated_width);

    // number of v4l2 buffers in kernel queue
    nbufs = 4;
    if (nbufs > V4L_BUFFERS_MAX) nbufs = V4L_BUFFERS_MAX;

    // video device node
    if (!video_has_fd(&dev)) {

        ret = video_open(&dev, device_node.c_str());
        // uvc camera device node at /dev/videoX
        if (ret < 0) return 1;
    } else {
        LOG("video_has_fd() success\n");
    }

    if (!no_query) {
        LOG("!no_query\n");
        ret = video_querycap(&dev, &capabilities);
        if (ret < 0)
            return 1;
    }

    // capture, output, ...
    ret = cap_get_buf_type(capabilities);
    if (ret < 0)
        return 1;


    if (!video_has_valid_buf_type(&dev))
        video_set_buf_type(&dev, (v4l2_buf_type)ret);

    dev.memtype = memtype;

    /* Set the video format. */
    if (do_set_format) {
        if (video_set_format(&dev, enumerated_width, enumerated_height, pixelformat, stride, field) < 0) {
            video_close(&dev);
            return 1;
        }
    }

    if (!no_query || do_capture)
        video_get_format(&dev);

    /* Set the frame rate. */
    if (do_set_time_per_frame) {
        if (video_set_framerate(&dev, &time_per_frame) < 0) {
            video_close(&dev);
            return 1;
        }
    }

    /* Set the compression quality. */
    if (video_set_quality(&dev, quality) < 0) {
        video_close(&dev);
        return 1;
    }

    if (video_prepare_capture(&dev, nbufs, userptr_offset, filename, fill_mode)) {
        video_close(&dev);
        return 1;
    }

    /* Start streaming. */
    ret = video_enable(&dev, 1);
    if (ret < 0) {
        LOG("video_enable() failure\n");
        LOG("exit\n");
        video_free_buffers(&dev);
        return 1;
    }

    return 0;
}

int shift_down() {
    if (g_shift) g_shift--;
}

int shift_up() {
    if (g_shift < 8) g_shift++;
}

int uvc_v4l2_get_frame(void) {

    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    struct v4l2_buffer buf;
    int ret;

    /* Dequeue a buffer. */
    memset(&buf, 0, sizeof buf);
    memset(planes, 0, sizeof planes);

    buf.type = dev.type;
    buf.memory = dev.memtype;
    buf.length = VIDEO_MAX_PLANES;
    buf.m.planes = planes;

    ret = ioctl(dev.fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        if (errno != EIO) {
            LOG("Unable to dequeue buffer: %s (%d).\n", strerror(errno), errno);
            //goto done;
            return -1;
        }
        buf.type = dev.type;
        buf.memory = dev.memtype;
        if (dev.memtype == V4L2_MEMORY_USERPTR) {
            // TODO reivew the use over userptr in error cases ...
            //video_buffer_fill_userptr(&dev, &(dev.buffers[i]), &buf);
        }
    }

    // render image
    uint8_t* buffer = (uint8_t*)(dev.buffers[buf.index].mem[0]);

    int width = camera_config.width_actual;
    int height = camera_config.height;
    int bytes_per_pixel = camera_config.bytes_per_pixel;
    int bytes_per_row = bytes_per_pixel * width;

    // try libuvc macros
    // uvc_error_t uvc_yuyv2rgb(uvc_frame_t *in, uvc_frame_t *out)

    uint8_t *pyuv = buffer;
// TODO initally return yuv and decode in opencv module
// allow client to request RGB formats later ...
#if 1
    // uint8_t *prgb = g_rgb_buffer; 
    IFrameQueue::Frame frame = frame_queue_ifc_->GetFrame();
    uint8_t* yuyv_buffer = frame.buffer;
    width = dev.width;
    height = dev.height;

    memcpy(yuyv_buffer, pyuv, width*height*2);
#else
    uint8_t *prgb = g_rgb_buffer; 
    width = frame.frame_size.width;
    height = frame.frame_size.height;
    // TODO allocation units for stride

    //uint8_t *prgb_end = prgb + out->data_bytes;
    uint8_t *prgb_end = prgb + (width*3*height);//out->data_bytes;

    while (prgb < prgb_end) {
        IYUYV2RGB_8(pyuv, prgb);

        prgb += 3 * 8;
        pyuv += 2 * 8;
    }
#endif

    buffer_fill_mode fill_mode = BUFFER_FILL_NONE;
    ret = video_queue_buffer(&dev, buf.index, fill_mode);
    if (ret < 0) {
        LOG("Unable to requeue buffer: %s (%d).\n",
            strerror(errno), errno);
        return -1;
    }

    return 0;
}

int uvc_v4l2_exit(void) {

    LOG("uvc_v4l2_exit() entry\n");

    // signal stop streaming

    // joing

    /* Stop streaming. */
    video_enable(&dev, 0);

    video_free_buffers(&dev);

    video_close(&dev);

    LOG("uvc_v4l2_exit() exit\n");

    return 0;
}
