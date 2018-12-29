/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  Port component
--
------------------------------------------------------------------------------*/


#ifndef HANTRO_PORT_H
#define HANTRO_PORT_H

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include "OSAL.h"


#ifdef __cplusplus
extern "C" {
#endif 

#define BUFFER_FLAG_IN_USE      0x1
#define BUFFER_FLAG_MY_BUFFER   0x2
#define BUFFER_FLAG_IS_TUNNELED 0x4
#define BUFFER_FLAG_MARK        0x8

#ifdef ANDROID_MOD
#include "../../gralloc/gralloc_priv.h"

/*
 * MetadataBufferType defines the type of the metadata buffers that
 * can be passed to video encoder component for encoding, via Stagefright
 * media recording framework. To see how to work with the metadata buffers
 * in media recording framework, please consult HardwareAPI.h
 *
 * The creator of metadata buffers and video encoder share common knowledge
 * on what is actually being stored in these metadata buffers, and
 * how the information can be used by the video encoder component
 * to locate the actual pixel data as the source input for video
 * encoder, plus whatever other information that is necessary. Stagefright
 * media recording framework does not need to know anything specific about the
 * metadata buffers, except for receving each individual metadata buffer
 * as the source input, making a copy of the metadata buffer, and passing the
 * copy via OpenMAX API to the video encoder component.
 *
 * The creator of the metadata buffers must ensure that the first
 * 4 bytes in every metadata buffer indicates its buffer type,
 * and the rest of the metadata buffer contains the
 * actual metadata information. When a video encoder component receives
 * a metadata buffer, it uses the first 4 bytes in that buffer to find
 * out the type of the metadata buffer, and takes action appropriate
 * to that type of metadata buffers (for instance, locate the actual
 * pixel data input and then encoding the input data to produce a
 * compressed output buffer).
 *
 * The following shows the layout of a metadata buffer,
 * where buffer type is a 4-byte field of MetadataBufferType,
 * and the payload is the metadata information.
 *
 * --------------------------------------------------------------
 * |  buffer type  |          payload                           |
 * --------------------------------------------------------------
 *
 */
typedef enum {

    /*
     * kMetadataBufferTypeCameraSource is used to indicate that
     * the source of the metadata buffer is the camera component.
     */
    kMetadataBufferTypeCameraSource  = 0,

    /*
     * kMetadataBufferTypeGrallocSource is used to indicate that
     * the payload of the metadata buffers can be interpreted as
     * a buffer_handle_t.
     * So in this case,the metadata that the encoder receives
     * will have a byte stream that consists of two parts:
     * 1. First, there is an integer indicating that it is a GRAlloc
     * source (kMetadataBufferTypeGrallocSource)
     * 2. This is followed by the buffer_handle_t that is a handle to the
     * GRalloc buffer. The encoder needs to interpret this GRalloc handle
     * and encode the frames.
     * --------------------------------------------------------------
     * |  kMetadataBufferTypeGrallocSource | buffer_handle_t buffer |
     * --------------------------------------------------------------
     *
     * See the VideoGrallocMetadata structure.
     */
    kMetadataBufferTypeGrallocSource = 1,

    /*
     * kMetadataBufferTypeGraphicBuffer is used to indicate that
     * the payload of the metadata buffers can be interpreted as
     * an ANativeWindowBuffer, and that a fence is provided.
     *
     * In this case, the metadata will have a byte stream that consists of three parts:
     * 1. First, there is an integer indicating that the metadata
     * contains an ANativeWindowBuffer (kMetadataBufferTypeANWBuffer)
     * 2. This is followed by the pointer to the ANativeWindowBuffer.
     * Codec must not free this buffer as it does not actually own this buffer.
     * 3. Finally, there is an integer containing a fence file descriptor.
     * The codec must wait on the fence before encoding or decoding into this
     * buffer. When the buffer is returned, codec must replace this file descriptor
     * with a new fence, that will be waited on before the buffer is replaced
     * (encoder) or read (decoder).
     * ---------------------------------
     * |  kMetadataBufferTypeANWBuffer |
     * ---------------------------------
     * |  ANativeWindowBuffer *buffer  |
     * ---------------------------------
     * |  int fenceFd                  |
     * ---------------------------------
     *
     * See the VideoNativeMetadata structure.
     */
    kMetadataBufferTypeANWBuffer = 2,

    /* This value is used by framework, but is never used inside a metadata buffer  */
    kMetadataBufferTypeInvalid = -1,


    // Add more here...

} MetadataBufferType;

// A pointer to this struct is passed to OMX_SetParameter() when the extension index
// "OMX.google.android.index.storeMetaDataInBuffers" or
// "OMX.google.android.index.storeANWBufferInMetadata" is given.
//
// When meta data is stored in the video buffers passed between OMX clients
// and OMX components, interpretation of the buffer data is up to the
// buffer receiver, and the data may or may not be the actual video data, but
// some information helpful for the receiver to locate the actual data.
// The buffer receiver thus needs to know how to interpret what is stored
// in these buffers, with mechanisms pre-determined externally. How to
// interpret the meta data is outside of the scope of this parameter.
//
// Currently, this is used to pass meta data from video source (camera component, for instance) to
// video encoder to avoid memcpying of input video frame data, as well as to pass dynamic output
// buffer to video decoder. To do this, bStoreMetaData is set to OMX_TRUE.
//
// If bStoreMetaData is set to false, real YUV frame data will be stored in input buffers, and
// the output buffers contain either real YUV frame data, or are themselves native handles as
// directed by enable/use-android-native-buffer parameter settings.
// In addition, if no OMX_SetParameter() call is made on a port with the corresponding extension
// index, the component should not assume that the client is not using metadata mode for the port.
//
// If the component supports this using the "OMX.google.android.index.storeANWBufferInMetadata"
// extension and bStoreMetaData is set to OMX_TRUE, data is passed using the VideoNativeMetadata
// layout as defined below. Each buffer will be accompanied by a fence. The fence must signal
// before the buffer can be used (e.g. read from or written into). When returning such buffer to
// the client, component must provide a new fence that must signal before the returned buffer can
// be used (e.g. read from or written into). The component owns the incoming fenceFd, and must close
// it when fence has signaled. The client will own and close the returned fence file descriptor.
//
// If the component supports this using the "OMX.google.android.index.storeMetaDataInBuffers"
// extension and bStoreMetaData is set to OMX_TRUE, data is passed using VideoGrallocMetadata
// (the layout of which is the VideoGrallocMetadata defined below). Camera input can be also passed
// as "CameraSource", the layout of which is vendor dependent.
//
// Metadata buffers are registered with the component using UseBuffer calls, or can be allocated
// by the component for encoder-metadata-output buffers.
struct StoreMetaDataInBuffersParams {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStoreMetaData;
};

// Meta data buffer layout used to transport output frames to the decoder for
// dynamic buffer handling.
struct VideoGrallocMetadata {
    MetadataBufferType eType;               // must be kMetadataBufferTypeGrallocSource
#ifdef OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
    OMX_PTR pHandle;
#else
    buffer_handle_t pHandle;
#endif
};

// Meta data buffer layout used by Huawei for CameraSource
struct CameraSourceMetadata {
    OMX_U32 magic;
    OMX_U32 phys_addr;
    OMX_U8 *virt_addr;
    OMX_U32 buffer_id;
    OMX_U32 width;
    OMX_U32 height;
    OMX_U64 padding;
};

#endif // ANDROID_MOD


typedef struct BUFFER
{
    OMX_BUFFERHEADERTYPE* header;
    OMX_BUFFERHEADERTYPE  headerdata;
    OMX_U32               flags;
    OMX_U32               allocsize;
    OSAL_BUS_WIDTH        bus_address;
    OMX_U8*               bus_data;
} BUFFER;


typedef struct BUFFERLIST
{
    BUFFER** list;
    OMX_U32  size; // list size
    OMX_U32  capacity;
}BUFFERLIST;

OMX_ERRORTYPE HantroOmx_bufferlist_init(BUFFERLIST* list, OMX_U32 size);
OMX_ERRORTYPE HantroOmx_bufferlist_reserve(BUFFERLIST* list, OMX_U32 newsize);
void          HantroOmx_bufferlist_destroy(BUFFERLIST* list);
OMX_U32       HantroOmx_bufferlist_get_size(BUFFERLIST* list);
OMX_U32       HantroOmx_bufferlist_get_capacity(BUFFERLIST* list);
BUFFER**      HantroOmx_bufferlist_at(BUFFERLIST* list, OMX_U32 i);
void          HantroOmx_bufferlist_remove(BUFFERLIST* list, OMX_U32 i);
void          HantroOmx_bufferlist_clear(BUFFERLIST* list);
OMX_BOOL      HantroOmx_bufferlist_push_back(BUFFERLIST* list, BUFFER* buff);


typedef struct PORT
{
    OMX_PARAM_PORTDEFINITIONTYPE def;          // OMX port definition
    OMX_TUNNELSETUPTYPE          tunnel;
    OMX_HANDLETYPE               tunnelcomp;   // handle to the tunneled component
    OMX_U32                      tunnelport;   // port index of the tunneled components port
    BUFFERLIST                   buffers;      // buffers for this port
    BUFFERLIST                   bufferqueue;  // buffers queued up for processing
    OMX_HANDLETYPE               buffermutex;  // mutex to protect the buffer queue
    OMX_HANDLETYPE               bufferevent;  // event object for buffer queue
    OMX_BOOL                     StoreMetadata;
} PORT;


// nBufferCountMin is a read-only field that specifies the minimum number of
// buffers that the port requires. The component shall define this non-zero default
// value.

// nBufferCountActual represents the number of buffers that are required on
// this port before it is populated, as indicated by the bPopulated field of this
// structure. The component shall set a default value no less than
// nBufferCountMin for this field.

// nBufferSize is a read-only field that specifies the minimum size in bytes for
// buffers that are allocated for this port. .
OMX_ERRORTYPE HantroOmx_port_init(PORT* p, OMX_U32 nBufferCountMin, OMX_U32 nBufferCountActual, OMX_U32 nBuffers, OMX_U32 buffersize);

void     HantroOmx_port_destroy(PORT* p);

OMX_BOOL HantroOmx_port_is_allocated(PORT* p);

OMX_BOOL HantroOmx_port_is_ready(PORT* p);

OMX_BOOL HantroOmx_port_is_enabled(PORT* p);

// Return true if port has allocated buffers, otherwise false.
OMX_BOOL HantroOmx_port_has_buffers(PORT* p);

OMX_BOOL HantroOmx_port_is_supplier(PORT* p);
OMX_BOOL HantroOmx_port_is_tunneled(PORT* p);

OMX_BOOL HantroOmx_port_has_all_supplied_buffers(PORT* p);

void     HantroOmx_port_setup_tunnel(PORT* p, OMX_HANDLETYPE comp, OMX_U32 port, OMX_BUFFERSUPPLIERTYPE type);


BUFFER*  HantroOmx_port_find_buffer(PORT* p, OMX_BUFFERHEADERTYPE* header);


// Try to allocate next available buffer from the array of buffers associated with
// with the port. The finding is done by looking at the associated buffer flags and
// checking the BUFFER_FLAG_IN_USE flag.
//
// Returns OMX_TRUE if next buffer could be found. Otherwise OMX_FALSE, which
// means that all buffer headers are in use. 
OMX_BOOL HantroOmx_port_allocate_next_buffer(PORT* p, BUFFER** buff);



// 
OMX_BOOL HantroOmx_port_release_buffer(PORT* p, BUFFER* buff);

//
OMX_BOOL HantroOmx_port_release_all_allocated(PORT* p);


// Return how many buffers are allocated for this port.
OMX_U32 HantroOmx_port_buffer_count(PORT* p);


// Get an allocated buffer. 
OMX_BOOL HantroOmx_port_get_allocated_buffer_at(PORT* p, BUFFER** buff, OMX_U32 i);


/// queue functions


// Push next buffer into the port's buffer queue. 
OMX_ERRORTYPE HantroOmx_port_push_buffer(PORT* p, BUFFER* buff);


// Get next buffer from the port's buffer queue. 
OMX_BOOL HantroOmx_port_get_buffer(PORT* p, BUFFER** buff);


// Get a buffer at a certain location from port's buffer queue
OMX_BOOL HantroOmx_port_get_buffer_at(PORT* P, BUFFER** buff, OMX_U32 i);


// Pop off the first buffer from the port's buffer queue
OMX_BOOL      HantroOmx_port_pop_buffer(PORT* p);

// Lock the buffer queue
OMX_ERRORTYPE HantroOmx_port_lock_buffers(PORT* p);
// Unlock the buffer queue
OMX_ERRORTYPE HantroOmx_port_unlock_buffers(PORT* p);


// Return how many buffers are queued for this port.
OMX_U32 HantroOmx_port_buffer_queue_count(PORT* p);

void HantroOmx_port_buffer_queue_clear(PORT* p);

#ifdef __cplusplus
} 
#endif
#endif // HANTRO_PORT_H
