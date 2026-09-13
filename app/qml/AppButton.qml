import QtQuick
import QtQuick.Controls

Button { id: control
    property string variant: "primary" // primary, tonal, text, destructive
    implicitHeight: Math.max(AppTheme.minimumInteractiveHeight, contentItem.implicitHeight + 16)
    padding: AppTheme.space3
    hoverEnabled: true
    Accessible.name: text
    contentItem: Text {
        text: control.text
        color: !control.enabled ? AppTheme.mutedForeground
             : control.variant === "text" ? AppTheme.foreground
             : control.variant === "destructive" ? AppTheme.errorContainerForeground
             : control.variant === "tonal" ? AppTheme.primaryContainerForeground : AppTheme.primaryForeground
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: AppTheme.controlRadius
        color: !control.enabled ? AppTheme.surfaceContainerHighest
             : control.variant === "text" ? (control.pressed ? AppTheme.surfaceContainerHighest : (control.hovered ? AppTheme.surfaceContainerHigh : "transparent"))
             : control.variant === "destructive" ? (control.pressed ? AppTheme.error : AppTheme.errorContainer)
             : control.variant === "tonal" ? (control.pressed ? AppTheme.primary : AppTheme.primaryContainer)
             : (control.pressed ? AppTheme.primary : (control.hovered ? AppTheme.primaryContainer : AppTheme.primary))
        border.color: control.activeFocus ? AppTheme.primary : "transparent"
        border.width: control.activeFocus ? 2 : 0
        Behavior on color { ColorAnimation { duration: AppTheme.reducedMotion ? 0 : AppTheme.animationFast } }
    }
}
