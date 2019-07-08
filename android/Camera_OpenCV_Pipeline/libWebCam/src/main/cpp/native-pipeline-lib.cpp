#include <jni.h>
#include <android/native_window_jni.h>
#include <string>

#include "webcam.h"
#include "util.h"
#include "opencv.h"

#include <android/native_window.h>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "renderer.h"

#define MAIN_TAG "native_pipeline_lib"


WebCam* camera_ = nullptr;

/**
 * set the value into the long field
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @param field_name
 * @params val
 */
static jlong setField_long(JNIEnv *env, jobject java_obj, const char *field_name, jlong val) {
    LOGV_(MAIN_TAG, "setField_long:");

    jclass clazz = env->GetObjectClass(java_obj);

    jclass findClass = env->FindClass ("com/ess/webcam/Camera");
    if (JNI_TRUE == env->IsInstanceOf(java_obj, findClass)) {
        LOGV_(MAIN_TAG, "is com/ess/webcam/Camera");
    } else {
        LOGV_(MAIN_TAG, "is NOT com/ess/webcam/Camera");
    }

    jfieldID field = env->GetFieldID(clazz, field_name, "J");
    if (field)
        env->SetLongField(java_obj, field, val);
    else {
        // check exception
        jthrowable exc = env->ExceptionOccurred();
        if (exc) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        LOGE_(MAIN_TAG, "__setField_long:field '%s' not found", field_name);
    }
    env->DeleteLocalRef(clazz);

    return val;
}

/** camera interface **/

static jlong nativeCameraCreate(JNIEnv *env, jobject thiz) {
    ENTER_(MAIN_TAG);

    camera_ = new WebCam();

    //setField_long(env, thiz, "nativePtr_", reinterpret_cast<jlong>(camera_));
    RETURN_(MAIN_TAG, reinterpret_cast<jlong>(camera_), jlong);
}

static jint nativeCameraPrepare(JNIEnv *env, jobject thiz,
                                jlong id_camera,
                                jint vid, jint pid, jint fd,
                                jint busNum, jint devAddr, jstring usbfs_str) {
    ENTER_(MAIN_TAG);

    int result = JNI_ERR;
    IWebcamControl* camera_control = reinterpret_cast<WebCam *>(id_camera);;

    const char *c_usbfs = env->GetStringUTFChars(usbfs_str, JNI_FALSE);
    if (camera_control && (fd > 0)) {
        result = camera_control->Prepare(vid, pid, fd, busNum, devAddr, c_usbfs);
    }
    env->ReleaseStringUTFChars(usbfs_str, c_usbfs);

    RETURN_(MAIN_TAG, result, jint);
}

static jint nativeCameraSetStatusCallback(JNIEnv *env, jobject thiz,
                                          jlong id_camera, jobject jIStatusCallback) {
    ENTER_(MAIN_TAG);

    jint result = JNI_ERR;
    IWebcamControl* camera_control = reinterpret_cast<WebCam *>(id_camera);

    if (camera_control) {
        jobject status_callback_obj = env->NewGlobalRef(jIStatusCallback);
        result = camera_control->SetStatusCallback(env, status_callback_obj);
    }

    RETURN_(MAIN_TAG, result, jint);
}

static jobject nativeCameraGetSupportedVideoModes(JNIEnv *env, jobject thiz,
                                                jlong id_camera) {
    ENTER_(MAIN_TAG);

    jstring result = NULL;
    IWebcamControl* camera_control = reinterpret_cast<WebCam *>(id_camera);

    if (camera_control) {
        char *c_str = camera_control->GetSupportedVideoModes();
        if (c_str) {
            result = env->NewStringUTF(c_str);
            free(c_str);
        }
    }

    return result;
}

static jint nativeCameraSetCaptureMode(JNIEnv *env, jobject thiz,
                                       ID_TYPE id_camera, jint width, jint height, jint min_fps, jint max_fps, jint mode) {
    ENTER_(MAIN_TAG);

    IWebcamControl* camera_control = reinterpret_cast<WebCam *>(id_camera);

    CameraFormat frame_format = static_cast<CameraFormat>(mode);
    if (camera_control) {
        if (EXIT_SUCCESS == camera_control->SetPreviewSize(width, height, min_fps, max_fps, frame_format)) {
            RETURN_(MAIN_TAG, JNI_OK, jint);
        } else {
            LOGE_(TAG, "camera_control->SetPreviewSize() failure");
        }
    } else {
        LOGE_(TAG, "camera_control == null");
    }

    RETURN_(MAIN_TAG, JNI_ERR, jint);
}

static jlong nativeCameraGetFrameAccessIfc(JNIEnv *env, jobject thiz,
                                           jlong id_camera, jint interface_number) {
    ENTER_(MAIN_TAG);

    IWebcamControl* camera_control = reinterpret_cast<WebCam *>(id_camera);

    IFrameAccessRegistration* frame_access_registration = nullptr;

    if (camera_control) {
        if (nullptr == (frame_access_registration = camera_control->GetFrameAccessIfc(interface_number))) {
            LOGE_(TAG, "camera_control->GetFrameAccessIfc() failure");
        }
    } else {
        LOGE_(TAG, "camera_control == null");
    }

    RETURN_(MAIN_TAG, reinterpret_cast<jlong>(frame_access_registration), jlong);
}

static jint nativeCameraStart(JNIEnv *env, jobject thiz,
                              jlong id_camera) {
    ENTER_(MAIN_TAG);

    IWebcamControl* camera_control = reinterpret_cast<WebCam *>(id_camera);

    if (camera_control) {

        if (EXIT_SUCCESS == camera_control->Start()) {
            RETURN_(MAIN_TAG, JNI_OK, jint);
        } else {
            LOGE_(TAG, "camera_control->Start() failure");
        }
    } else {
        LOGE_(TAG, "camera_control == null");
    }

    RETURN_(MAIN_TAG, JNI_ERR, jint);
}

static jint nativeCameraStop(JNIEnv *env, jobject thiz,
                             jlong id_camera) {
    ENTER_(MAIN_TAG);

    jint result = JNI_OK;

    IWebcamControl* camera_control = reinterpret_cast<WebCam *>(id_camera);

    if (camera_control) {

        if (CAMERA_SUCCESS != camera_control->Stop()) {
            result = JNI_ERR;
        }

        jlong null_handle = 0;
        setField_long(env, thiz, "nativePtr", null_handle);
        WebCam *camera = reinterpret_cast<WebCam *>(id_camera);
        if (camera) {
            SAFE_DELETE_(camera);
        }
    }

    RETURN_(MAIN_TAG, result, jint);
}


/** opencv interface **/

static jlong nativeOpencvCreate(JNIEnv *env, jobject thiz,
                                long id_frame_access_handle, int height, int width, jobject context) {

    ENTER_(MAIN_TAG);

    IFrameAccessRegistration* frame_access_registration = reinterpret_cast<IFrameAccessRegistration *>(id_frame_access_handle);

    OpenCV* opencv = nullptr;
    opencv = new OpenCV(frame_access_registration, width, height);

    // https://stackoverflow.com/questions/13317387/how-to-get-file-in-assets-from-android-ndk
    // https://stackoverflow.com/questions/7595324/creating-temporary-files-in-android-with-ndk/10334111

    /* asset manager from activity context */
    jclass context_class = env->GetObjectClass(context);
    jmethodID getAssets = env->GetMethodID(context_class, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject AssetManager = env->CallObjectMethod(context, getAssets);
    AAssetManager* aasset_manager = AAssetManager_fromJava(env, AssetManager);
    AAssetDir* asset_dir = AAssetManager_openDir(aasset_manager, "cascades");

    const char* filename = (const char*)NULL;
    while ((filename = AAssetDir_getNextFileName(asset_dir)) != NULL) {

        if (0 == strcmp(filename, "haarcascade_frontalface_default.xml")) {

            AAsset* asset = AAssetManager_open(aasset_manager, filename, AASSET_MODE_STREAMING);
            char buf[1024];
            int nb_read = 0;

            /* use cache directory for created files */
            jmethodID getCacheDir = env->GetMethodID(context_class, "getCacheDir", "()Ljava/io/File;");

            /* append filename to cache directory path */
            jobject file = env->CallObjectMethod(context, getCacheDir);
            jclass fileClass = env->FindClass("java/io/File");
            jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
            jstring java_file_path = (jstring)env->CallObjectMethod(file, getAbsolutePath);
            const char* c_file_path = env->GetStringUTFChars(java_file_path, NULL);
            std::string cpp_file_path(c_file_path);
            cpp_file_path += "/";
            cpp_file_path += filename;

            /* create file if it does not already exist in cache directory */
            if (access(cpp_file_path.c_str(), F_OK ) != -1 ) {
                /* file exists */
            } else {
                /* file doesn't exist */
                FILE* out = fopen(cpp_file_path.c_str(), "w");
                while ((nb_read = AAsset_read(asset, buf, 1024)) > 0)
                    fwrite(buf, nb_read, 1, out);
                fclose(out);
            }

            /* release resources */
            AAsset_close(asset);
            env->ReleaseStringUTFChars(java_file_path, c_file_path);

            break;
        }
    }

    RETURN_(MAIN_TAG, reinterpret_cast<jlong>(opencv), jlong);
}

static jlong nativeOpencvGetFrameAccessIfc(JNIEnv *env, jobject thiz,
                                           jlong id_opencv, jint interface_number) {
    ENTER_(MAIN_TAG);

    IOpenCVControl* opencv_control = reinterpret_cast<OpenCV *>(id_opencv);

    IFrameAccessRegistration* frame_access_registration = nullptr;

    if (opencv_control) {
        if (nullptr == (frame_access_registration = opencv_control->GetFrameAccessIfc(interface_number))) {
            LOGE_(TAG, "opencv_control->GetFrameAccessIfc() failure");
        }
    } else {
        LOGE_(TAG, "demux_control == null");
    }

    RETURN_(MAIN_TAG, reinterpret_cast<jlong>(frame_access_registration), jlong);
}

static jint nativeOpencvStart(JNIEnv *env, jobject thiz,
                              jlong id_opencv) {
    ENTER_(MAIN_TAG);

    IOpenCVControl* opencv_control_ifc = reinterpret_cast<OpenCV*>(id_opencv);

    if (opencv_control_ifc) {
        if (opencv_control_ifc && (EXIT_SUCCESS == opencv_control_ifc->Start())) {
            RETURN_(MAIN_TAG, JNI_OK, jint);
        } else {
            LOGE_(TAG, "renderer_control->StartRender() failure");
        }
    } else {
        LOGE_(TAG, "renderer_control == null");
    }

    RETURN_(MAIN_TAG, JNI_ERR, jint);
}

static jint nativeOpencvStop(JNIEnv *env, jobject thiz,
                             jlong id_opencv) {
    ENTER_(MAIN_TAG);

    jint result = JNI_OK;

    IOpenCVControl* opencv_control_ifc = reinterpret_cast<OpenCV*>(id_opencv);

    if (opencv_control_ifc) {
        if (CAMERA_SUCCESS != opencv_control_ifc->Stop()) {
            result = JNI_ERR;
        }
    }

    setField_long(env, thiz, "toOpencvHandle", 0);
    OpenCV* opencv = reinterpret_cast<OpenCV*>(id_opencv);
    SAFE_DELETE_(opencv);

    RETURN_(MAIN_TAG, result, jint);
}

static jint nativeOpencvCycleProcessingMode(JNIEnv *env, jobject thiz,
                                            jlong id_opencv) {
    ENTER_(MAIN_TAG);

    jint result = JNI_OK;

    IOpenCVControl* opencv_control_ifc = reinterpret_cast<OpenCV*>(id_opencv);

    if (opencv_control_ifc) {
        if (CAMERA_SUCCESS != opencv_control_ifc->CycleProcessingMode()) {
            result = JNI_ERR;
        }
    }

    RETURN_(MAIN_TAG, result, jint);
}


/** renderer interface **/

static jlong nativeCreateRenderer(JNIEnv *env, jobject thiz,
                                  long id_frame_access_handle, int height, int width) {
    ENTER_(MAIN_TAG);

    IFrameAccessRegistration* frame_access_registration = reinterpret_cast<IFrameAccessRegistration *>(id_frame_access_handle);

    Renderer* renderer_ = nullptr;

    renderer_ = new Renderer(frame_access_registration, width, height);

    RETURN_(MAIN_TAG, reinterpret_cast<jlong>(renderer_), jlong);
}

static int nativeRendererSetSurface(JNIEnv *env, jobject thiz,
                                    long id_renderer_demux, jobject jSurface) {
    ENTER_(MAIN_TAG);

    jint result = JNI_ERR;

    IRendererControl* renderer_control = reinterpret_cast<Renderer *>(id_renderer_demux);

    if (renderer_control) {
        ANativeWindow *preview_window = jSurface ? ANativeWindow_fromSurface(env, jSurface) : NULL;
        if ( EXIT_SUCCESS == renderer_control->SetSurface(preview_window)) {
                result = JNI_OK;
        }
    }

    RETURN_(MAIN_TAG, result, jint);
}

static jint nativeRendererStart(JNIEnv *env, jobject thiz,
                                jlong id_renderer) {
    ENTER_(MAIN_TAG);

    IRendererControl* renderer_control_ifc = reinterpret_cast<Renderer *>(id_renderer);

    if (renderer_control_ifc) {
        if (renderer_control_ifc && (EXIT_SUCCESS == renderer_control_ifc->Start())) {
            RETURN_(MAIN_TAG, JNI_OK, jint);
        } else {
            LOGE_(TAG, "renderer_control->StartRender() failure");
        }
    } else {
        LOGE_(TAG, "renderer_control == null");
    }

    RETURN_(MAIN_TAG, JNI_ERR, jint);
}

static jint nativeRendererStop(JNIEnv *env, jobject thiz,
                               jlong id_renderer) {
    ENTER_(MAIN_TAG);

    jint result = JNI_OK;

    IRendererControl* renderer_control = reinterpret_cast<Renderer *>(id_renderer);

    if (renderer_control) {

        if (CAMERA_SUCCESS != renderer_control->Stop()) {
            result = JNI_ERR;
            LOGE_(MAIN_TAG, "renderer_control->Stop() failure");
        }

        setField_long(env, thiz, "rendererHandle", 0);
        Renderer* renderer = reinterpret_cast<Renderer*>(id_renderer);
        SAFE_DELETE_(renderer);
    }

    RETURN_(MAIN_TAG, result, jint);
}

namespace boost {

    void throw_exception(std::exception const &e) {

    }
}

jint registerNativeMethods(JNIEnv* env, const char *class_name, JNINativeMethod *methods, int num_methods) {
    int result = 0;

    jclass clazz = env->FindClass(class_name);
    if (clazz) {
        result = env->RegisterNatives(clazz, methods, num_methods);
        if (result < 0) {
            LOGE_(MAIN_TAG, "registerNativeMethods failed(class=%s)", class_name);
        }
    } else {
        LOGE_(MAIN_TAG, "registerNativeMethods: class'%s' not found", class_name);
    }
    return result;
}

static JNINativeMethod methods[] = {
    { "nativeCreate", "()J", (void *) nativeCameraCreate },
    { "nativePrepare", "(JIIIIILjava/lang/String;)I", (void *) nativeCameraPrepare },
    { "nativeSetStatusCallback", "(JLcom/ess/webcam/IStatusCallback;)I", (void *) nativeCameraSetStatusCallback },
    { "nativeGetSupportedVideoModes", "(J)Ljava/lang/String;", (void *) nativeCameraGetSupportedVideoModes },
    { "nativeSetCaptureMode", "(JIIIII)I", (void *) nativeCameraSetCaptureMode },
    { "nativeGetFrameAccessIfc", "(JI)J", (void *) nativeCameraGetFrameAccessIfc },
    { "nativeStart", "(J)I", (void *) nativeCameraStart },
    { "nativeStop", "(J)I", (void *) nativeCameraStop },

        // TODO add camera CCI API here
};

int register_camera(JNIEnv *env) {
    LOGV_(MAIN_TAG, "register camera:");
    if (registerNativeMethods(env,
                              "com/ess/webcam/Camera",
                              methods, NUM_ARRAY_ELEMENTS_(methods)) < 0) {
    }
    return 0;
};

// opencv stage
static JNINativeMethod methods_opencv[] = {
    { "nativeCreate", "(JIILandroid/content/Context;)J", (void *) nativeOpencvCreate },
    { "nativeGetFrameAccessIfc", "(JI)J", (void *) nativeOpencvGetFrameAccessIfc },
    { "nativeStart", "(J)I", (void *) nativeOpencvStart },
    { "nativeStop", "(J)I", (void *) nativeOpencvStop },
    { "nativeCycleProcessingMode", "(J)I", (void *) nativeOpencvCycleProcessingMode }

};

int register_opencv(JNIEnv *env) {
    ENTER_(MAIN_TAG);

    if (registerNativeMethods(env,
                              "com/ess/webcam/Opencv",
                              methods_opencv, NUM_ARRAY_ELEMENTS_(methods_opencv)) < 0) {
    }

    RETURN_(MAIN_TAG, 0, int);
}

// renderer stage
static JNINativeMethod methods_renderer[] = {
    { "nativeCreate", "(JII)J", (void *) nativeCreateRenderer },
    { "nativeSetSurface", "(JLandroid/view/Surface;)I", (void *) nativeRendererSetSurface },
    { "nativeStart", "(J)I", (void *) nativeRendererStart },
    { "nativeStop", "(J)I", (void *) nativeRendererStop },
};

int register_renderer(JNIEnv *env) {
    ENTER_(MAIN_TAG);

    if (registerNativeMethods(env,
                              "com/ess/webcam/Renderer",
                              methods_renderer, NUM_ARRAY_ELEMENTS_(methods_renderer)) < 0) {
    }

    RETURN_(MAIN_TAG, 0, int);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    ENTER_(MAIN_TAG);

    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    // register native methods
    int result_camera = register_camera(env);
    int result_renderer = register_renderer(env);
    int result_opencv = register_opencv(env);

    LOGD_(MAIN_TAG, "result_camera : %d, result_renderer_demux : %d, result_opencv : %d",
          result_camera, result_renderer, result_opencv);

    setVM_(vm);

    PRE_EXIT_(MAIN_TAG);

    return JNI_VERSION_1_6;
}
