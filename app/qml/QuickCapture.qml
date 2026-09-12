import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 440
    height: 170
    minimumWidth: 360
    minimumHeight: 150
    visible: true
    title: "YaTL Quick Capture"
    color: "#f5f4f0"
    palette.window: "#f5f4f0"
    palette.windowText: "#202a23"
    palette.text: "#202a23"
    palette.base: "#ffffff"
    palette.button: "#e9ede6"
    palette.buttonText: "#202a23"
    palette.highlight: "#376548"
    palette.highlightedText: "#ffffff"

    function capture() {
        if (taskModel.add(titleInput.text))
            window.close();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Label {
            text: "Quick capture to Inbox"
            font.pixelSize: 20
            font.bold: true
        }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: titleInput
                objectName: "quickTitleInput"
                Layout.fillWidth: true
                placeholderText: "What needs doing?"
                focus: true
                Accessible.name: "Task title"
                onAccepted: window.capture()
            }
            Button {
                objectName: "quickAddButton"
                text: "Add"
                onClicked: window.capture()
            }
        }
        Label {
            objectName: "quickErrorLabel"
            Layout.fillWidth: true
            visible: taskModel.error.length > 0
            text: taskModel.error
            color: "#aa2727"
            wrapMode: Text.Wrap
            Accessible.role: Accessible.AlertMessage
        }
    }
}
