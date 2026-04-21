#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <QObject>
#include <QTimer>
#include <QSettings>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

class BluetoothManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString lastUpdateTime READ lastUpdateTime NOTIFY lastUpdateTimeChanged)
    Q_PROPERTY(int temperature READ temperature NOTIFY sensorDataChanged)
    Q_PROPERTY(int humidity READ humidity NOTIFY sensorDataChanged)
    Q_PROPERTY(int soilMoisture READ soilMoisture NOTIFY sensorDataChanged)
    Q_PROPERTY(int lightIntensity READ lightIntensity NOTIFY sensorDataChanged)

public:
    explicit BluetoothManager(QObject *parent = nullptr);
    ~BluetoothManager();

    QString connectionStatus() const { return m_connectionStatus; }
    QString lastUpdateTime() const { return m_lastUpdateTime; }
    int temperature() const { return m_temperature; }
    int humidity() const { return m_humidity; }
    int soilMoisture() const { return m_soilMoisture; }
    int lightIntensity() const { return m_lightIntensity; }

    Q_INVOKABLE void startConnection();
    Q_INVOKABLE void sendStatusRequest();
    Q_INVOKABLE void sendWaterCommand();
    Q_INVOKABLE void connectByMac(const QString &macAddress);

    Q_INVOKABLE void saveMacAddress(const QString &mac);
    Q_INVOKABLE QString getSavedMacAddress();

    static void onDataReceived(const QByteArray &data);
    static void onConnectionChanged(int state);

signals:
    void connectionStatusChanged();
    void lastUpdateTimeChanged();
    void sensorDataChanged();
    void waterAckReceived();

private slots:
    void onBluetoothDataReceived(const QByteArray &data);
    void onBluetoothStateChanged(int state);

private:
#ifdef Q_OS_ANDROID
    QJniObject m_javaHelper;
#endif

    QString m_connectionStatus;
    QString m_lastUpdateTime;
    QString m_savedMacAddress;

    int m_temperature = 0;
    int m_humidity = 0;
    int m_soilMoisture = 0;
    int m_lightIntensity = 0;

    static BluetoothManager *s_instance;
};

#endif // BLUETOOTHMANAGER_H
