import QtQuick
import QtQuick.Controls

SpinBox {
    id: control
    implicitHeight: AppTheme.minimumInteractiveHeight
    editable: true
    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)
        color: control.enabled ? AppTheme.onSurface : AppTheme.onSurfaceVariant
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        selectByMouse: true
        readOnly: !control.editable
    }
    up.indicator: AppIconButton { iconText: "＋"; implicitWidth: 28; implicitHeight: 20; onClicked: control.increase() }
    down.indicator: AppIconButton { iconText: "−"; implicitWidth: 28; implicitHeight: 20; onClicked: control.decrease() }
    background: Rectangle {
        radius: AppTheme.controlRadius
        color: AppTheme.surface
        border.color: control.activeFocus ? AppTheme.primary : AppTheme.outline
        border.width: control.activeFocus ? 2 : 1
    }
}
