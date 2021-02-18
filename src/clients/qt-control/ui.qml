import QtQuick 2.10
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4

ApplicationWindow
{
    visible: true
    width: 1280
    height: 720
    title: qsTr("Record control")

    GridLayout {
        id: mainGrid
        anchors.fill: parent

        rows: 20
        columns: 1

        Rectangle{
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.row: 1
            Layout.rowSpan: 5
            Layout.column: 1
            height: 1

            GridLayout {
                id: timesGrid
                anchors.fill: parent

                rows: 3
                columns: 2

                Rectangle{
                    color: "red"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 0
                    Layout.column: 0
                    width: 0.5
                    Text {
                        text: "Total recording time"
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 20
                    }
                }

                Rectangle{
                    color: "red"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 0
                    Layout.column: 1
                    Text {
                        id: totalRecTime
                        text: "00:00:00"
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 20
                    }
                }

                Rectangle{
                    id: rectangle
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 1
                    Layout.column: 0
                    width: 0.5
                    Text {
                        text: "Time from last sync. point"
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 20
                    }
                }

                Rectangle{
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 1
                    Layout.column: 1
                    Text {
                        id: syncRecTime
                        text: "00:00:00"
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 20
                    }
                }

                Rectangle{
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 2
                    Layout.column: 0
                    width: 0.5
                    Text {
                        text: "Current sync. point"
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 20
                    }
                }

                Rectangle{
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 2
                    Layout.column: 1
                    Text {
                        id: syncRecPoint
                        text: "-1"
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 20
                    }
                }
            }
        }

        Rectangle{
            color: "white"
            border.color: "black"
            border.width: 1

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.row: 6
            Layout.column: 1
            Layout.rowSpan: 4
            height: 1

            GridLayout {
                anchors.topMargin: 5
                anchors.leftMargin: 5
                anchors.bottomMargin: 5
                anchors.rightMargin: 5

                id: buttonGrid
                anchors.fill: parent
                columns: 5
                Button{
                    text: "Start recording"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    onClicked: {
                        baseConnections.startRecording();
                        mainTimer.running = true
                        recordMonitoring.running = true
                    }
                }

                Button{
                    text: "Stop recording"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    onClicked: {
                        baseConnections.stopRecording();
                        mainTimer.running = false
                        recordMonitoring.running = false
                    }
                }

                Button{
                    id: button
                    text: "Start sync. recording"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    onClicked: {
                        baseConnections.startSyncRecording();
                        syncTimer.running = true;
                        nextSyncPointTimer.running = true
                    }
                }

                Button{
                    text: "Stop sync. recording"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    onClicked: {
                        baseConnections.stopSyncRecording();
                        syncTimer.running = false;
                        nextSyncPointTimer.running = false
                    }
                }

                Button{
                    text: "Check all device"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    onClicked: {
                        baseConnections.checkSignal();
                    }
                }
            }
        }

        Rectangle{
            color: "white"
            border.color: "black"
            border.width: 1

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.row: 11
            Layout.column: 1
            Layout.rowSpan: 9
            height: 5

            ListView {
                id: logList
                anchors.topMargin: 5
                anchors.leftMargin: 5
                anchors.bottomMargin: 5
                anchors.rightMargin: 5

                model: ListModel {
                    id: logListModel
                }

                anchors.fill: parent
                delegate: Item {
                    id: listItem
                    anchors.left: parent.left
                    anchors.right: parent.right

                    height: 50

                    GridLayout{
                        anchors.fill: parent
                        rows: 1
                        columns: 100

                        TextArea{
                            Layout.fillHeight: true
                            Layout.fillWidth: false
                            Layout.row: 0
                            Layout.column: 0
                            Layout.columnSpan: 5
                            readOnly: true

                            horizontalAlignment: TextEdit.AlignHCenter
                            verticalAlignment: TextEdit.AlignVCenter

                            text: "Device ID = " + device_key_id

                            font.pointSize: 20
                            style: TextAreaStyle{
                                textColor: "#FF0000"
                                backgroundColor: "#BBBBBB"
                            }
                        }

                        TextArea{
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.row: 0
                            Layout.column: 6
                            Layout.columnSpan: 80
                            readOnly: true

                            horizontalAlignment: TextEdit.AlignLeft
                            verticalAlignment: TextEdit.AlignVCenter

                            textFormat: TextEdit.RichText

                            text: device_info_str

                            style: TextAreaStyle{
                                textColor: "#FF0000"
                                backgroundColor: "#BBBBBB"
                            }
                        }

                        Button{
                            Layout.fillHeight: true
                            Layout.column: 86
                            Layout.columnSpan: 14
                            Layout.row: 0
                            text: "X"
                            width: 50
                            height: 50

                            onClicked: {
                                baseConnections.onExcludeDevice(device_key_id)
                            }
                        }
                    }
                }
            }
        }
    }

    Item{
        Timer{
            id: mainTimer
            interval: 16
            running: false
            repeat: true
            onTriggered: {
                baseConnections.timerRecordEvent();
            }
        }

        Timer{
            id: syncTimer
            interval: 16
            running: false
            repeat: true
            onTriggered: {
                baseConnections.timerSyncRecordEvent();
            }
        }

        Timer{
            id: nextSyncPointTimer
            running: false
            repeat: true
            onTriggered: {
                baseConnections.emitNextSyncPoint();
            }
        }

        Timer{
            id: recordMonitoring
            interval: 100
            running: false
            repeat: true
            onTriggered: {
                baseConnections.sendProbSignal();
            }
        }
    }

    Connections{
        target: baseConnections

        onSetupSyncSenderTimer: {
            nextSyncPointTimer.interval = interval
        }

        onRecordTimerEvent: {
            totalRecTime.text = elapsedTime
        }

        onSyncTimerEvent: {
            syncRecTime.text = elapsedTime
        }

        onShowSyncPointId: {
            syncRecPoint.text = syncId.toString()
        }

        onStopTimers: {
            mainTimer.running = false
            syncTimer.running = false
            nextSyncPointTimer.running = false
            recordMonitoring.running = false
        }

        function find(model, criteria) {
          for(var i = 0; i < model.count; ++i)
              if (criteria(model.get(i))) return model.get(i)
          return null
        }

        onShowCaptureLog: {
            var item_index = find(logListModel, function(item){
                return item.device_key_id === key.toString();
            })

            if (item_index !== null){
                logListModel.set(item_index, {
                                     device_key_id: key.toString(),
                                     device_info_str: message
                                 })
            } else {
                logListModel.append({
                                        device_key_id : key.toString(),
                                        device_info_str: message
                                    })
            }
        }

        onExcludeDevice: {
            var index = find(logListModel, function(item){
                return item.device_key_id === key.toString();
            })
            if (index !== null){
                logListModel.remove(index)
            }
        }
    }

    onClosing: {
        baseConnections.stopControl()
    }
}


/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}
}
##^##*/
