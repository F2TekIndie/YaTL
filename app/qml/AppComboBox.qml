import QtQuick
import QtQuick.Controls

ComboBox { id: control
    implicitHeight: AppTheme.minimumInteractiveHeight
    padding: AppTheme.space3
    contentItem: Text { text: control.displayText; color: control.enabled ? AppTheme.onSurface : AppTheme.onSurfaceVariant; verticalAlignment: Text.AlignVCenter; leftPadding: 4; rightPadding: control.indicator.width + 8; elide: Text.ElideRight }
    indicator: Text { x: control.width - width - 12; y: (control.height - height) / 2; text: "▾"; color: control.enabled ? AppTheme.onSurfaceVariant : AppTheme.outlineVariant; font.pixelSize: 16 }
    background: Rectangle { radius: AppTheme.controlRadius; color: AppTheme.surface; border.color: control.activeFocus ? AppTheme.primary : AppTheme.outline; border.width: control.activeFocus ? 2 : 1 }
    delegate: ItemDelegate {
        required property int index
        width: control.popup.width
        highlighted: control.highlightedIndex === index
        contentItem: Text { text: control.textAt(index); color: highlighted ? AppTheme.onPrimaryContainer : AppTheme.onSurface; verticalAlignment: Text.AlignVCenter; leftPadding: 12; elide: Text.ElideRight }
        background: Rectangle { color: highlighted ? AppTheme.primaryContainer : (parent.hovered ? AppTheme.surfaceContainerHigh : AppTheme.surface); radius: AppTheme.controlRadius }
        onClicked: { control.currentIndex = index; control.popup.close(); }
    }
}
