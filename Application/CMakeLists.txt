cmake_minimum_required(VERSION 3.16)

project(IrrigationApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(Qt5_DIR E:/Qt/5.15.2/mingw81_64/lib/cmake/Qt5)

find_package(Qt5 REQUIRED COMPONENTS Quick Qml Bluetooth Charts)

set(PROJECT_SOURCES
    src/main.cpp
    src/bluetoothmanager.h
    src/bluetoothmanager.cpp
)

qt5_add_qml_module(IrrigationApp
    URI IrrigationApp
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/Card.qml
    SOURCES
        ${PROJECT_SOURCES}
)

target_link_libraries(IrrigationApp PRIVATE
    Qt5::Quick
    Qt5::Qml
    Qt5::Bluetooth
    Qt5::Charts
)

# Android
if(ANDROID)
    set(ANDROID_TARGET_SDK_VERSION 33)
    set(ANDROID_MIN_SDK_VERSION 21)

    set_property(TARGET IrrigationApp PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
endif()
