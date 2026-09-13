import QtQuick

Rectangle {
    id: control
    default property alias content: contentItem.data
    property bool interactive: false
    property bool selected: false
    property bool hovered: false
    color: selected ? AppTheme.primaryContainer : (hovered ? AppTheme.surfaceContainerHigh : AppTheme.surfaceContainerLow)
    radius: AppTheme.cardRadius
    border.width: 1
    border.color: AppTheme.outlineVariant
    implicitWidth: contentItem.implicitWidth + AppTheme.space5 * 2
    implicitHeight: contentItem.implicitHeight + AppTheme.space4 * 2
    HoverHandler {
        enabled: control.interactive
        onHoveredChanged: control.hovered = hovered
    }
    Item { id: contentItem; anchors.fill: parent; anchors.margins: AppTheme.space5; implicitWidth: childrenRect.width; implicitHeight: childrenRect.height }
}
