#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dm_socket/dm_socket.h>

// 全局变量，用于保存 JavaVM 指针和上行数据回调全局引用
static JavaVM* g_vm = NULL;
static jobject gDmSocketCallbackObj = NULL;  // 保存 Java 回调对象

// AT指令回调包装函数，将调用本函数，此处通过 JNI 转发到 Java 层
void at_command_handler_wrapper(int command_type, const char* data) {

    JNIEnv* env = NULL;
    int attached = 0;
    
    if ((*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) {
            return;
        }
        attached = 1;
    }
    
    jclass callbackClass = (*env)->GetObjectClass(env, gDmSocketCallbackObj);
    jmethodID methodId = (*env)->GetMethodID(env, callbackClass, "onAtCommand", "(ILjava/lang/String;)V");
    
    jstring jdata = (*env)->NewStringUTF(env, data ? data : "");
    (*env)->CallVoidMethod(env, gDmSocketCallbackObj, methodId, command_type, jdata);
    (*env)->DeleteLocalRef(env, jdata);
    
    if (attached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

// OTA指令回调包装函数，将调用本函数，此处通过 JNI 转发到 Java 层
void ota_command_handler_wrapper(int command_type, int command_id, const char* args, void* data) {
    // 明确标记该参数为"已使用"状态,避免-Wunused-parameter错误
    (void)data; 
    JNIEnv* env = NULL;
    int attached = 0;
    
    if ((*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) {
            return;
        }
        attached = 1;
    }
    
    jclass callbackClass = (*env)->GetObjectClass(env, gDmSocketCallbackObj);
    jmethodID methodId = (*env)->GetMethodID(env, callbackClass, "onOtaCommand", 
                                           "(IILjava/lang/String;Ljava/lang/Object;)V");
    
    jstring jargs = (*env)->NewStringUTF(env, args ? args : "");
    jobject jdata = NULL; // 可以根据实际需求转换data为Java对象
    
    (*env)->CallVoidMethod(env, gDmSocketCallbackObj, methodId, command_type, command_id, jargs, jdata);
    
    (*env)->DeleteLocalRef(env, jargs);
    if (jdata) (*env)->DeleteLocalRef(env, jdata);
    
    if (attached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

// LOG指令回调包装函数，将调用本函数，此处通过 JNI 转发到 Java 层
void log_command_handler_wrapper(int command_type, int command_id, const char* args, void* data) {
    // 明确标记该参数为"已使用"状态
    (void)data;
    // 实现与ota_command_handler_wrapper类似
    JNIEnv* env = NULL;
    int attached = 0;
    
    if ((*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) {
            return;
        }
        attached = 1;
    }
    
    jclass callbackClass = (*env)->GetObjectClass(env, gDmSocketCallbackObj);
    jmethodID methodId = (*env)->GetMethodID(env, callbackClass, "onLogCommand", 
                                           "(IILjava/lang/String;Ljava/lang/Object;)V");
    
    jstring jargs = (*env)->NewStringUTF(env, args ? args : "");
    jobject jdata = NULL;
    
    (*env)->CallVoidMethod(env, gDmSocketCallbackObj, methodId, command_type, command_id, jargs, jdata);
    
    (*env)->DeleteLocalRef(env, jargs);
    if (jdata) (*env)->DeleteLocalRef(env, jdata);
    
    if (attached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

// JNI 封装：Dm初始化时传入接受和发送的路径，以及回调函数
static jint nativeDmSocketInit(JNIEnv* env, jclass clazz, jstring receiveSockPath, jstring sendSockPath, jobject callbacks) {
    // 保存 Java 回调对象（全局引用），便于后续在 callback 中调用
    (void)clazz;
    if (gDmSocketCallbackObj != NULL) {
        (*env)->DeleteGlobalRef(env, gDmSocketCallbackObj);
    }
    gDmSocketCallbackObj = (*env)->NewGlobalRef(env, callbacks);

    // 转换jstring到const char*
    const char* recv_path = (*env)->GetStringUTFChars(env, receiveSockPath, NULL);
    const char* send_path = (*env)->GetStringUTFChars(env, sendSockPath, NULL);
    // 注册回调到底层音频流接口，传入包装函数，context 可置为 NULL

    // 准备C层回调结构
    dm_socket_callbacks c_callbacks = {
        .at_command_handler = callbacks ? at_command_handler_wrapper : NULL,
        .ota_command_handler = callbacks ? ota_command_handler_wrapper : NULL,
        .log_command_handler = callbacks ? log_command_handler_wrapper : NULL,
        .reserved = {0}
    };

    // 调用底层函数
    int result = dm_socket_init(recv_path, send_path, callbacks ? &c_callbacks : NULL);
    
    // 释放资源
    (*env)->ReleaseStringUTFChars(env, receiveSockPath, recv_path);
    (*env)->ReleaseStringUTFChars(env, sendSockPath, send_path);
    
    return result;
}

static void nativeDmSocketDestroy(JNIEnv* env, jclass clazz) {
    (void)clazz;
    dm_socket_destroy();
    if (gDmSocketCallbackObj != NULL) {
        (*env)->DeleteGlobalRef(env, gDmSocketCallbackObj);
        gDmSocketCallbackObj = NULL;
    }
}

// JNI 注册方法
static JNINativeMethod gMethods[] = {
    {"nativeDmSocketInit", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)I", (void*)nativeDmSocketInit},
    {"nativeDmSocketDestroy", "()V", (void*)nativeDmSocketDestroy},
};

// 动态注册 native 方法
jint initializeDmSocket(JavaVM* vm) {
    g_vm = vm;
    JNIEnv* env = NULL;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    // 根据要求挂载到 Java 类 package/class/name
    jclass clazz = (*env)->FindClass(env, ">>>package/class/name");
    if (clazz == NULL) {
        return JNI_ERR;
    }

    int methodCount = sizeof(gMethods) / sizeof(gMethods[0]);
    if ((*env)->RegisterNatives(env, clazz, gMethods, methodCount) < 0) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
