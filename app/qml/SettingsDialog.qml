import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AppDialog {
    id: dialog
    property var model
    property string defaultProjectIdChoice: ""
    title: ""
    width: 560
    height: 560
    modal: true
    function showSettings() {
        model.clearError()
        defaultProjectIdChoice = model.settings.defaultProjectId || ""
        defaultProject.currentIndex = defaultProject.model.findIndex(item => item.id === defaultProjectIdChoice)
        notificationsEnabled.checked = model.settings.notificationsEnabled
        notificationDays.value = model.settings.notificationDaysBefore || 0
        dmsShowNext.checked = model.settings.dmsShowNextTask
        dmsUseDefault.checked = model.settings.dmsUseDefaultProject
        open()
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: AppTheme.space3
        AppSectionHeader { title: "Settings"; subtitle: "Defaults and desktop integration"; Layout.fillWidth: true }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: parent.width
                spacing: AppTheme.space3
                AppCard {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        AppSectionHeader { title: "General"; Layout.fillWidth: true }
                        GridLayout {
                            columns: 2
                            columnSpacing: AppTheme.space4
                            rowSpacing: AppTheme.space2
                            Layout.fillWidth: true
                            Label { text: "Default capture project"; color: AppTheme.foreground }
                            AppComboBox {
                                id: defaultProject
                                objectName: "defaultProjectSetting"
                                Layout.preferredWidth: 260
                                Layout.alignment: Qt.AlignRight
                                model: dialog.model.projects.filter(item => !item.archived)
                                textRole: "displayName"
                                valueRole: "id"
                                onActivated: dialog.defaultProjectIdChoice = currentValue
                                onCurrentIndexChanged: if (currentIndex >= 0 && currentIndex < count) dialog.defaultProjectIdChoice = model[currentIndex].id
                                Accessible.name: "Default capture project"
                            }
                        }
                    }
                }
                AppCard {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        AppSectionHeader { title: "Notifications"; Layout.fillWidth: true }
                        GridLayout {
                            columns: 2
                            columnSpacing: AppTheme.space4
                            rowSpacing: AppTheme.space2
                            Layout.fillWidth: true
                            Label { text: "Enable notifications" }
                            AppSwitch { id: notificationsEnabled; objectName: "notificationsEnabledSetting"; Layout.alignment: Qt.AlignRight; Accessible.name: "Enable desktop notifications" }
                            Label { text: "Notify before task date"; enabled: notificationsEnabled.checked }
                            RowLayout {
                                enabled: notificationsEnabled.checked
                                Layout.alignment: Qt.AlignRight
                                AppSpinBox { id: notificationDays; objectName: "notificationDaysSetting"; from: 0; to: 30; editable: true; Accessible.name: "Notification days before" }
                                Label { text: notificationDays.value === 1 ? "day" : "days" }
                            }
                        }
                    }
                }
                AppCard {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        AppSectionHeader { title: "DankMaterialShell"; Layout.fillWidth: true }
                        GridLayout {
                            columns: 2
                            columnSpacing: AppTheme.space4
                            rowSpacing: AppTheme.space2
                            Layout.fillWidth: true
                            Label { text: "Show next task in the bar" }
                            AppSwitch { id: dmsShowNext; objectName: "dmsShowNextSetting"; Layout.alignment: Qt.AlignRight }
                            Label { text: "Quick add uses default project" }
                            AppSwitch { id: dmsUseDefault; objectName: "dmsUseDefaultSetting"; Layout.alignment: Qt.AlignRight }
                        }
                    }
                }
                AppCard {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        AppSectionHeader { title: "Desktop integration"; Layout.fillWidth: true }
                        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: AppTheme.mutedForeground; text: "Include /usr/local/share/yatl/niri/yatl.kdl in ~/.config/niri/config.kdl, then run niri validate before reloading." }
                    }
                }
                Label { Layout.fillWidth: true; visible: dialog.model.error.length > 0; text: dialog.model.error; color: AppTheme.error; wrapMode: Text.Wrap }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            AppButton { text: "Cancel"; variant: "text"; onClicked: dialog.close() }
            AppButton {
                objectName: "saveApplicationSettings"
                text: "Save settings"
                onClicked: if (dialog.model.saveSettings(dialog.defaultProjectIdChoice, notificationsEnabled.checked, notificationDays.value, dmsShowNext.checked, dmsUseDefault.checked)) dialog.close()
            }
        }
    }
}
