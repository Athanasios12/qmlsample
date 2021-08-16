import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.2
import QtQml 2.2

Window
{
    id: mainWindow
    visible: true
    width: 640
    height: 480
    title: qsTr("Stm32 Rpi App")
    SystemPalette { id: activePalette }
    color: activePalette.dark

    function appendTextArea(text)
    {
        commLogWindow.append(text);
    }

    Connections
    {
        target: backend
        onUpdateLogWindow:
        {
            appendTextArea(text)
        }
    }

    Rectangle
    {
        id:textAreaRect
        color: "white"
        anchors.centerIn: parent
        width:  Math.floor(parent.width / 2)
        height:  Math.floor(parent.height / 2)
        ScrollView
        {
            id:scView
            anchors.fill:parent
            TextArea
            {
                id: commLogWindow
                text:"Log Window, to start communication click Start Data Comm"
                objectName: "commLogWindow"
                anchors.fill: parent
                readOnly: true
            }
        }
    }

    Button
    {
       id: btnStartComm
       objectName: "btnStartComm"
       text: "Start Data Comm"
       anchors.left: textAreaRect.left
       y: textAreaRect.height + textAreaRect.y + 10
       onClicked: _SpiHandlerApi.onUiStartComm();

    }

    Button
    {
        id: btnRequestSensorData
        objectName: "btnRequestSensorData"
        anchors.right: textAreaRect.right
        anchors.verticalCenter: btnStartComm.verticalCenter
        text: "Get Sensor Data"
        onClicked: _SpiHandlerApi.onUiRequestSensorData();
    }
}
