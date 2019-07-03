#include <malloc.h>
#include "librtmp-jni.h"
#include "rtmp.h"
#include <android/log.h>
//
// Created by faraklit on 01.01.2016.
//


#define  LOG_TAG    "rtmp-jni"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

//RTMP *rtmp = NULL;

JNIEXPORT jlong JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_nativeAlloc(JNIEnv* env, jobject thiz) {
    RTMP *rtmp = RTMP_Alloc();
    return (jlong)rtmp;
}

/*
 * Class:     net_butterflytv_rtmp_client_RtmpClient
 * Method:    open
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_nativeOpen(JNIEnv* env, jobject thiz, jstring url_,
                                                        jboolean isPublishMode, jlong rtmpPointer,
                                                        jint sendTimeoutInMs, jint receiveTimeoutInMs) {

    const char *url = (*env)->GetStringUTFChars(env, url_, NULL);
    RTMP *rtmp = (RTMP *) rtmpPointer;
   // rtmp = RTMP_Alloc();
    if (rtmp == NULL) {
        throwIllegalStateException(env, "RTMP open called without allocating rtmp object");
        return RTMP_ERROR_IGNORED;
    }

    rtmp->m_error = RTMP_SUCCESS;
    RTMP_Init(rtmp);
    rtmp->Link.receiveTimeoutInMs = receiveTimeoutInMs;
    rtmp->Link.sendTimeoutInMs = sendTimeoutInMs;
    int ret = RTMP_SetupURL(rtmp, url);

    if (!ret) {
        RTMP_Free(rtmp);
        return rtmp->m_error;
    }
    if (isPublishMode) {
        RTMP_EnableWrite(rtmp);
    }

    ret = RTMP_Connect(rtmp, NULL);
    if (!ret) {
        RTMP_Free(rtmp);
        return rtmp->m_error;
    }
    ret = RTMP_ConnectStream(rtmp, 0);

    if (!ret) {
        return RTMP_ERROR_OPEN_CONNECT_STREAM;
    }
    (*env)->ReleaseStringUTFChars(env, url_, url);
    return RTMP_SUCCESS;
}

/*
 * Class:     net_butterflytv_rtmp_client_RtmpClient
 * Method:    read
 * Signature: ([CI)I
 */
JNIEXPORT jint JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_nativeRead(JNIEnv* env, jobject thiz, jbyteArray data_,
                                                        jint offset, jint size, jlong rtmpPointer) {

    RTMP *rtmp = (RTMP *) rtmpPointer;
    if (rtmp == NULL) {
        throwIllegalStateException(env, "RTMP open function has to be called before read");
        return RTMP_ERROR_IGNORED;
    }

    rtmp->m_error = RTMP_SUCCESS;
    int connected = RTMP_IsConnected(rtmp);
    if (!connected) {
        return RTMP_ERROR_CONNECTION_LOST;
    }

    char* data = malloc(size);

    int readCount = RTMP_Read(rtmp, data, size);

    if (readCount > 0) {
        (*env)->SetByteArrayRegion(env, data_, offset, readCount, data);  // copy
    }
    free(data);
    if (readCount <= 0) {
        switch (readCount) {
            /* For return values READ_EOF and READ_COMPLETE, return -1 to indicate stream is
             * complete. */
            case RTMP_READ_EOF:
            case RTMP_READ_COMPLETE:
                return RTMP_READ_DONE;

            case RTMP_READ_IGNORE:
            case RTMP_READ_ERROR:
            default:
                return rtmp->m_error;
        }
    }
    return readCount;
}

/*
 * Class:     net_butterflytv_rtmp_client_RtmpClient
 * Method:    write
 * Signature: ([CI)I
 */
JNIEXPORT jint JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_nativeWrite(JNIEnv* env, jobject thiz, jbyteArray data,
                                                         jint offset, jint size, jlong rtmpPointer) {

    RTMP *rtmp = (RTMP *) rtmpPointer;
    if (rtmp == NULL) {
        throwIllegalStateException(env, "RTMP open function has to be called before write");
        return RTMP_ERROR_IGNORED;
    }

    rtmp->m_error = RTMP_SUCCESS;
    int connected = RTMP_IsConnected(rtmp);
    if (!connected) {
        return RTMP_ERROR_CONNECTION_LOST;
    }

    jbyte* buf = malloc(size);
    (*env)->GetByteArrayRegion(env, data, offset, size, buf);
    int result = RTMP_Write(rtmp, buf, size);
    free(buf);
    if (!result) {
        return rtmp->m_error;
    } else {
        return result;
    }
}

/*
 * Class:     net_butterflytv_rtmp_client_RtmpClient
 * Method:    seek
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_seek(JNIEnv* env, jobject thiz, jint seekTime) {
    return 0;
}

/*
 * Class:     net_butterflytv_rtmp_client_RtmpClient
 * Method:    pause
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_nativePause(JNIEnv* env, jobject thiz, jboolean pause,
                                                         jlong rtmpPointer) {

    RTMP *rtmp = (RTMP *) rtmpPointer;
    if (rtmp == NULL) {
        throwIllegalStateException(env, "RTMP open function has to be called before pause");
        return RTMP_ERROR_IGNORED;
    }

    rtmp->m_error = RTMP_SUCCESS;
    int paused = RTMP_Pause(rtmp, pause);
    if (!paused) {
        if (rtmp->m_error == RTMP_ERROR_SEND_PACKET_FAILED) {
          rtmp->m_error = RTMP_ERROR_PAUSE_FAIL;
        }
        return rtmp->m_error;
    }
    return RTMP_SUCCESS;
}

/*
 * Class:     net_butterflytv_rtmp_client_RtmpClient
 * Method:    close
 * Signature: ()I
 */
JNIEXPORT void JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_nativeClose(JNIEnv* env, jobject thiz, jlong rtmpPointer) {

    RTMP *rtmp = (RTMP *) rtmpPointer;
    if (rtmp != NULL) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
}


JNIEXPORT jboolean JNICALL
Java_net_butterflytv_rtmp_1client_RtmpClient_nativeIsConnected(JNIEnv* env, jobject thiz, jlong rtmpPointer) {
    RTMP *rtmp = (RTMP *) rtmpPointer;
    if (rtmp == NULL) {
        return false;
    }
    int connected = RTMP_IsConnected(rtmp);
    return connected ? true : false;
}

jint throwIllegalStateException (JNIEnv* env, char* message)
{
    jclass exception = (*env)->FindClass(env, "java/lang/IllegalStateException");
    return (*env)->ThrowNew(env, exception, message);
}
