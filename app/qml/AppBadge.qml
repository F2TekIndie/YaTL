import QtQuick

Rectangle {
    property string label: ""
    implicitWidth: badgeText.implicitWidth + 16
    implicitHeight: 28
    radius: 14
    color: AppTheme.primaryContainer
    Accessible.name: label
    Accessible.role: Accessible.StaticText
    Text { id: badgeText; anchors.centerIn: parent; text: parent.label; color: AppTheme.onPrimaryContainer; font.pixelSize: 12 }
}
