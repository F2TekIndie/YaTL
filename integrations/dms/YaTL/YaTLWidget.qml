import QtQuick
import Quickshell
import qs.Common
import qs.Services
import qs.Widgets
import qs.Modules.Plugins

PluginComponent {
    id: root

    property int openCount: 0
    property int todayCount: 0
    property string nextTitle: "No open tasks"
    property var todayTasks: []
    property string errorText: ""
    property bool showNextTask: true
    property bool useDefaultProject: false

    function parseResult(output, exitCode, kind) {
        if (exitCode !== 0) {
            errorText = "YaTL command failed"
            return
        }
        try {
            const result = JSON.parse(output)
            if (kind === "summary") {
                openCount = result.open_count || 0
                todayCount = result.today_count || 0
                nextTitle = result.next_task ? result.next_task.title : "No open tasks"
            } else if (kind === "settings") {
                showNextTask = result.settings ? result.settings.dms_show_next_task : true
                useDefaultProject = result.settings ? result.settings.dms_use_default_project : false
            } else {
                todayTasks = result.tasks || []
            }
            errorText = ""
        } catch (error) {
            errorText = "YaTL returned invalid data"
        }
    }

    function refresh() {
        Proc.runCommand("yatl.summary", ["yatlctl", "summary"], function(output, exitCode) {
            root.parseResult(output, exitCode, "summary")
        })
        Proc.runCommand("yatl.today", ["yatlctl", "today"], function(output, exitCode) {
            root.parseResult(output, exitCode, "today")
        })
        Proc.runCommand("yatl.settings", ["yatlctl", "settings"], function(output, exitCode) {
            root.parseResult(output, exitCode, "settings")
        })
    }

    function addTask(title) {
        const cleanTitle = title.trim()
        if (!cleanTitle) return
        const command = ["yatlctl", "add", cleanTitle]
        if (useDefaultProject) command.push("--use-default")
        Proc.runCommand("yatl.add", command, function(output, exitCode) {
            if (exitCode !== 0) root.errorText = "Could not add task"
            else root.refresh()
        })
    }

    function completeTask(taskId) {
        Proc.runCommand("yatl.complete", ["yatlctl", "complete", taskId], function(output, exitCode) {
            if (exitCode !== 0) root.errorText = "Could not complete task"
            else root.refresh()
        })
    }

    Component.onCompleted: refresh()

    Timer {
        interval: 5000
        repeat: true
        running: true
        onTriggered: root.refresh()
    }

    horizontalBarPill: Component {
        Row {
            spacing: Theme.spacingXS
            DankIcon {
                name: "checklist"
                size: Theme.iconSize - 4
                color: Theme.primary
                anchors.verticalCenter: parent.verticalCenter
            }
            StyledText {
                text: root.todayCount + " today"
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.Medium
                color: Theme.surfaceText
                anchors.verticalCenter: parent.verticalCenter
            }
            StyledText {
                width: Math.min(implicitWidth, 180)
                text: "• " + root.nextTitle
                visible: root.showNextTask
                elide: Text.ElideRight
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.surfaceVariantText
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    verticalBarPill: Component {
        Column {
            spacing: Theme.spacingXS
            DankIcon {
                name: "checklist"
                size: Theme.iconSize - 4
                color: Theme.primary
                anchors.horizontalCenter: parent.horizontalCenter
            }
            StyledText {
                text: root.todayCount
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.Medium
                color: Theme.surfaceText
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    popoutContent: Component {
        PopoutComponent {
            headerText: "YaTL"
            detailsText: root.openCount + " open • " + root.todayCount + " today"
            showCloseButton: true

            Column {
                width: parent.width
                spacing: Theme.spacingM

                Row {
                    width: parent.width
                    spacing: Theme.spacingS
                    DankTextField {
                        id: quickAddField
                        width: parent.width - addButton.width - parent.spacing
                        placeholderText: root.useDefaultProject ? "Add to default project" : "Add to Inbox"
                        onAccepted: {
                            root.addTask(text)
                            clear()
                        }
                    }
                    DankButton {
                        id: addButton
                        text: "Add"
                        iconName: "add_task"
                        onClicked: {
                            root.addTask(quickAddField.text)
                            quickAddField.clear()
                        }
                    }
                }

                StyledText {
                    width: parent.width
                    text: root.errorText
                    color: Theme.error
                    font.pixelSize: Theme.fontSizeSmall
                    visible: text.length > 0
                }

                StyledText {
                    text: root.todayTasks.length > 0 ? "Due today" : "Nothing due today"
                    color: Theme.surfaceVariantText
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.Medium
                }

                Repeater {
                    model: root.todayTasks.slice(0, 5)
                    delegate: StyledRect {
                        required property var modelData
                        width: parent.width
                        height: 48
                        radius: Theme.cornerRadius
                        color: Theme.surfaceContainerHigh
                        StyledText {
                            anchors.left: parent.left
                            anchors.right: completeButton.left
                            anchors.leftMargin: Theme.spacingM
                            anchors.rightMargin: Theme.spacingS
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.title
                            elide: Text.ElideRight
                            color: Theme.surfaceText
                            font.pixelSize: Theme.fontSizeMedium
                        }
                        DankButton {
                            id: completeButton
                            anchors.right: parent.right
                            anchors.rightMargin: Theme.spacingS
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Done"
                            iconName: "check"
                            onClicked: root.completeTask(modelData.id)
                        }
                    }
                }

                Row {
                    spacing: Theme.spacingS
                    DankButton {
                        text: "Open Today"
                        iconName: "today"
                        onClicked: Quickshell.execDetached(["yatlctl", "open", "today"])
                    }
                    DankButton {
                        text: "Quick capture"
                        iconName: "add"
                        onClicked: Quickshell.execDetached(["yatlctl", "capture"])
                    }
                    DankButton {
                        text: "Refresh"
                        iconName: "refresh"
                        onClicked: root.refresh()
                    }
                }
            }
        }
    }

    popoutWidth: 440
    popoutHeight: 520
}
