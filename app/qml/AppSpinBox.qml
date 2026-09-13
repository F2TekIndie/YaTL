import QtQuick
import QtQuick.Controls

SpinBox {
    id: control
    implicitHeight: AppTheme.minimumInteractiveHeight
    editable: true
    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)
        color: control.enabled ? AppTheme.foreground : AppTheme.mutedForeground
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        leftPadding: 4
        rightPadding: 32
        selectByMouse: true
        readOnly: !control.editable
    }
    up.indicator: AppIconButton {
        x: control.width - width - 4
        y: 2
        iconText: "＋"
        implicitWidth: 28
        implicitHeight: 18
        onClicked: control.increase()
    }
    down.indicator: AppIconButton {
        x: control.width - width - 4
        y: control.height - height - 2
        iconText: "−"
        implicitWidth: 28
        implicitHeight: 18
        onClicked: control.decrease()
    }
    background: Rectangle {
        radius: AppTheme.controlRadius
        color: AppTheme.surface
        border.color: control.activeFocus ? AppTheme.primary : AppTheme.outline
        border.width: control.activeFocus ? 2 : 1
    }
}
