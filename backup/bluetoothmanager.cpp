#include "bluetoothmanager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QSettings>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

BluetoothManager *BluetoothManager::s_instance = nullptr;

BluetoothManager::BluetoothManager(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    QSettings settings("IrrigationApp", "Settings");
    m_savedMacAddress = settings.value("macAddress", "").toString();
    m_connectionStatus = "未连接";

#ifdef Q_OS_ANDROID
    // 创建Java帮助对象
    m_javaHelper = QJniObject("com/irrigation/app/BluetoothHelper");
#else
    // 桌面平台模拟连接
    QTimer::singleShot(1000, this, [this]() {
        m_connectionStatus = "已连接(桌面模拟)";
        emit connectionStatusChanged();
    });
#endif
}

BluetoothManager::~BluetoothManager()
{
#ifdef Q_OS_ANDROID
    if (m_javaHelper.isValid()) {
        m_javaHelper.callMethod<void>("disconnect");
    }
#endif
}

void BluetoothManager::startConnection()
{
#ifdef Q_OS_ANDROID
    if (m_javaHelper.isValid()) {
        m_javaHelper.callMethod<void>("startDiscovery");
        m_connectionStatus = "正在搜索设备...";
        emit connectionStatusChanged();
    }
#else
    m_connectionStatus = "正在连接...";
    emit connectionStatusChanged();
    QTimer::singleShot(1000, this, [this]() {
        m_connectionStatus = "已连接(桌面模拟)";
        emit connectionStatusChanged();
    });
#endif
}

void BluetoothManager::connectByMac(const QString &macAddress)
{
    m_savedMacAddress = macAddress;
    QSettings settings("IrrigationApp", "Settings");
    settings.setValue("macAddress", macAddress);

#ifdef Q_OS_ANDROID
    if (m_javaHelper.isValid()) {
        jstring macStr = QJniObject::fromString(macAddress).object<jstring>();
        m_javaHelper.callMethod<void>("connectByMac", "(Ljava/lang/String;)V", macStr);
        m_connectionStatus = "正在连接...";
        emit connectionStatusChanged();
    }
#else
    m_connectionStatus = "正在连接...";
    emit connectionStatusChanged();
    QTimer::singleShot(1000, this, [this]() {
        m_connectionStatus = "已连接(桌面模拟)";
        emit connectionStatusChanged();
    });
#endif
}

void BluetoothManager::sendStatusRequest()
{
#ifdef Q_OS_ANDROID
    if (m_javaHelper.isValid()) {
        m_javaHelper.callMethod<void>("sendStatus");
    }
#endif
}

void BluetoothManager::sendWaterCommand()
{
#ifdef Q_OS_ANDROID
    if (m_javaHelper.isValid()) {
        m_javaHelper.callMethod<void>("sendWater");
    }
#endif

    // 模拟ACK响应
    QTimer::singleShot(1000, this, [this]() {
        emit waterAckReceived();
    });
}

void BluetoothManager::saveMacAddress(const QString &mac)
{
    m_savedMacAddress = mac;
    QSettings settings("IrrigationApp", "Settings");
    settings.setValue("macAddress", mac);
}

QString BluetoothManager::getSavedMacAddress()
{
    return m_savedMacAddress;
}

// Java调用这些静态方法
void BluetoothManager::onDataReceived(const QByteArray &data)
{
    if (s_instance) {
        qDebug() << "Received data:" << data.toHex();

        if (data.size() < 4) return;

        // 解析hex数据: [temp, humidity, soil, light]
        s_instance->m_temperature = static_cast<uint8_t>(data[0]);
        int rawHumidity = static_cast<uint8_t>(data[1]);
        s_instance->m_humidity = rawHumidity;
        s_instance->m_soilMoisture = static_cast<uint8_t>(data[2]);
        s_instance->m_lightIntensity = static_cast<uint8_t>(data[3]);

        s_instance->m_lastUpdateTime = QDateTime::currentDateTime().toString("hh:mm:ss");

        emit s_instance->sensorDataChanged();
        emit s_instance->lastUpdateTimeChanged();
    }
}

void BluetoothManager::onConnectionChanged(int state)
{
    if (s_instance) {
        // state: 0=未连接, 1=正在连接, 2=已连接
        switch (state) {
            case 0:
                s_instance->m_connectionStatus = "连接断开";
                break;
            case 1:
                s_instance->m_connectionStatus = "正在连接...";
                break;
            case 2:
                s_instance->m_connectionStatus = "已连接";
                break;
        }
        emit s_instance->connectionStatusChanged();
    }
}

void BluetoothManager::onBluetoothDataReceived(const QByteArray &data)
{
    onDataReceived(data);
}

void BluetoothManager::onBluetoothStateChanged(int state)
{
    onConnectionChanged(state);
}
