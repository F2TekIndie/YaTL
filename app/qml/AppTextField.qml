import QtQuick
import QtQuick.Controls

TextField { id: control
    implicitHeight: AppTheme.minimumInteractiveHeight
    padding: AppTheme.space3
    color: AppTheme.onSurface
    placeholderTextColor: AppTheme.onSurfaceVariant
    background: Rectangle { radius: AppTheme.controlRadius; color: AppTheme.surface; border.color: control.activeFocus ? AppTheme.primary : AppTheme.outline; border.width: control.activeFocus ? 2 : 1 }
}
