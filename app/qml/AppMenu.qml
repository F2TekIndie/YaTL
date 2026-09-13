import QtQuick
import QtQuick.Controls

Menu {
    palette.text: AppTheme.onSurface
    palette.highlight: AppTheme.primaryContainer
    palette.highlightedText: AppTheme.onPrimaryContainer
    background: Rectangle { color: AppTheme.surfaceContainerHigh; radius: AppTheme.controlRadius; border.color: AppTheme.outlineVariant }
}
