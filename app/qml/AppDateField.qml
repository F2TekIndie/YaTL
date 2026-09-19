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
    readonly property var weekdayNames: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
    function syncMonth() {
        const parsed = new Date(dateInput.text + "T00:00:00")
        if (!isNaN(parsed.getTime()) && /^\d{4}-\d{2}-\d{2}$/.test(dateInput.text))
            displayMonth = new Date(parsed.getFullYear(), parsed.getMonth(), 1)
    }
    function moveBy(days) {
        const base = dateInput.text ? new Date(dateInput.text + "T00:00:00") : new Date()
        const next = new Date(base.getFullYear(), base.getMonth(), base.getDate() + days)
        dateInput.text = Qt.formatDate(next, "yyyy-MM-dd")
        displayMonth = new Date(next.getFullYear(), next.getMonth(), 1)
    }
    spacing: AppTheme.space2
    AppTextField {
        id: dateInput
        placeholderText: "YYYY-MM-DD"
        validator: RegularExpressionValidator { regularExpression: /^$|^\d{4}(-\d{0,2}(-\d{0,2})?)?$/ }
        Accessible.name: "Date"
        onTextChanged: dateField.syncMonth()
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
                    color: AppTheme.foreground
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
                    model: dateField.weekdayNames
                    delegate: Label {
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 24
                        text: modelData
                        color: AppTheme.mutedForeground
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Repeater {
                    model: 42
                    delegate: Rectangle {
                        required property int index
                        readonly property date cellDate: dateField.dayFor(index)
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        radius: AppTheme.controlRadius
                        color: cellDate.getMonth() === dateField.displayMonth.getMonth()
                               ? (Qt.formatDate(cellDate, "yyyy-MM-dd") === dateInput.text ? AppTheme.primaryContainer : AppTheme.surfaceContainerHigh)
                               : "transparent"
                        opacity: cellDate.getMonth() === dateField.displayMonth.getMonth() ? 1 : 0.45
                        border.color: Qt.formatDate(cellDate, "yyyy-MM-dd") === Qt.formatDate(new Date(), "yyyy-MM-dd") ? AppTheme.primary : "transparent"
                        border.width: 1
                        Text {
                            anchors.fill: parent
                            text: parent.cellDate.getMonth() === dateField.displayMonth.getMonth() ? Qt.formatDate(parent.cellDate, "d") : ""
                            color: parent.cellDate.getMonth() === dateField.displayMonth.getMonth() ? AppTheme.foreground : AppTheme.mutedForeground
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        focus: true
                        Keys.onPressed: event => {
                            if (event.key === Qt.Key_Left) dateField.moveBy(-1)
                            else if (event.key === Qt.Key_Right) dateField.moveBy(1)
                            else if (event.key === Qt.Key_Up) dateField.moveBy(-7)
                            else if (event.key === Qt.Key_Down) dateField.moveBy(7)
                            else if (event.key === Qt.Key_PageUp) dateField.moveBy(event.modifiers & Qt.ControlModifier ? -365 : -30)
                            else if (event.key === Qt.Key_PageDown) dateField.moveBy(event.modifiers & Qt.ControlModifier ? 365 : 30)
                            else if (event.key === Qt.Key_Home) dateField.moveBy(1 - cellDate.getDate())
                            else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                if (cellDate.getMonth() === dateField.displayMonth.getMonth()) {
                                    dateInput.text = Qt.formatDate(cellDate, "yyyy-MM-dd")
                                    datePopup.close()
                                }
                                event.accepted = true
                            } else if (event.key === Qt.Key_Escape) {
                                datePopup.close()
                                event.accepted = true
                            }
                            if ([Qt.Key_Left, Qt.Key_Right, Qt.Key_Up, Qt.Key_Down, Qt.Key_PageUp, Qt.Key_PageDown, Qt.Key_Home].indexOf(event.key) >= 0)
                                event.accepted = true
                        }
                        Accessible.name: Qt.formatDate(cellDate, "yyyy-MM-dd")
                        Accessible.role: Accessible.Button
                        MouseArea {
                            anchors.fill: parent
                            enabled: cellDate.getMonth() === dateField.displayMonth.getMonth()
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
}
