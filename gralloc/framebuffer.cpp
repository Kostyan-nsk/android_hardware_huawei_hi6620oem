#include <cutils/log.h>
#include <gralloc_priv.h>

unsigned int get_num_fb_buffers()
{
	ALOGD("****************************** %d\n", NUM_FB_BUFFERS);
	return NUM_FB_BUFFERS;
}
