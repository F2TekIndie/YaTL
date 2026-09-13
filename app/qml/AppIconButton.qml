import QtQuick
import QtQuick.Controls

Button { id: control
    property string iconText: ""
    implicitWidth: AppTheme.minimumInteractiveHeight
    implicitHeight: AppTheme.minimumInteractiveHeight
    text: iconText
    hoverEnabled: true
    Accessible.name: text
    background: Rectangle {
        radius: width / 2
        color: control.pressed ? AppTheme.surfaceContainerHighest : (control.hovered ? AppTheme.surfaceContainerHigh : "transparent")
        border.color: control.activeFocus ? AppTheme.primary : "transparent"
        border.width: 2
        Behavior on color { ColorAnimation { duration: AppTheme.reducedMotion ? 0 : AppTheme.animationFast } }
    }
    contentItem: Text { text: control.text; color: control.enabled ? AppTheme.onSurface : AppTheme.onSurfaceVariant; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 18 }
}
