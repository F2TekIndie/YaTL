import QtQuick
import QtQuick.Controls

Menu {
    palette.text: AppTheme.foreground
    palette.highlight: AppTheme.primaryContainer
    palette.highlightedText: AppTheme.primaryContainerForeground
    background: Rectangle { color: AppTheme.surfaceContainerHigh; radius: AppTheme.controlRadius; border.color: AppTheme.outlineVariant }
}
