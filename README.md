my jni demo

__报错解决__

1. Java层要调用到native方法

```
 jni注册时报错 ：Abort message: 'JNI DETECTED ERROR IN APPLICATION: JNI NewGlobalRef called with pending exception java.lang.NoSuchMethodError: no static or non-static method "Lcom/starkylin/dmmanager/base/DMSocketManager;.nativeSendMessage(Ljava/lang/String;)I"
```

2. Android.bp配置混淆

```
报错  ：Throwing new exception 'no non-static method "Lcom/starkylin/dmmanager/DmManagerService;.onOtaCommand(IILjava/lang/String;Ljava/lang/Object;)V"' with unexpected pending exception: java.lang.NoSuchMethodError: no non-static method "Lcom/starkylin/dmmanager/DmManagerService;.onAtCommand(ILjava/lang/String;)V"
```

应用层Android.bp文件配置，proguard.flags文件示例

```
	optimize: {
        proguard_flags_files: ["proguard.flags"],
    },
```

proguard.flags
```
-keep class com.starkylin.dmmanager.** {*;}
-verbose
```
