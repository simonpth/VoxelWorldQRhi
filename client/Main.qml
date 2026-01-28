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

        focus: true
        Keys.onPressed: event => {
            switch (event.key) {
            case Qt.Key_W:
                rhiView.localPlayerPosition.z += 1;
                break;
            case Qt.Key_S:
                rhiView.localPlayerPosition.z -= 1;
                break;
            case Qt.Key_A:
                rhiView.localPlayerPosition.x -= 1;
                break;
            case Qt.Key_D:
                rhiView.localPlayerPosition.x += 1;
                break;
            case Qt.Key_Up:
                rhiView.cameraRotation.x += 1;
                break;
            case Qt.Key_Down:
                rhiView.cameraRotation.x -= 1;
                break;
            case Qt.Key_Left:
                rhiView.cameraRotation.y += 1;
                break;
            case Qt.Key_Right:
                rhiView.cameraRotation.y -= 1;
                break;
            }
        }
    }

    Text {
        id: fpsText
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 8
        text: ""
        visible: true

        Timer {
            id: timer
            interval: 50
            running: true
            repeat: true
            onTriggered: {
                fpsText.text = "FPS: " + Math.round(rhiView.fps) + " Pos: " + rhiView.localPlayerPosition + " Rot: " + rhiView.cameraRotation;
            }
        }
    }
}
