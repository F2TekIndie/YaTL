import QtQuick
import QtQuick.Controls

Dialog {
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    padding: AppTheme.space5
    background: Rectangle { color: AppTheme.surface; radius: AppTheme.cardRadius; border.color: AppTheme.outlineVariant; border.width: 1 }
}
