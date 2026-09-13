import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as App

ApplicationWindow {
    id: window
    width: 440
    height: 170
    minimumWidth: 360
    minimumHeight: 150
    visible: true
    title: "YaTL Quick Capture"
    color: App.AppTheme.background
    palette.window: App.AppTheme.background
    palette.windowText: App.AppTheme.onSurface
    palette.text: App.AppTheme.onSurface
    palette.base: App.AppTheme.surface
    palette.button: App.AppTheme.surfaceContainerHigh
    palette.buttonText: App.AppTheme.onSurface
    palette.highlight: App.AppTheme.primary
    palette.highlightedText: App.AppTheme.onPrimary

    function capture() {
        if (taskModel.add(titleInput.text))
            window.close();
    }

    App.AppCard {
        anchors.fill: parent
        anchors.margins: App.AppTheme.space3
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: App.AppTheme.space3
        spacing: 10

        Label {
            text: "Quick capture to Inbox"
            font.pixelSize: 20
            font.bold: true
            color: App.AppTheme.onSurface
        }
        RowLayout {
            Layout.fillWidth: true
            App.AppTextField {
                id: titleInput
                objectName: "quickTitleInput"
                Layout.fillWidth: true
                placeholderText: "What needs doing?"
                focus: true
                Accessible.name: "Task title"
                onAccepted: window.capture()
            }
            App.AppButton {
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
            color: App.AppTheme.error
            wrapMode: Text.Wrap
            Accessible.role: Accessible.AlertMessage
        }
        }
    }
}
