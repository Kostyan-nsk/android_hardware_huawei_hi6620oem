typedef struct __attribute__((packed)) { /**** BMP file header structure ****/
    unsigned short bfType;               /* Magic number for file */
    unsigned int   bfSize;               /* Size of file */
    unsigned short bfReserved1;          /* Reserved */
    unsigned short bfReserved2;          /* ... */
    unsigned int   bfOffBits;            /* Offset to bitmap data */
} BITMAPFILEHEADER;

#define BF_TYPE 0x4D42                   /* "MB" */

typedef struct __attribute__((packed)) { /**** BMP file info structure ****/
    unsigned int   biSize;               /* Size of info header */
    int            biWidth;              /* Width of image */
    int            biHeight;             /* Height of image */
    unsigned short biPlanes;             /* Number of color planes */
    unsigned short biBitCount;           /* Number of bits per pixel */
    unsigned int   biCompression;        /* Type of compression to use */
    unsigned int   biSizeImage;          /* Size of image data */
    int            biXPelsPerMeter;      /* X pixels per meter */
    int            biYPelsPerMeter;      /* Y pixels per meter */
    unsigned int   biClrUsed;            /* Number of colors used */
    unsigned int   biClrImportant;       /* Number of important colors */
} BITMAPINFOHEADER;

/*
 * Constants for the biCompression field...
 */

#  define BI_RGB       0                 /* No compression - straight BGR data */
#  define BI_RLE8      1                 /* 8-bit run-length compression */
#  define BI_RLE4      2                 /* 4-bit run-length compression */
#  define BI_BITFIELDS 3                 /* RGB bitmap with RGB masks */

typedef struct __attribute__((packed)) { /**** Colormap entry structure ****/
    unsigned char  rgbBlue;              /* Blue value */
    unsigned char  rgbGreen;             /* Green value */
    unsigned char  rgbRed;               /* Red value */
    unsigned char  rgbReserved;          /* Reserved */
} RGBQUAD;

typedef struct {                     /**** Bitmap information structure ****/
    BITMAPINFOHEADER bmiHeader;      /* Image header */
    RGBQUAD          bmiColors[256]; /* Image colormap */
} BITMAPINFO;