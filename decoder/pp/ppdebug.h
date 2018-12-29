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
--  Abstract : Utility macros for debugging and tracing
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: ppdebug.h,v $
--  $Date: 2007/05/15 09:39:57 $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/
#ifndef __PPDEBUG_H__
#define __PPDEBUG_H__

/* macro for assertion, used only when _ASSERT_USED is defined */
#ifdef _ASSERT_USED
#ifndef ASSERT
#include <assert.h>
#define ASSERT(expr) assert(expr)
#endif
#else
#define ASSERT(expr)
#endif

/* macro for debug printing, used only when _PPDEBUG_PRINT is defined */
#ifdef _DEBUG_PRINT
#include <log/log.h>
#undef LOG_TAG
#define LOG_TAG "pp"
#define PPDEBUG_PRINT(fmt, args...) ALOGD(__FILE__ ":%d: " fmt, __LINE__ , ## args)
#else
#define PPDEBUG_PRINT(fmt, args...)
#endif

#endif /* __PPDEBUG_H__ */
