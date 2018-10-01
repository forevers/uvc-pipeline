#include "util.h"

static JavaVM *savedVm;

void setVM_(JavaVM *vm) {
    savedVm = vm;
}

JavaVM *getVM_() {
    return savedVm;
}

JNIEnv *getEnv_() {
    JNIEnv *env = NULL;
    if (savedVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        env = NULL;
    }
    return env;
}