QT += quick qml charts androidextras

CONFIG += c++17

# Qt路径配置
QTDIR = E:/Qt/5.15.2/mingw81_64

# Android配置
ANDROID_TARGET_SDK_VERSION = 33
ANDROID_MIN_SDK_VERSION = 21

SOURCES += \
    src/main.cpp \
    src/bluetoothmanager.cpp

HEADERS += \
    src/bluetoothmanager.h

RESOURCES += qml.qrc

# Android configuration
android {
    QT += androidextras
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    ANDROID_PERMISSIONS += bluetooth bluetooth-admin
}

qmlFiles.files = qml
qmlFiles.prefix = /

INSTALLS += qmlFiles
