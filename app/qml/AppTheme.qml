pragma Singleton
import QtQuick
import YaTL 1.0

QtObject {
    readonly property string background: DmsTheme.background
    readonly property string surface: DmsTheme.surface
    readonly property string surfaceContainerLow: DmsTheme.surfaceContainerLow
    readonly property string surfaceContainerHigh: DmsTheme.surfaceContainerHigh
    readonly property string surfaceContainerHighest: DmsTheme.surfaceContainerHighest
    readonly property string onSurface: DmsTheme.textColor
    readonly property string onSurfaceVariant: DmsTheme.mutedTextColor
    readonly property string foreground: DmsTheme.textColor
    readonly property string mutedForeground: DmsTheme.mutedTextColor
    readonly property string primary: DmsTheme.primary
    readonly property string onPrimary: DmsTheme.primaryForeground
    readonly property string primaryForeground: DmsTheme.primaryForeground
    readonly property string primaryContainer: DmsTheme.primaryContainer
    readonly property string onPrimaryContainer: DmsTheme.primaryContainerForeground
    readonly property string primaryContainerForeground: DmsTheme.primaryContainerForeground
    readonly property string outline: DmsTheme.outline
    readonly property string outlineVariant: DmsTheme.outlineVariant
    readonly property string error: DmsTheme.error
    readonly property string errorContainer: DmsTheme.errorContainer
    readonly property string onErrorContainer: DmsTheme.errorContainerForeground
    readonly property string errorContainerForeground: DmsTheme.errorContainerForeground
    readonly property string scrim: DmsTheme.scrim
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 24
    readonly property int space6: 32
    readonly property int cardRadius: 15
    readonly property int controlRadius: 10
    readonly property int minimumInteractiveHeight: 40
    readonly property int bodySize: 14
    readonly property int titleSize: 22
    readonly property int animationFast: 100
    readonly property bool reducedMotion: DmsTheme.reducedMotion
    readonly property int sidebarWidth: 224
    readonly property int contentMaxWidth: 1200
}
