import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.1

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
                    Layout.row: 1
                    Layout.column: 1
                    width: 0.5
                    Text {
                        text: "Total recording time"
                    }
                }

                Rectangle{
                    color: "red"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 1
                    Layout.column: 2
                    Text {
                        id: totalRecTime
                        text: "0:0:0"
                    }
                }

                Rectangle{
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 2
                    Layout.column: 1
                    width: 0.5
                    Text {
                        text: "Time from last sync. point"
                    }
                }

                Rectangle{
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 2
                    Layout.column: 2
                    Text {
                        id: syncRecTime
                        text: "0:0:0"
                    }
                }

                Rectangle{
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 3
                    Layout.column: 1
                    width: 0.5
                    Text {
                        text: "Current sync. point"
                    }
                }

                Rectangle{
                    color: "gray"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: 3
                    Layout.column: 2
                    Text {
                        id: syncRecPoint
                        text: "-1"
                    }
                }
            }
        }

        Rectangle{
            color: "gray"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.row: 6
            Layout.column: 1
            Layout.rowSpan: 4
            height: 1
            GridLayout {
                id: buttonGrid
                anchors.fill: parent
                columns: 5
                Button{
                    text: "Start recording"
                    onClicked: {
                        baseConnections.startRecording();
                        mainTimer.running = true
                    }
                }

                Button{
                    text: "Stop recording"
                    onClicked: {
                        baseConnections.stopRecording();
                        mainTimer.running = false
                    }
                }

                Button{
                    text: "Start sync. recording"
                    onClicked: {
                        baseConnections.startSyncRecording();
                        syncTimer.running = true;
                    }
                }

                Button{
                    text: "Stop sync. recording"
                    onClicked: {
                        baseConnections.stopSyncRecording();
                        syncTimer.running = false;
                    }
                }
            }
        }

        Rectangle{
            color: "gray"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.row: 11
            Layout.column: 1
            Layout.rowSpan: 9
            height: 5
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
    }

    onClosing: {
        baseConnections.stopControl()
    }
}