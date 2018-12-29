#include <hardware/hwcomposer.h>

struct hwc_context_1_t {
	hwc_composer_device_1_t base;

	/* our private state goes below here */
	const private_module_t	*gralloc;

	uint32_t		vsync_period;
	int 			uevent_fd;
	int			blank;
	const hwc_procs_t	*procs;
	pthread_t		vsync_thread;
	bool			vsync_enabled;
};

#define UEVENT_MSG_LEN 2048

/* ++Huawei framebuffer stuff */
#define MAX_EDC_CHANNEL (5)

#define K3FB_IOCTL_MAGIC 'm'
#define K3FB_OVERLAY_GET	_IOR(K3FB_IOCTL_MAGIC, 135, struct overlay_info)
#define K3FB_OVERLAY_SET	_IOWR(K3FB_IOCTL_MAGIC, 136, struct overlay_info)
#define K3FB_OVERLAY_UNSET	_IOW(K3FB_IOCTL_MAGIC, 137, unsigned int)
#define K3FB_OVERLAY_PLAY	_IOW(K3FB_IOCTL_MAGIC, 138, struct overlay_data)
#define K3FB_VSYNC_INT_SET	_IOW(K3FB_IOCTL_MAGIC, 132, unsigned int)
#define K3FB_OVC_CHECK_DDR_FREQ _IOW(K3FB_IOCTL_MAGIC, 142, char *)
#define K3FB_G2D_LOCK_FREQ	_IOW(K3FB_IOCTL_MAGIC, 143, int)

enum {
	EDC_ARGB_1555 = 0,
	EDC_RGB_565,
	EDC_XRGB_8888,
	EDC_ARGB_8888,
	EDC_YUYV_I,
	EDC_UYVY_I,
	EDC_YVYU_I,
	EDC_VYUY_I,
};

enum {
    OVC_NONE,
    OVC_PARTIAL,
    OVC_FULL,
};

enum {
	EDC_RGB = 0,
	EDC_BGR,
};

struct k3fb_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct k3fb_img {
	uint32_t type;
	uint32_t phy_addr;
	uint32_t actual_width;
	uint32_t actual_height;
	uint32_t format;
	uint32_t s3d_format;
	uint32_t stride;
	uint32_t width;
	uint32_t height;
	uint32_t is_video;
	/* CONFIG_OVERLAY_COMPOSE start */
	uint32_t compose_mode;
	uint32_t bgr_fmt;
	int32_t blending;
	int32_t acquire_fence;
	int32_t release_fence;
	/* CONFIG_OVERLAY_COMPOSE end */
	//wifi display begin
	int32_t buff_slot;
	//wifi display end
};

struct overlay_data {
	uint32_t id;
	uint32_t is_graphic;
	struct k3fb_img src;
};

struct overlay_info {
	struct k3fb_rect src_rect;
	struct k3fb_rect dst_rect;
	uint32_t id;
	uint32_t is_pipe_used;
	uint32_t rotation;
	uint32_t dither;
	uint32_t hd_enable;
	/* CONFIG_OVERLAY_COMPOSE start */
	uint32_t cfg_disable;
	uint32_t is_overlay_compose;
	uint32_t need_unset;
	uint32_t overlay_num;
	/* CONFIG_OVERLAY_COMPOSE end */
	/*begin support edc  pixels algin  modified by weiyi 00175802*/
	uint32_t  clip_right;
	uint32_t  clip_bottom;
	uint32_t  clip_right_partial;
	uint32_t  clip_bottom_partial;
	/*end support edc  pixels algin  modified by weiyi 00175802*/	
};
/* --Huawei framebuffer stuff */

struct fb_overlay {
	bool enabled;
	struct overlay_info info;
	struct overlay_data data;
};

