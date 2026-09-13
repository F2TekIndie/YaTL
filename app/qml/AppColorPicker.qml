import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: control
    property color selectedColor: AppTheme.primary
    property string accessibleName: "Color"
    readonly property string normalizedColor: selectedColor.toString().slice(0, 7).toLowerCase()
    spacing: AppTheme.space2

    RowLayout {
        Layout.fillWidth: true
        Rectangle {
            Layout.preferredWidth: 44
            Layout.preferredHeight: AppTheme.minimumInteractiveHeight
            radius: AppTheme.controlRadius
            color: control.selectedColor
            border.color: AppTheme.outline
            Accessible.name: control.accessibleName + " preview"
        }
        Item { Layout.fillWidth: true }
        AppButton {
            text: "Choose color"
            variant: "tonal"
            Accessible.name: "Choose " + control.accessibleName
            onClicked: {
                dialog.draftColor = control.selectedColor
                dialog.hue = control.selectedColor.hsvHue / 360
                dialog.saturation = control.selectedColor.hsvSaturation
                dialog.value = control.selectedColor.hsvValue
                dialog.open()
            }
        }
    }

    AppDialog {
        id: dialog
        property color draftColor: control.selectedColor
        property real hue: 0.6
        property real saturation: 1
        property real value: 1
        title: "Choose " + control.accessibleName
        width: 380
        height: 390
        modal: true

        ColumnLayout {
            anchors.fill: parent
            spacing: AppTheme.space3
            Label {
                text: "Choose a custom color"
                color: AppTheme.foreground
            }
            Rectangle {
                id: canvas
                Layout.alignment: Qt.AlignHCenter
                width: 300
                height: 170
                radius: AppTheme.controlRadius
                color: Qt.hsva(dialog.hue, 1, 1, 1)
                clip: true
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "white"
                        }
                        GradientStop {
                            position: 1
                            color: "transparent"
                        }
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "transparent"
                        }
                        GradientStop {
                            position: 1
                            color: "black"
                        }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    function choose(point) {
                        dialog.saturation = Math.max(0, Math.min(1, point.x / width))
                        dialog.value = Math.max(0, Math.min(1, 1 - point.y / height))
                        dialog.draftColor = Qt.hsva(dialog.hue, dialog.saturation, dialog.value, 1)
                    }
                    onPressed: choose(mouse)
                    onPositionChanged: if (pressed) choose(mouse)
                }
                Rectangle {
                    x: dialog.saturation * (canvas.width - width)
                    y: (1 - dialog.value) * (canvas.height - height)
                    width: 14
                    height: 14
                    radius: 7
                    color: "transparent"
                    border.color: "white"
                    border.width: 2
                }
            }
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 300
                height: 24
                radius: 12
                gradient: Gradient {
                    GradientStop { position: 0; color: "#ff0000" }
                    GradientStop { position: 0.17; color: "#ffff00" }
                    GradientStop { position: 0.34; color: "#00ff00" }
                    GradientStop { position: 0.51; color: "#00ffff" }
                    GradientStop { position: 0.68; color: "#0000ff" }
                    GradientStop { position: 0.85; color: "#ff00ff" }
                    GradientStop { position: 1; color: "#ff0000" }
                }
                MouseArea {
                    anchors.fill: parent
                    function choose(point) {
                        dialog.hue = Math.max(0, Math.min(1, point.x / width))
                        dialog.draftColor = Qt.hsva(dialog.hue, dialog.saturation, dialog.value, 1)
                    }
                    onPressed: choose(mouse)
                    onPositionChanged: if (pressed) choose(mouse)
                }
            }
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 52
                height: 34
                radius: AppTheme.controlRadius
                color: dialog.draftColor
                border.color: AppTheme.outline
                Accessible.name: control.accessibleName + " preview"
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                AppButton {
                    text: "Cancel"
                    variant: "text"
                    onClicked: dialog.close()
                }
                AppButton {
                    text: "Choose color"
                    onClicked: {
                        control.selectedColor = dialog.draftColor
                        dialog.close()
                    }
                }
            }
        }
    }
}
