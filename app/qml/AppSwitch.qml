import QtQuick
import QtQuick.Controls

Switch { id: control
    implicitHeight: AppTheme.minimumInteractiveHeight
    indicator: Rectangle { x: control.leftPadding; y: parent.height / 2 - height / 2; width: 40; height: 24; radius: 12; color: control.checked ? AppTheme.primary : AppTheme.surfaceContainerHighest; border.color: control.activeFocus ? AppTheme.onPrimaryContainer : AppTheme.outline; border.width: 2; Rectangle { x: control.checked ? parent.width - width - 3 : 3; y: 3; width: 18; height: 18; radius: 9; color: control.checked ? AppTheme.onPrimary : AppTheme.onSurfaceVariant } }
    contentItem: Text { text: control.text; color: AppTheme.onSurface; leftPadding: 48; verticalAlignment: Text.AlignVCenter }
}
