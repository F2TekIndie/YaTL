import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Row {
    id: dateField
    property alias text: dateInput.text
    property date displayMonth: new Date()
    signal cleared()
    function dayFor(index) {
        const first = new Date(displayMonth.getFullYear(), displayMonth.getMonth(), 1)
        const offset = (first.getDay() + 6) % 7
        return new Date(displayMonth.getFullYear(), displayMonth.getMonth(), index - offset + 1)
    }
    spacing: AppTheme.space2
    AppTextField {
        id: dateInput
        placeholderText: "YYYY-MM-DD"
        validator: RegularExpressionValidator { regularExpression: /^$|^\d{4}(-\d{0,2}(-\d{0,2})?)?$/ }
        Accessible.name: "Date"
    }
    AppIconButton {
        iconText: "▣"
        Accessible.name: "Choose date"
        onClicked: datePopup.open()
    }
    AppIconButton {
        iconText: "×"
        Accessible.name: "Clear date"
        onClicked: {
            dateInput.clear()
            parent.cleared()
        }
    }
    Popup {
        id: datePopup
        padding: AppTheme.space2
        background: Rectangle {
            color: AppTheme.surface
            radius: AppTheme.controlRadius
            border.color: AppTheme.outlineVariant
        }
        ColumnLayout {
            spacing: AppTheme.space2
            RowLayout {
                Layout.fillWidth: true
                AppIconButton {
                    iconText: "‹"
                    Accessible.name: "Previous month"
                    onClicked: {
                        dateField.displayMonth = new Date(dateField.displayMonth.getFullYear(), dateField.displayMonth.getMonth() - 1, 1)
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: Qt.formatDate(dateField.displayMonth, "MMMM yyyy")
                    color: AppTheme.onSurface
                    horizontalAlignment: Text.AlignHCenter
                }
                AppIconButton {
                    iconText: "›"
                    Accessible.name: "Next month"
                    onClicked: {
                        dateField.displayMonth = new Date(dateField.displayMonth.getFullYear(), dateField.displayMonth.getMonth() + 1, 1)
                    }
                }
            }
            GridLayout {
                columns: 7
                columnSpacing: 2
                rowSpacing: 2
                Repeater {
                    model: 42
                    delegate: AppButton {
                        required property int index
                        readonly property date cellDate: dateField.dayFor(index)
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        text: Qt.formatDate(cellDate, "d")
                        visible: cellDate.getMonth() === dateField.displayMonth.getMonth()
                        Accessible.name: Qt.formatDate(cellDate, "yyyy-MM-dd")
                        onClicked: {
                            dateInput.text = Qt.formatDate(cellDate, "yyyy-MM-dd")
                            datePopup.close()
                        }
                    }
                }
            }
        }
    }
}
