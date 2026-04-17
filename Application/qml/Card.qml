import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    property string icon: ""
    property string label: ""
    property string value: "--"

    color: "white"
    radius: 8
    border.width: 1
    border.color: "#e0e0e0"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 2

        Text {
            text: icon
            font.pixelSize: 20
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: label
            font.pixelSize: 11
            color: "#90A4AE"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: value
            font.pixelSize: 16
            font.bold: true
            color: "#37474F"
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
