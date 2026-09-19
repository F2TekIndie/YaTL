import QtQuick
import QtQuick.Controls

Dialog {
    id: control
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    padding: AppTheme.space5
    // Keep the modal overlay lifetime aligned with `visible`; the styled
    // default exit animation could otherwise swallow the next quick click.
    enter: Transition {}
    exit: Transition {}
    header: Label {
        visible: control.title.length > 0
        text: control.title
        color: AppTheme.foreground
        font.bold: true
        leftPadding: AppTheme.space5
        rightPadding: AppTheme.space5
        topPadding: AppTheme.space3
        bottomPadding: AppTheme.space2
        background: Item {}
    }
    background: Rectangle {
        color: AppTheme.surface
        radius: AppTheme.cardRadius
        border.color: AppTheme.outlineVariant
        border.width: 1
        antialiasing: true
        layer.enabled: true
    }
}
