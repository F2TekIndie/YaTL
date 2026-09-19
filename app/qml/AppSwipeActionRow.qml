import QtQuick
import QtQuick.Controls
import "." as App

Rectangle {
    id: control
    default property alias content: contentItem.data
    property string actionLabel: "Delete"
    property bool interactive: false
    property color actionColor: AppTheme.error
    property bool actionEnabled: true
    signal actionTriggered()
    property real reveal: 0
    property real dragStartReveal: 0
    readonly property real actionWidth: 92
    color: AppTheme.surface
    radius: AppTheme.cardRadius
    border.width: 1
    border.color: AppTheme.outlineVariant
    width: 92
    clip: true
    focus: true
    Accessible.name: actionLabel + " action"
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            reveal = 0
            event.accepted = true
        } else if (event.key === Qt.Key_Delete && actionEnabled) {
            actionTriggered()
            event.accepted = true
        }
    }

    App.AppButton {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        objectName: "projectRowItemDelete"
        variant: "destructive"
        enabled: control.reveal > 0.8 && control.actionEnabled
        opacity: control.reveal
        text: "Delete"
        width: control.actionWidth
        onClicked: {
            control.reveal = 0
            control.actionTriggered()
        }
    }

    Item {
        id: contentItem
        x: -control.reveal * control.actionWidth
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width
        clip: true
    }

    DragHandler {
        parent: control
        target: null
        enabled: control.interactive && control.actionEnabled
        xAxis.enabled: true
        yAxis.enabled: false
        onTranslationChanged: {
            control.reveal = Math.max(0, Math.min(1,
                control.dragStartReveal - translation.x / control.actionWidth))
        }
        onActiveChanged: {
            if (active)
                control.dragStartReveal = control.reveal
            else
                control.reveal = control.reveal > 0.45 ? 1 : 0
        }
    }
}
