import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 880
    height: 620
    minimumWidth: 520
    minimumHeight: 400
    visible: true
    title: "YaTL — " + destination.currentText
    color: "#f5f4f0"
    palette.window: "#f5f4f0"
    palette.windowText: "#202a23"
    palette.text: "#202a23"
    palette.base: "#ffffff"
    palette.button: "#e9ede6"
    palette.buttonText: "#202a23"
    palette.highlight: "#376548"
    palette.highlightedText: "#ffffff"

    function capture() {
        if (taskModel.add(titleInput.text)) {
            titleInput.clear()
            titleInput.forceActiveFocus()
        }
    }
    function projectIndex(id) {
        for (let i = 0; i < taskModel.projects.length; ++i)
            if (taskModel.projects[i].id === id) return i
        return 0
    }
    Dialog {
        id: projectDialog
        objectName: "projectDialog"
        title: "New project"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 420)
        modal: true
        onOpened: projectName.forceActiveFocus()
        ColumnLayout {
            anchors.fill: parent
            TextField {
                id: projectName
                objectName: "projectName"
                Layout.fillWidth: true
                placeholderText: "Project name"
                Accessible.name: "Project name"
                onAccepted: createProjectButton.clicked()
            }
            Label { Layout.fillWidth: true; text: taskModel.error; color: "#aa2727"; wrapMode: Text.Wrap }
            RowLayout {
                Item { Layout.fillWidth: true }
                Button { text: "Cancel"; onClicked: projectDialog.close() }
                Button {
                    id: createProjectButton
                    objectName: "createProjectButton"
                    text: "Create project"
                    onClicked: if (taskModel.addProject(projectName.text)) {
                        projectName.clear()
                        projectDialog.close()
                    }
                }
            }
        }
    }
    Dialog {
        id: editor
        objectName: "taskEditor"
        property string taskId: ""
        property string destinationId: ""
        title: "Edit task"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 560)
        height: Math.min(window.height - 40, 450)
        modal: true
        function editTask(id, taskTitle, taskNote, project) {
            taskId = id
            editTitle.text = taskTitle
            editNote.text = taskNote
            destinationId = project
            open()
        }
        onOpened: editTitle.forceActiveFocus()
        ColumnLayout {
            anchors.fill: parent
            TextField {
                id: editTitle
                objectName: "editTitle"
                Layout.fillWidth: true
                Accessible.name: "Task title"
            }
            ComboBox {
                id: editDestination
                objectName: "editDestination"
                Layout.fillWidth: true
                model: taskModel.projects
                textRole: "name"
                valueRole: "id"
                currentIndex: window.projectIndex(editor.destinationId)
                onActivated: editor.destinationId = currentValue
                Accessible.name: "Move to project"
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                TextArea {
                    id: editNote
                    objectName: "editNote"
                    placeholderText: "Notes"
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.Wrap
                    Accessible.name: "Task notes"
                }
            }
            Label { Layout.fillWidth: true; text: taskModel.error; color: "#aa2727"; wrapMode: Text.Wrap }
            RowLayout {
                Item { Layout.fillWidth: true }
                Button { objectName: "cancelEditButton"; text: "Cancel"; onClicked: editor.close() }
                Button {
                    objectName: "saveEditButton"
                    text: "Save changes"
                    onClicked: if (taskModel.edit(editor.taskId, editTitle.text, editNote.text, editor.destinationId)) editor.close()
                }
            }
        }
    }
    Shortcut { sequence: "Ctrl+N"; onActivated: titleInput.forceActiveFocus() }
    Shortcut { sequence: "Refresh"; onActivated: taskModel.refresh() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 18
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 4
                Label { text: "YaTL"; font.pixelSize: 16; color: "#59675c"; font.bold: true }
                Label { text: taskModel.filter === "completed" ? "Completed" : "Tasks"; font.pixelSize: 32; font.bold: true }
            }
            Item { Layout.fillWidth: true }
            Label { text: "LOCAL · PRIVATE"; font.pixelSize: 11; color: "#59675c" }
        }
        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: destination
                objectName: "projectSelector"
                Layout.fillWidth: true
                model: taskModel.projects
                textRole: "name"
                valueRole: "id"
                currentIndex: window.projectIndex(taskModel.projectId)
                onActivated: taskModel.projectId = currentValue
                Accessible.name: "Project"
            }
            Button { objectName: "newProjectButton"; text: "New project"; onClicked: projectDialog.open() }
        }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: titleInput
                objectName: "titleInput"
                Layout.fillWidth: true
                placeholderText: "What needs doing?"
                Accessible.name: "Task title"
                onAccepted: window.capture()
                focus: true
            }
            Button { objectName: "addButton"; text: "Add task"; onClicked: window.capture() }
        }
        Label {
            objectName: "errorLabel"
            Layout.fillWidth: true
            visible: taskModel.error.length > 0
            text: taskModel.error
            color: "#aa2727"
            wrapMode: Text.Wrap
            Accessible.role: Accessible.AlertMessage
        }
        RowLayout {
            Button {
                objectName: "inboxButton"
                text: "Open"
                checkable: true
                checked: taskModel.filter === "open"
                onClicked: taskModel.filter = "open"
            }
            Button {
                objectName: "completedButton"
                text: "Completed"
                checkable: true
                checked: taskModel.filter === "completed"
                onClicked: taskModel.filter = "completed"
            }
            Item { Layout.fillWidth: true }
            Label { text: taskModel.count + (taskModel.count === 1 ? " task" : " tasks"); color: "#59675c" }
        }
        ListView {
            id: taskList
            objectName: "taskList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: taskModel
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property string taskId
                required property string title
                required property bool completed
                required property string completedAt
                required property string projectId
                required property string note
                width: taskList.width
                height: row.implicitHeight + 24
                radius: 8
                color: "#ffffff"
                border.color: "#dfE3dc"
                RowLayout {
                    id: row
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 12
                    Button {
                        objectName: "complete_" + taskId
                        text: completed ? "Reopen" : "Complete"
                        Accessible.name: (completed ? "Reopen " : "Complete ") + title
                        onClicked: completed ? taskModel.reopen(taskId) : taskModel.complete(taskId)
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: title
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            font.strikeout: completed
                            color: "#202a23"
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: note.length > 0
                            text: note
                            textFormat: Text.PlainText
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            wrapMode: Text.Wrap
                            color: "#59675c"
                        }
                        Label {
                            visible: completed
                            text: completed ? "Completed " + new Date(completedAt).toLocaleString(Qt.locale(), Locale.ShortFormat) : ""
                            font.pixelSize: 11
                            color: "#59675c"
                        }
                    }
                    Button {
                        objectName: "edit_" + taskId
                        text: "Edit"
                        Accessible.name: "Edit " + title
                        onClicked: editor.editTask(taskId, title, note, projectId)
                    }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: taskModel.count === 0
                text: taskModel.filter === "completed" ? "Completed work will appear here." : "No open tasks here. Add your next task above."
                color: "#59675c"
            }
        }
        Label { text: "Ctrl+N to capture · Changes saved automatically"; color: "#59675c"; font.pixelSize: 12 }
    }
}
