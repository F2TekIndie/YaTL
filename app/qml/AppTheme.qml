pragma Singleton
import QtQuick

QtObject {
    readonly property string background: dmsTheme.background
    readonly property string surface: dmsTheme.surface
    readonly property string surfaceContainerLow: dmsTheme.surfaceContainerLow
    readonly property string surfaceContainerHigh: dmsTheme.surfaceContainerHigh
    readonly property string surfaceContainerHighest: dmsTheme.surfaceContainerHighest
    readonly property string onSurface: dmsTheme.onSurface
    readonly property string onSurfaceVariant: dmsTheme.onSurfaceVariant
    readonly property string primary: dmsTheme.primary
    readonly property string onPrimary: dmsTheme.onPrimary
    readonly property string primaryContainer: dmsTheme.primaryContainer
    readonly property string onPrimaryContainer: dmsTheme.onPrimaryContainer
    readonly property string outline: dmsTheme.outline
    readonly property string outlineVariant: dmsTheme.outlineVariant
    readonly property string error: dmsTheme.error
    readonly property string errorContainer: dmsTheme.errorContainer
    readonly property string onErrorContainer: dmsTheme.onErrorContainer
    readonly property string scrim: dmsTheme.scrim
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
    readonly property bool reducedMotion: dmsTheme.reducedMotion
    readonly property int sidebarWidth: 224
    readonly property int contentMaxWidth: 1200
}
