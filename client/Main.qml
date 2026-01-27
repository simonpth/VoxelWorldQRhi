import QtQuick
import Client

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello Voxel World")

    RHIView {
        id: rhiView

        Component.onCompleted: {
            Engine.rhiView = rhiView;
        }
    }

    Text {
        id: fpsText
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 8
        text: ""

        Timer {
            id: timer
            interval: 50
            running: true
            repeat: true
            onTriggered: {
                fpsText.text = "FPS: " + Math.round(rhiView.fps);
            }
        }
    }
}
