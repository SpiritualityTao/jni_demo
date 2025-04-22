#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dm_socket.h>
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 全局变量，用于保存 JavaVM 指针和上行数据回调全局引用
static JavaVM* g_vm = NULL;
static jobject gDmSocketCallbackObj = NULL;  // 保存 Java 回调对象


static jmethodID gOnAtCommandMethodID = NULL;
static jmethodID gOnOtaCommandMethodID = NULL;
static jmethodID gOnLogCommandMethodID = NULL;

// AT指令回调包装函数，将调用本函数，此处通过 JNI 转发到 Java 层
void at_command_handler(int command_type, const char* data) {

    LOGD("at_command_handler command_type:%d, data:%s", command_type, data);
    JNIEnv* env = NULL;
    int attached = 0;
    
    if ((*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) {
            return;
        }
        attached = 1;
    }

    if (gOnAtCommandMethodID == NULL) {
        LOGD("methodId is NULL");
        return;
    }
    jstring jdata = (*env)->NewStringUTF(env, data ? data : "");
    (*env)->CallVoidMethod(env, gDmSocketCallbackObj, gOnAtCommandMethodID, command_type, jdata);
    (*env)->DeleteLocalRef(env, jdata);
    
    if (attached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

// OTA指令回调包装函数，将调用本函数，此处通过 JNI 转发到 Java 层
void ota_command_handler(int command_type, int command_id, const char* args, void* data) {

    LOGD("ota_command_handler command_type:%d, command_id:%d, args:%s", command_type, command_id, args);
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

    if (gOnOtaCommandMethodID == NULL) {
        LOGD("methodId is NULL");
        return;
    }
    jstring jargs = (*env)->NewStringUTF(env, args ? args : "");
    jobject jdata = NULL; // 可以根据实际需求转换data为Java对象
    
    (*env)->CallVoidMethod(env, gDmSocketCallbackObj, gOnOtaCommandMethodID, command_type, command_id, jargs, jdata);
    
    (*env)->DeleteLocalRef(env, jargs);
    if (jdata) (*env)->DeleteLocalRef(env, jdata);
    
    if (attached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

// LOG指令回调包装函数，将调用本函数，此处通过 JNI 转发到 Java 层
void log_command_handler(int command_type, int command_id, const char* args, void* data) {
    LOGD("log_command_handler command_type:%d, command_id:%d, args:%s", command_type, command_id, args);
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

    if (gOnOtaCommandMethodID == NULL) {
        LOGD("methodId is NULL");
        return;
    }
    jstring jargs = (*env)->NewStringUTF(env, args ? args : "");
    jobject jdata = NULL;
    
    (*env)->CallVoidMethod(env, gDmSocketCallbackObj, gOnLogCommandMethodID, command_type, command_id, jargs, jdata);
    
    (*env)->DeleteLocalRef(env, jargs);
    if (jdata) (*env)->DeleteLocalRef(env, jdata);
    
    if (attached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

// JNI 封装：Dm初始化时传入接受和发送的路径，以及回调函数
static jint nativeDmSocketInitImpl(JNIEnv* env, jclass clazz, jstring receiveSockPath, jstring sendSockPath, jobject callbacks) {

    LOGD("nativeDmSocketInitImpl ...");
    // 保存 Java 回调对象（全局引用），便于后续在 callback 中调用
    (void)clazz;
    if (gDmSocketCallbackObj != NULL) {
        (*env)->DeleteGlobalRef(env, gDmSocketCallbackObj);
        gDmSocketCallbackObj = NULL;
    }
    gDmSocketCallbackObj = (*env)->NewGlobalRef(env, callbacks);
    // 获取类并缓存方法 ID
    jclass callbackClass = (*env)->GetObjectClass(env, gDmSocketCallbackObj);


    gOnAtCommandMethodID = (*env)->GetMethodID(env, callbackClass, "onAtCommand", "(ILjava/lang/String;)V");
    gOnOtaCommandMethodID = (*env)->GetMethodID(env, callbackClass, "onOtaCommand", "(IILjava/lang/String;Ljava/lang/Object;)V");
    gOnLogCommandMethodID = (*env)->GetMethodID(env, callbackClass, "onLogCommand", "(IILjava/lang/String;Ljava/lang/Object;)V");

    if (gOnAtCommandMethodID == NULL) {
        LOGD("gOnAtCommandMethodID methodId is NULL");
    }
    if (gOnOtaCommandMethodID == NULL) {
        LOGD("gOnOtaCommandMethodID methodId is NULL");
    }
    if (gOnLogCommandMethodID == NULL) {
        LOGD("gOnLogCommandMethodID methodId is NULL");
    }

    (*env)->DeleteLocalRef(env, callbackClass); // 删除局部引用

    // 转换jstring到const char*
    const char* recv_path = (*env)->GetStringUTFChars(env, receiveSockPath, NULL);
    const char* send_path = (*env)->GetStringUTFChars(env, sendSockPath, NULL);
    // 注册回调到底层音频流接口，传入包装函数，context 可置为 NULL

    // 准备C层回调结构
    dm_socket_callbacks c_callbacks;

    memset(&c_callbacks, 0, sizeof(dm_socket_callbacks));
    c_callbacks.at_command_handler = at_command_handler;
    c_callbacks.ota_command_handler = ota_command_handler;
    c_callbacks.log_command_handler = log_command_handler;

    // 调用底层函数
    int result = dm_socket_init(recv_path, send_path, callbacks ? &c_callbacks : NULL);
    
    // 释放资源
    (*env)->ReleaseStringUTFChars(env, receiveSockPath, recv_path);
    (*env)->ReleaseStringUTFChars(env, sendSockPath, send_path);
    LOGD("nativeDmSocketInitImpl result: %d", result);
    return result;
}

static void nativeDmSocketDestroyImpl(JNIEnv* env, jclass clazz) {

    LOGD("nativeDmSocketDestroyImpl ...");
    (void)clazz;
    dm_socket_destroy();
    if (gDmSocketCallbackObj != NULL) {
        (*env)->DeleteGlobalRef(env, gDmSocketCallbackObj);
        gDmSocketCallbackObj = NULL;
    }
}

static jint nativeSendMessageImpl(JNIEnv* env, jclass clazz, jstring jmessage) {
    LOGD("nativeSendMessageImpl ...");
    (void)clazz;
    // 1. 将jstring转换为C字符串
    const char *c_message = (*env)->GetStringUTFChars(env, jmessage, NULL);
    if (c_message == NULL) {
        return -1; // 内存不足
    }

    LOGD("nativeSendMessageImpl:%s", c_message);

    // 2. 调用底层函数
    int result = dm_socket_send_message(c_message);

    // 3. 释放字符串资源
    (*env)->ReleaseStringUTFChars(env, jmessage, c_message);
    LOGD("nativeSendMessageImpl result:%d", result);

    return result;
}

// JNI 注册方法
static JNINativeMethod gMethods[] = {
    {"nativeDmSocketInit", "(Ljava/lang/String;Ljava/lang/String;Lcom/starkylin/dmmanager/base/DMSocketManager$DMSocketCallbacks;)I", (void*)nativeDmSocketInitImpl},
    {"nativeDmSocketDestroy", "()V", (void*)nativeDmSocketDestroyImpl},
    {"nativeSendMessage", "(Ljava/lang/String;)I", (void*)nativeSendMessageImpl},
};

// 动态注册 native 方法
jint initializeDmSocket(JavaVM* vm) {
    LOGD("initializeDmSocket...");
    g_vm = vm;
    JNIEnv* env = NULL;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv error ...");
        return JNI_ERR;
    }
    // 根据要求挂载到 Java 类 com/starkylin/dmmanager/base/DMSocketManager
    jclass clazz = (*env)->FindClass(env, "com/starkylin/dmmanager/base/DMSocketManager");
    if (clazz == NULL) {
        LOGE("clazz is null ...");
        return JNI_ERR;
    }

    int methodCount = sizeof(gMethods) / sizeof(gMethods[0]);
    if ((*env)->RegisterNatives(env, clazz, gMethods, methodCount) < 0) {
        LOGE("RegisterNatives failed ...");
        return JNI_ERR;
    }

    LOGD("initializeDmSocket finish ...");
    return JNI_VERSION_1_6;
}
