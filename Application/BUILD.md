# 智能灌溉APP - 构建指南

## 环境要求

- Qt 6.5.3
- Android SDK
- Android NDK
- Qt Creator（推荐）

## Qt Creator构建

1. 打开Qt Creator
2. 文件 → 打开文件或项目
3. 选择 `IrrigationApp.pro`
4. 配置构建套件：
   - 选择 Android 构建套件
   - 配置 Android SDK 和 NDK 路径
5. 点击构建按钮（或按 Ctrl+B）
6. 连接手机或启动模拟器
7. 点击运行按钮（或按 Ctrl+R）

## 命令行构��

```bash
cd Application
E:/Qt/6.5.3/msvc2019_64/bin/qmake IrrigationApp.pro
make
make install
```

## CMake构建

```bash
cd Application
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=E:/Qt/6.5.3/msvc2019_64
make
```

## 部署到手机

确保手机：
1. 开启开发者选项
2. 开启USB调试
3. 安装好驱动

Qt Creator会自动处理APK安装和启动。

## 蓝牙权限

应用首次启动时会请求蓝牙权限，需要授予以下权限：
- BLUETOOTH_SCAN
- BLUETOOTH_CONNECT

## 调试

使用 `adb logcat` 查看日志：

```bash
adb logcat | grep IrrigationApp
```

或在Qt Creator的"应用程序输出"窗口查看。
