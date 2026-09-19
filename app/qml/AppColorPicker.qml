import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: control
    property color selectedColor: AppTheme.primary
    property string accessibleName: "Color"
    readonly property color themePrimary: AppTheme.primary
    readonly property string normalizedColor: selectedColor.toString().slice(0, 7).toLowerCase()
    readonly property var presetColors: [
        Qt.hsva(themePrimary.hsvHue, 0.30, 1.0, 1),
        Qt.hsva(themePrimary.hsvHue, 0.55, 1.0, 1),
        Qt.hsva(themePrimary.hsvHue, 0.80, 1.0, 1),
        Qt.hsva(themePrimary.hsvHue, 1.00, 0.80, 1),
        Qt.hsva(themePrimary.hsvHue, 0.25, 0.72, 1),
        Qt.hsva(themePrimary.hsvHue, 0.10, 0.94, 1)
    ]
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
                dialog.hue = control.selectedColor.hsvHue
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
        width: Math.min(520, Math.max(320, control.width > 0 ? control.width : 420))
        height: 500
        modal: true

        ColumnLayout {
            anchors.fill: parent
            spacing: AppTheme.space3
            Label {
                text: "Choose a custom color"
                color: AppTheme.foreground
            }
            Flow {
                Layout.fillWidth: true
                spacing: AppTheme.space2
                Repeater {
                    model: control.presetColors
                    delegate: Rectangle {
                        required property color modelData
                        width: 42
                        height: 32
                        radius: AppTheme.controlRadius
                        color: modelData
                        border.color: AppTheme.outline
                        Accessible.name: "Preset " + modelData.toString().slice(0, 7)
                        Accessible.role: Accessible.Button
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                dialog.draftColor = modelData
                                dialog.hue = modelData.hsvHue
                                dialog.saturation = modelData.hsvSaturation
                                dialog.value = modelData.hsvValue
                            }
                        }
                    }
                }
            }
            Rectangle {
                id: canvas
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.maximumWidth: 440
                Layout.minimumWidth: 240
                height: Math.max(140, Math.min(220, width * 0.58))
                radius: AppTheme.controlRadius
                color: Qt.hsva(dialog.hue, 1, 1, 1)
                clip: true
                focus: true
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Left) dialog.saturation = Math.max(0, dialog.saturation - 0.02)
                    else if (event.key === Qt.Key_Right) dialog.saturation = Math.min(1, dialog.saturation + 0.02)
                    else if (event.key === Qt.Key_Up) dialog.value = Math.min(1, dialog.value + 0.02)
                    else if (event.key === Qt.Key_Down) dialog.value = Math.max(0, dialog.value - 0.02)
                    else return
                    dialog.draftColor = Qt.hsva(dialog.hue, dialog.saturation, dialog.value, 1)
                    event.accepted = true
                }
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
                    border.color: dialog.value > 0.5 ? "#202020" : "white"
                    border.width: 2
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        color: "transparent"
                        border.color: dialog.value > 0.5 ? "white" : "#202020"
                        border.width: 1
                    }
                }
            }
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.maximumWidth: 440
                Layout.minimumWidth: 240
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
                    orientation: Gradient.Horizontal
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
                Rectangle {
                    x: dialog.hue * (parent.width - width)
                    y: -2
                    width: 12
                    height: parent.height + 4
                    radius: 6
                    color: "transparent"
                    border.color: AppTheme.foreground
                    border.width: 2
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
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: dialog.draftColor.toString().slice(0, 7).toLowerCase()
                color: AppTheme.mutedForeground
                Accessible.name: "Selected color " + text
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
