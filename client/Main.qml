import QtQuick
import Client

Window {
    id: rootWindow
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello Voxel World")

    Item {
        id: inputHandler
        anchors.fill: parent

        focus: true
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
                mouseLock.active = false;
            }
            Engine.gameLoop.handleKeyPressed(event.key);
        }
        Keys.onReleased: event => {
            Engine.gameLoop.handleKeyReleased(event.key);
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: mouseLock.active ? Qt.BlankCursor : Qt.ArrowCursor

            property QtObject mouseLock: QtObject {
                id: mouseLock
                property bool active: false
            }

            onPressed: {
                mouseLock.active = true;
                Engine.moveMouseToCenter(rootWindow);
            }

            onPositionChanged: mouse => {
                if (!mouseLock.active)
                    return;

                let centerX = width / 2;
                let centerY = height / 2;
                let dx = mouse.x - centerX;
                let dy = mouse.y - centerY;

                if (dx === 0 && dy === 0)
                    return;

                Engine.gameLoop.handleMouseDelta(Qt.vector2d(dx, dy));
                Engine.moveMouseToCenter(rootWindow);
            }
        }
    }

    RHIView {
        id: rhiView

        Component.onCompleted: {
            rhiView.setEngine(Engine);
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

            function getText() {
                var text = "FPS: ";
                text += Math.round(rhiView.fps) + "\n";
                text += "Player World Chunk Pos: " + Engine.gameLoop.getPlayerWorldChunkPosString() + "\n";
                text += "Local Player Position: " + Engine.gameLoop.localPlayerPosition + "\n";
                text += "Camera Rotation: " + Engine.gameLoop.cameraRotation + "\n";
                text += "Velocity: " + Engine.gameLoop.velocity + "\n";
                text += "Speed: " + Engine.gameLoop.velocity.length();
                return text;
            }

            onTriggered: {
                fpsText.text = getText();
            }
        }
    }
}
