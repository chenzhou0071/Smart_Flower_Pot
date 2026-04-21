import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtCharts 2.3

Window {
    width: 360
    height: 640
    visible: true
    title: "智能灌溉系统"
    color: "#f5f5f5"

    // Connection status check
    property bool isConnected: bluetoothManager.connectionStatus === "已连接"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // ========== 标题栏 ==========
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#2196F3"
            radius: 8

            RowLayout {
                anchors.centerIn: parent
                spacing: 10

                Text {
                    text: "智能灌溉系统"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                }

                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: isConnected ? "#4CAF50" : "#f44336"
                }
            }
        }

        // ========== 数据卡片 ==========
        GridLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            columns: 4
            columnSpacing: 5

            // 温度卡片
            Card {
                icon: "🌡️"
                label: "温度"
                value: bluetoothManager.temperature + "°C"
            }

            // 湿度卡片
            Card {
                icon: "💧"
                label: "湿度"
                value: bluetoothManager.humidity + "%"
            }

            // 土壤湿度卡片
            Card {
                icon: "🌱"
                label: "土壤"
                value: bluetoothManager.soilMoisture + "%"
            }

            // 光照卡片
            Card {
                icon: "☀️"
                label: "光照"
                value: bluetoothManager.lightIntensity + "%"
            }
        }

        // ========== 折线图 ==========
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            color: "white"
            radius: 8
            border.width: 1
            border.color: "#e0e0e0"

            ChartView {
                id: chartView
                anchors.fill: parent
                anchors.margins: 10
                legend.visible: true
                legend.alignment: Qt.AlignBottom
                antialiasing: true

                ValueAxis {
                    id: axisY
                    min: 0
                    max: 100
                }

                ValueAxis {
                    id: axisX
                    min: 0
                    max: 30
                }

                LineSeries {
                    name: "温度"
                    color: "#f44336"
                    axisX: axisX
                    axisY: axisY
                }
                LineSeries {
                    name: "湿度"
                    color: "#2196F3"
                    axisX: axisX
                    axisY: axisY
                }
                LineSeries {
                    name: "土壤"
                    color: "#4CAF50"
                    axisX: axisX
                    axisY: axisY
                }
                LineSeries {
                    name: "光照"
                    color: "#FFC107"
                    axisX: axisX
                    axisY: axisY
                }
            }
        }

        // ========== 操作按钮 ==========
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            spacing: 10

            Button {
                id: refreshButton
                text: "立即刷新"
                Layout.fillWidth: true
                Layout.fillHeight: true
                enabled: isConnected && refreshCd === 0
                font.pixelSize: 16

                background: Rectangle {
                    color: refreshButton.enabled ? "#2196F3" : "#B0BEC5"
                    radius: 4
                }

                contentItem: Text {
                    text: refreshCd > 0 ? refreshCd + "s" : parent.text
                    anchors.centerIn: parent
                    color: "white"
                    font: parent.font
                }

                onClicked: {
                    bluetoothManager.sendStatusRequest()
                    startRefreshCd()
                }
            }

            Button {
                id: waterButton
                text: "立即浇水"
                Layout.fillWidth: true
                Layout.fillHeight: true
                enabled: isConnected && waterCd === 0
                font.pixelSize: 16

                background: Rectangle {
                    color: waterButton.enabled ? "#4CAF50" : "#B0BEC5"
                    radius: 4
                }

                contentItem: Text {
                    text: waterCd > 0 ? waterCd + "s" : parent.text
                    anchors.centerIn: parent
                    color: "white"
                    font: parent.font
                }

                onClicked: {
                    bluetoothManager.sendWaterCommand()
                    startWaterCd()
                }
            }
        }

        // ========== 手动连接入口 ==========
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#fff3e0"
            radius: 8
            visible: !isConnected

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                TextField {
                    id: macInput
                    placeholderText: "输入MAC地址"
                    Layout.fillWidth: true
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhPreferUppercase
                }

                Button {
                    text: "连接"
                    enabled: macInput.text.length > 0

                    background: Rectangle {
                        color: parent.enabled ? "#FF9800" : "#B0BEC5"
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        anchors.centerIn: parent
                        color: "white"
                        font: parent.font
                    }

                    onClicked: {
                        bluetoothManager.connectByMac(macInput.text)
                    }
                }
            }
        }

        // ========== 状态栏 ==========
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#eceff1"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Text {
                    text: bluetoothManager.connectionStatus
                    color: "#607D8B"
                    font.pixelSize: 14
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "更新: " + bluetoothManager.lastUpdateTime
                    color: "#90A4AE"
                    font.pixelSize: 12
                }
            }
        }
    }

    // ========== CD变量 ==========
    property int refreshCd: 0
    property int waterCd: 0

    Timer {
        id: refreshCdTimer
        interval: 1000
        repeat: true
        running: refreshCd > 0
        onTriggered: refreshCd--
    }

    Timer {
        id: waterCdTimer
        interval: 1000
        repeat: true
        running: waterCd > 0
        onTriggered: waterCd--
    }

    function startRefreshCd() {
        refreshCd = 5
    }

    function startWaterCd() {
        waterCd = 5
    }

    // ========== 启动时自动连接 ==========
    Component.onCompleted: {
        bluetoothManager.startConnection()
    }
}
