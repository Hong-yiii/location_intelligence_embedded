
#ifndef __DBG_LOGGING_H_
#define __DBG_LOGGING_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
/*
 * Dbg_GetTimestamp and possibly Dbg_Get32KTimestamp functions are weak within
 * connectivity framework so need to be redefined to obtain proper timing
 * information
 */

void DbgLogDump(bool stop);

void DbgLogAdd(const char * function, const char *fmt, int n, ...);


void DbgLogInit(uint32_t log_buffer, uint32_t sz);
void DbgLogSetLock(bool lock);
void DbgLogSetWrapPrevention(bool prevent_wraplock);

void DbgLogGetStartAddrAndSize(uint32_t *addr, uint32_t *sz);


#define DEBUG_DWT_CYCLE_CNT_START() \
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; \
    DWT->CYCCNT = 0;     /* Reset cycle counter */ \
    DWT->CTRL  |= 1 ;     /* enable cycle counter */

#define DEBUG_DWT_CYCLE_CNT_GET(_CYC_) \
		_CYC_ = DWT->CYCCNT;

#define _GET_FIRST_ARG(_1, ...) _1

#define VA_NUM_ARGS_IMPL( _0, _1, _2, _3, _4, _5,  _6, _7, _8, N, ...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(17, ##__VA_ARGS__, 8,  7,  6,  5,  4,  3,  2,  1,  0, 0)

#define DBG_LOG_ADD(COND, FMT,  ...)             \
    do  {                                         \
        if (COND)                                 \
            DbgLogAdd((const char*)__FUNCTION__,  \
                      (const char*)(FMT),         \
                      VA_NUM_ARGS(__VA_ARGS__),   \
                     __VA_ARGS__);               \
    } while (0)


extern void dump_octet_string(const char * str, const unsigned char * data, size_t len);


#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif  /*  __DBG_LOGGING_H_ */
