#include "bluetoothmanager.h"
#include <QAndroidJniObject>
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QSettings>

BluetoothManager::BluetoothManager(QObject *parent)
    : QObject(parent)
{
    // Load saved MAC address
    QSettings settings("IrrigationApp", "Settings");
    m_savedMacAddress = settings.value("macAddress", "").toString();

    m_connectionStatus = "未连接";

    // Register native methods
    QJniObject::registerNativeMethods("com/irrigation/app/BluetoothHelper",
        {
            {"onDataReceived", "([B)V", reinterpret_cast<void*>(BluetoothManager::onDataReceived)},
            {"onConnectionStateChanged", "(I)V", reinterpret_cast<void*>(BluetoothManager::onConnectionStateChanged)}
        }
    );
}

BluetoothManager::~BluetoothManager()
{
    if (m_bluetoothSocket.isValid()) {
        m_bluetoothSocket.callMethod<void>("close");
    }
}

void BluetoothManager::startConnection()
{
    // Get Bluetooth adapter
    QJniObject bluetoothAdapter = QJniObject::callStaticObjectMethod(
        "android/bluetooth/BluetoothAdapter",
        "getDefaultAdapter"
    );

    if (!bluetoothAdapter.isValid()) {
        m_connectionStatus = "蓝牙不可用";
        emit connectionStatusChanged();
        return;
    }

    m_connectionStatus = "正在搜索设备...";
    emit connectionStatusChanged();

    // Start discovery via Java helper
    QJniObject::callStaticMethod<void>(
        "com/irrigation/app/BluetoothHelper",
        "startDiscovery",
        "(J)V",
        reinterpret_cast<jlong>(this)
    );
}

void BluetoothManager::connectByMac(const QString &macAddress)
{
    QJniObject::callStaticMethod<void>(
        "com/irrigation/app/BluetoothHelper",
        "connectToDevice",
        "(Ljava/lang/String;J)V",
        QJniObject::fromString(macAddress).object<jstring>(),
        reinterpret_cast<jlong>(this)
    );

    m_connectionStatus = "正在连接...";
    emit connectionStatusChanged();
}

void BluetoothManager::onConnectionStateChanged(int state)
{
    BluetoothManager *manager = reinterpret_cast<BluetoothManager*>(state);

    if (state == 2) { // Connected
        manager->m_connectionStatus = "已连接";
        emit manager->connectionStatusChanged();
    } else if (state == 0) { // Disconnected
        manager->m_connectionStatus = "连接断开";
        emit manager->connectionStatusChanged();
    }
}

void BluetoothManager::onDataReceived(const QByteArray &data)
{
    BluetoothManager *manager = reinterpret_cast<BluetoothManager*>(data);

    if (data.size() < 4) return;

    // Parse hex data
    manager->m_temperature = static_cast<uint8_t>(data[0]);
    int rawHumidity = static_cast<uint8_t>(data[1]);
    manager->m_humidity = rawHumidity;
    manager->m_soilMoisture = static_cast<uint8_t>(data[2]);
    manager->m_lightIntensity = static_cast<uint8_t>(data[3]);

    // Update time
    manager->m_lastUpdateTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    emit manager->sensorDataChanged();
    emit manager->lastUpdateTimeChanged();
}

void BluetoothManager::sendStatusRequest()
{
    QByteArray cmd = "STATUS\r\n";

    QJniObject::callStaticMethod<void>(
        "com/irrigation/app/BluetoothHelper",
        "sendData",
        "([B)V",
        QJniObject::newByteArray(cmd.size(), cmd.constData()).object<jbyteArray>()
    );
}

void BluetoothManager::sendWaterCommand()
{
    QByteArray cmd = "WATER\r\n";

    QJniObject::callStaticMethod<void>(
        "com/irrigation/app/BluetoothHelper",
        "sendData",
        "([B)V",
        QJniObject::newByteArray(cmd.size(), cmd.constData()).object<jbyteArray>()
    );
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

void BluetoothManager::parseSensorData(const QByteArray &data)
{
    // Handled in onDataReceived
}
