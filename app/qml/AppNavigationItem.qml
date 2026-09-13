import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool selected: false
    implicitHeight: AppTheme.minimumInteractiveHeight
    hoverEnabled: true
    background: Rectangle {
        radius: AppTheme.controlRadius
        color: control.selected ? AppTheme.primaryContainer : (control.hovered ? AppTheme.surfaceContainerHigh : "transparent")
        border.color: control.activeFocus ? AppTheme.primary : "transparent"
        border.width: 2
    }
    contentItem: Text {
        text: control.text
        color: control.selected ? AppTheme.primaryContainerForeground : AppTheme.foreground
        verticalAlignment: Text.AlignVCenter
        leftPadding: 12
    }
}
