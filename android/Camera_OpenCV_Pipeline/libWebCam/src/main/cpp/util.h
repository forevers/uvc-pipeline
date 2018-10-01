#ifndef UTIL_H
#define UTIL_H

#include <android/log.h>
#include <libgen.h>
#include <jni.h>
#include <unistd.h>

#define	NUM_ARRAY_ELEMENTS_(p) ((int) sizeof(p) / sizeof(p[0]))
#define	SAFE_DELETE_(p) { if (p) { delete (p); (p) = NULL; } }
#define	SAFE_DELETE_ARRAY_(p) { if (p) { delete [](p); (p) = NULL; } }

#if !defined(LOG_NDEBUG)

#define LOGV_(TAG, FMT, ...) __android_log_print(ANDROID_LOG_VERBOSE, "test", "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGI_(TAG, FMT, ...) __android_log_print(ANDROID_LOG_INFO, "test", "[%d*%s:%d:%s]:" FMT,	\
                            gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGW_(TAG, FMT, ...) __android_log_print(ANDROID_LOG_WARN, "test", "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGD_(TAG, FMT, ...) __android_log_print(ANDROID_LOG_DEBUG, "test", "[%d*%s:%d:%s]:" FMT,	\
                            gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGE_(TAG, FMT, ...) __android_log_print(ANDROID_LOG_ERROR, "test", "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define	LOG_FATAL_IF_(TAG, cond, ...) LOG_ALWAYS_FATAL_IF("test", cond, ## __VA_ARGS__)

#else

#define LOGV_(TAG, ...)
#define LOGI_(TAG, FMT, ...)
#define LOGW_(TAG, FMT, ...)
#define LOGD_(TAG, FMT, ...)
#define LOGE_(TAG, ...)
#define LOG_FATAL_IF_(TAG, cond, ...)

#endif

#define	LOG_ALWAYS_FATAL_IF_(TAG, cond, ...) \
				( (CONDITION(cond)) \
				? ((void)__android_log_assert(#cond, TAG, ## __VA_ARGS__)) \
				: (void)0 )
#define	LOG_ASSERT_(TAG, cond, ...) LOG_FATAL_IF(TAG, !(cond), ## __VA_ARGS__)

#define		ENTER_(TAG) LOGV_(TAG, "begin")
#define		RETURN_(TAG, code, type) {type RESULT = code; LOGV_(TAG, "end (%d)", static_cast<int>(RESULT)); return RESULT;}
#define		RET_(TAG, code) {LOGV_(TAG, "end"); return code;}
#define		EXIT_(TAG) {LOGV_(TAG, "end"); return;}
#define		PRE_EXIT_(TAG) LOGD_(TAG, "end")

// exit codes
#ifndef EXIT_FAILURE
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif
#ifndef EXIT_WARNING
#define EXIT_WARNING  2
#endif

void setVM_(JavaVM *);
JavaVM *getVM_();
JNIEnv *getEnv_();

#endif // UTIL_H
