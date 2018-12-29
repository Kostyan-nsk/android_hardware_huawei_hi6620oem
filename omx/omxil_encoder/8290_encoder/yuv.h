//#define __USE_GNU 1
#include <sched.h>
//#undef __USE_GNU
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <semaphore.h>
#include <log/log.h>

//#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))

struct thread_args {
    const uint8_t *src_argb;
    uint8_t *dst_y;
    uint8_t *dst_u;
    uint8_t *dst_v;
    int src_stride_argb;
    int dst_stride_y;
    int dst_stride_u;
    int dst_stride_v;
    int width;
    int height;
    int cpus;
//    void (*ABGRToYRow)(const uint8_t *src_abgr, uint8_t *dst_y, int pix);
//    void (*ABGRToUVRow)(const uint8_t *src_abgr0, int src_stride_abgr, uint8_t *dst_u, uint8_t *dst_v, int width);
};

static sem_t sem;

#define RGBTOUV(QB, QG, QR)                                                 \
  "vmul.s16   q8, " #QB ", q10               \n" /* B                    */ \
  "vmls.s16   q8, " #QG ", q11               \n" /* G                    */ \
  "vmls.s16   q8, " #QR ", q12               \n" /* R                    */ \
  "vadd.u16   q8, q8, q15                    \n" /* +128 -> unsigned     */ \
  "vmul.s16   q9, " #QR ", q10               \n" /* R                    */ \
  "vmls.s16   q9, " #QG ", q14               \n" /* G                    */ \
  "vmls.s16   q9, " #QB ", q13               \n" /* B                    */ \
  "vadd.u16   q9, q9, q15                    \n" /* +128 -> unsigned     */ \
  "vqshrn.u16  d0, q8, #8                    \n" /* 16 bit to 8 bit U    */ \
  "vqshrn.u16  d1, q9, #8                    \n" /* 16 bit to 8 bit V    */

void ABGRToUVRow_NEON(const uint8_t *src_abgr,
                      int src_stride_abgr,
                      uint8_t *dst_u,
                      uint8_t *dst_v,
                      int width) {
  asm volatile (
    "add        %1, %0, %1                     \n"  // src_stride + src_abgr
    "vmov.s16   q10, #112 / 2                  \n"  // UB / VR 0.875 coefficient
    "vmov.s16   q11, #74 / 2                   \n"  // UG -0.5781 coefficient
    "vmov.s16   q12, #38 / 2                   \n"  // UR -0.2969 coefficient
    "vmov.s16   q13, #18 / 2                   \n"  // VB -0.1406 coefficient
    "vmov.s16   q14, #94 / 2                   \n"  // VG -0.7344 coefficient
    "vmov.u16   q15, #0x8080                   \n"  // 128.5
    "1:                                        \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 ABGR pixels.
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 ABGR pixels.
    "vpaddl.u8  q2, q2                         \n"  // B 16 bytes -> 8 shorts.
    "vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
    "vpaddl.u8  q0, q0                         \n"  // R 16 bytes -> 8 shorts.
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  // load 8 more ABGR pixels.
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  // load last 8 ABGR pixels.
    "vpadal.u8  q2, q6                         \n"  // B 16 bytes -> 8 shorts.
    "vpadal.u8  q1, q5                         \n"  // G 16 bytes -> 8 shorts.
    "vpadal.u8  q0, q4                         \n"  // R 16 bytes -> 8 shorts.
    "vrshr.u16  q0, q0, #1                     \n"  // 2x average
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"
    "subs       %4, %4, #16                    \n"  // 32 processed per loop.
    RGBTOUV(q2, q1, q0)
    "vst1.8     {d0}, [%2]!                    \n"  // store 8 pixels U.
    "vst1.8     {d1}, [%3]!                    \n"  // store 8 pixels V.
    "bgt        1b                             \n"
  : "+r"(src_abgr),  // %0
    "+r"(src_stride_abgr),  // %1
    "+r"(dst_u),     // %2
    "+r"(dst_v),     // %3
    "+r"(width)        // %4
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void ABGRToYRow_NEON(const uint8_t *src_abgr, uint8_t *dst_y, int width) {
  asm volatile(
      "vmov.u8    d4, #33                        \n"  // R * 0.2578 coefficient
      "vmov.u8    d5, #65                        \n"  // G * 0.5078 coefficient
      "vmov.u8    d6, #13                        \n"  // B * 0.1016 coefficient
      "vmov.u8    d7, #16                        \n"  // Add 16 constant
      "1:                                        \n"
      "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 8 pixels of ABGR.
      "subs       %2, %2, #8                     \n"  // 8 processed per loop.
      "vmull.u8   q8, d0, d4                     \n"  // R
      "vmlal.u8   q8, d1, d5                     \n"  // G
      "vmlal.u8   q8, d2, d6                     \n"  // B
      "vqrshrun.s16 d0, q8, #7                   \n"  // 16 bit to 8 bit Y
      "vqadd.u8   d0, d7                         \n"
      "vst1.8     {d0}, [%1]!                    \n"  // store 8 pixels Y.
      "bgt        1b                             \n"
      : "+r"(src_abgr),  // %0
        "+r"(dst_y),     // %1
        "+r"(width)      // %2
      :
      : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "q8");
}
/*
static __inline int RGBToY(uint8_t r, uint8_t g, uint8_t b) {
    return (( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
}

static __inline int RGBToU(uint8_t r, uint8_t g, uint8_t b) {
    return ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
}
static __inline int RGBToV(uint8_t r, uint8_t g, uint8_t b) {
    return ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
}

static void ABGRToYRow_C(const uint8_t *src_argb0, uint8_t *dst_y, int width) {
    int x;

    for (x = 0; x < width; ++x) {
	dst_y[0] = RGBToY(src_argb0[0], src_argb0[1], src_argb0[2]);
	src_argb0 += 4;
	dst_y += 1;
    }
}

static void ABGRToUVRow_C(const uint8_t *src_rgb0, int src_stride_rgb, uint8_t *dst_u, uint8_t *dst_v, int width) {
    const uint8_t *src_rgb1 = src_rgb0 + src_stride_rgb;
    int x;

    for (x = 0; x < width - 1; x += 2) {
	uint8_t ab = (src_rgb0[2] + src_rgb0[6] + src_rgb1[2] + src_rgb1[6]) >> 2;
	uint8_t ag = (src_rgb0[1] + src_rgb0[5] + src_rgb1[1] + src_rgb1[5]) >> 2;
	uint8_t ar = (src_rgb0[0] + src_rgb0[4] + src_rgb1[0] + src_rgb1[4]) >> 2;
	dst_u[0] = RGBToU(ar, ag, ab);
	dst_v[0] = RGBToV(ar, ag, ab);
	src_rgb0 += 8;
	src_rgb1 += 8;
	dst_u += 1;
	dst_v += 1;
    }
}

void ABGRToYRow_Any_NEON(const uint8_t *src_argb, uint8_t *dst_y, int width) {
    int n = width & ~0x07;

    if (n > 0)
	ABGRToYRow_NEON(src_argb, dst_y, n);
    ABGRToYRow_C(src_argb + n * 4, dst_y + n, width & 0x07);
    }

void ABGRToUVRow_Any_NEON(const uint8_t *src_argb, int src_stride_argb, uint8_t *dst_u, uint8_t *dst_v, int width) {
    int n = width & ~0x0F;

    if (n > 0)
	ABGRToUVRow_NEON(src_argb, src_stride_argb, dst_u, dst_v, n);
    ABGRToUVRow_C(src_argb  + n * 4, src_stride_argb, dst_u + (n >> 1), dst_v + (n >> 1), width & 0x0F);
}
*/
static inline void setaffinity(int cpu) {
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) < 0)
	ALOGE("setaffinity: failed to set affinity on cpu %d", cpu);
}

static void *y1_thread(void *data) {

    struct thread_args *args = (struct thread_args *)data;
    struct sched_param param = {0};
    int y, err;
    const uint8_t *src_argb = args->src_argb;
    int src_stride_argb = args->src_stride_argb;
    uint8_t *dst_y = args->dst_y;
    int dst_stride_y = args->dst_stride_y;
    int width = args->width;
    int height = args->height;

    param.sched_priority = 1;
    err = sched_setscheduler(0, SCHED_FIFO, &param);
    if (err != 0)
	ALOGE("y2_thread: failed to set priority");
    if (args->cpus == 4)
	setaffinity(1);

    for (y = 0; y < height - 1; y += 2) {
	ABGRToYRow_NEON(src_argb, dst_y, width);
//	args->ABGRToYRow(src_argb, dst_y, width);
	src_argb += src_stride_argb * 2;
	dst_y += dst_stride_y * 2;
    }

    sem_post(&sem);
    return NULL;
}

static void *y2_thread(void *data) {

    struct thread_args *args = (struct thread_args *)data;
    struct sched_param param = {0};
    int y, err;
    const uint8_t *src_argb = args->src_argb;
    int src_stride_argb = args->src_stride_argb;
    uint8_t *dst_y = args->dst_y;
    int dst_stride_y = args->dst_stride_y;
    int width = args->width;
    int height = args->height;

    param.sched_priority = 1;
    err = sched_setscheduler(0, SCHED_FIFO, &param);
    if (err != 0)
	ALOGE("y2_thread: failed to set priority");
    if (args->cpus == 4)
	setaffinity(2);

    for (y = 0; y < height - 1; y += 2) {
	ABGRToYRow_NEON(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
//	args->ABGRToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
	src_argb += src_stride_argb * 2;
	dst_y += dst_stride_y * 2;
    }

    sem_post(&sem);
    return NULL;
}

static void *uv_thread(void *data) {

    struct thread_args *args = (struct thread_args *)data;
    struct sched_param param = {0};
    int y, err;
    const uint8_t *src_argb = args->src_argb;
    int src_stride_argb = args->src_stride_argb;
    uint8_t *dst_u = args->dst_u;
    uint8_t *dst_v = args->dst_v;
    int dst_stride_u = args->dst_stride_u;
    int dst_stride_v = args->dst_stride_v;
    int width = args->width;
    int height = args->height;

    param.sched_priority = 1;
    err = sched_setscheduler(0, SCHED_FIFO, &param);
    if (err != 0)
	ALOGE("uv_thread: failed to set priority");
    if (args->cpus == 4)
	setaffinity(3);

    for (y = 0; y < height - 1; y += 2) {
	ABGRToUVRow_NEON(src_argb, src_stride_argb, dst_u, dst_v, width);
//	args->ABGRToUVRow(src_argb, src_stride_argb, dst_u, dst_v, width);
	src_argb += src_stride_argb * 2;
	dst_u += dst_stride_u;
	dst_v += dst_stride_v;
    }
    sem_post(&sem);
    return NULL;
}

static int ABGRToI420_multithread(const uint8_t *src_argb, int src_stride_argb,
               uint8_t *dst_y, int dst_stride_y,
               uint8_t *dst_u, int dst_stride_u,
               uint8_t *dst_v, int dst_stride_v,
               int width, int height) {
    int i, err, policy;
    pthread_t tid;
    struct thread_args args;

    if (!src_argb || !dst_y || !dst_u || !dst_v || width <= 0 || height == 0)
	return -1;

    // Negative height means invert the image.
    if (height < 0) {
	height = -height;
	src_argb = src_argb + (height - 1) * src_stride_argb;
	src_stride_argb = -src_stride_argb;
    }

    args.src_argb = src_argb;
    args.dst_y = dst_y;
    args.dst_u = dst_u;
    args.dst_v = dst_v;
    args.src_stride_argb = src_stride_argb;
    args.dst_stride_y = dst_stride_y;
    args.dst_stride_u = dst_stride_u;
    args.dst_stride_v = dst_stride_v;
    args.width = width;
    args.height = height;
    args.cpus = get_nprocs();
/*
    if (IS_ALIGNED(width, 8))
	args.ABGRToYRow = ABGRToYRow_NEON;
    else
	args.ABGRToYRow = ABGRToYRow_Any_NEON;

    if (IS_ALIGNED(width, 16))
      args.ABGRToUVRow = ABGRToUVRow_NEON;
    else
	args.ABGRToUVRow = ABGRToUVRow_Any_NEON;
*/
    err = sem_init(&sem, 0, 0);
    if (err != 0)
	return err;
    err = pthread_create(&tid, NULL, y1_thread, &args);
    if (err != 0)
	return err;
    pthread_detach(tid);
    err = pthread_create(&tid, NULL, y2_thread, &args);
    if (err != 0)
	return err;
    pthread_detach(tid);
    err = pthread_create(&tid, NULL, uv_thread, &args);
    if (err != 0)
	return err;
    pthread_detach(tid);

    for (i = 0; i < 3; i++)
	sem_wait(&sem);

    sem_destroy(&sem);
    return 0;
}
