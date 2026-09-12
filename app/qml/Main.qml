import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 880
    height: 620
    minimumWidth: 640
    minimumHeight: 500
    visible: true
    title: "YaTL — " + viewTitle()
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
            titleInput.clear();
            titleInput.forceActiveFocus();
        }
    }
    function projectIndex(id) {
        for (let i = 0; i < taskModel.projects.length; ++i)
            if (taskModel.projects[i].id === id)
                return i;
        return -1;
    }
    function choiceIndex(choices, id) {
        for (let i = 0; i < choices.length; ++i)
            if (choices[i].id === id)
                return i;
        return 0;
    }
    function listChoices(project) {
        return [
            {
                id: "",
                name: "No list"
            }
        ].concat(taskModel.taskLists.filter(item => item.projectId === project));
    }
    function listName(id) {
        for (let i = 0; i < taskModel.taskLists.length; ++i)
            if (taskModel.taskLists[i].id === id)
                return taskModel.taskLists[i].name;
        return "";
    }
    function viewTitle() {
        if (taskModel.view === "today")
            return "Today";
        if (taskModel.view === "upcoming")
            return "Upcoming";
        if (taskModel.view === "search")
            return "Search";
        return destination.currentText;
    }
    function priorityName(value) {
        return ["No priority", "Low", "Medium", "High"][value];
    }
    function tagInfo(id) {
        for (let i = 0; i < taskModel.tags.length; ++i)
            if (taskModel.tags[i].id === id)
                return taskModel.tags[i];
        return {};
    }
    property var priorities: [
        {
            value: 0,
            name: "No priority"
        },
        {
            value: 1,
            name: "Low"
        },
        {
            value: 2,
            name: "Medium"
        },
        {
            value: 3,
            name: "High"
        }
    ]
    Dialog {
        id: tagDialog
        objectName: "tagDialog"
        property string tagId: ""
        title: tagId.length > 0 ? "Edit tag" : "New tag"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 420)
        modal: true
        function showTag(id) {
            tagId = id;
            const tag = window.tagInfo(id);
            tagName.text = tag.name || "";
            tagColor.text = tag.color || "#59675c";
            open();
        }
        onOpened: tagName.forceActiveFocus()
        ColumnLayout {
            anchors.fill: parent
            Label {
                text: "Tag name"
            }
            TextField {
                id: tagName
                objectName: "tagName"
                Layout.fillWidth: true
                Accessible.name: "Tag name"
            }
            Label {
                text: "Color (#RRGGBB)"
            }
            TextField {
                id: tagColor
                objectName: "tagColor"
                Layout.fillWidth: true
                Accessible.name: "Tag color"
            }
            Label {
                Layout.fillWidth: true
                text: taskModel.error
                color: "#aa2727"
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: "Cancel"
                    onClicked: tagDialog.close()
                }
                Button {
                    objectName: "saveTagButton"
                    text: tagDialog.tagId.length > 0 ? "Save tag" : "Create tag"
                    onClicked: {
                        const saved = tagDialog.tagId.length > 0 ? taskModel.editTag(tagDialog.tagId, tagName.text, tagColor.text) : taskModel.addTag(tagName.text, tagColor.text);
                        if (saved)
                            tagDialog.close();
                    }
                }
            }
        }
    }
    Dialog {
        id: manageProject
        objectName: "manageProjectDialog"
        title: "Project settings"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 440)
        modal: true
        function showSettings() {
            managedName.text = taskModel.projectInfo.name;
            managedColor.text = taskModel.projectInfo.color;
            open();
        }
        ColumnLayout {
            anchors.fill: parent
            Label {
                text: "Project name"
            }
            TextField {
                id: managedName
                objectName: "managedName"
                Layout.fillWidth: true
                Accessible.name: "Project name"
            }
            Label {
                text: "Color (#RRGGBB)"
            }
            TextField {
                id: managedColor
                objectName: "managedColor"
                Layout.fillWidth: true
                Accessible.name: "Project color"
            }
            Label {
                text: taskModel.error
                color: "#aa2727"
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: "Cancel"
                    onClicked: manageProject.close()
                }
                Button {
                    objectName: "saveProjectButton"
                    text: "Save project"
                    onClicked: if (taskModel.editProject(managedName.text, managedColor.text))
                        manageProject.close()
                }
            }
        }
    }
    Dialog {
        id: listDialog
        objectName: "listDialog"
        property bool renaming: false
        title: renaming ? "Rename list" : "New list"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 420)
        modal: true
        function showList(rename) {
            renaming = rename;
            listName.text = rename ? listSelector.currentText : "";
            open();
        }
        ColumnLayout {
            anchors.fill: parent
            TextField {
                id: listName
                objectName: "listName"
                Layout.fillWidth: true
                placeholderText: "List name"
                Accessible.name: "List name"
            }
            Label {
                text: taskModel.error
                color: "#aa2727"
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: "Cancel"
                    onClicked: listDialog.close()
                }
                Button {
                    objectName: "saveListButton"
                    text: "Save list"
                    onClicked: if (listDialog.renaming ? taskModel.renameList(listName.text) : taskModel.addList(listName.text))
                        listDialog.close()
                }
            }
        }
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
            Label {
                Layout.fillWidth: true
                text: taskModel.error
                color: "#aa2727"
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: "Cancel"
                    onClicked: projectDialog.close()
                }
                Button {
                    id: createProjectButton
                    objectName: "createProjectButton"
                    text: "Create project"
                    onClicked: if (taskModel.addProject(projectName.text)) {
                        projectName.clear();
                        projectDialog.close();
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
        property string listId: ""
        property int priority: 0
        property var tagIds: []
        title: "Edit task"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 560)
        height: Math.min(window.height - 40, 540)
        modal: true
        function editTask(id, taskTitle, taskNote, project, taskListId, scheduled, due, taskPriority, taskTags) {
            taskId = id;
            editTitle.text = taskTitle;
            editNote.text = taskNote;
            destinationId = project;
            listId = taskListId;
            editScheduled.text = scheduled;
            editDue.text = due;
            priority = taskPriority;
            tagIds = taskTags.map(tag => tag.id);
            open();
        }
        function setTag(id, assigned) {
            const updated = tagIds.slice();
            const index = updated.indexOf(id);
            if (assigned && index < 0)
                updated.push(id);
            else if (!assigned && index >= 0)
                updated.splice(index, 1);
            tagIds = updated;
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
                model: taskModel.projects.filter(item => !item.archived)
                textRole: "name"
                valueRole: "id"
                currentIndex: window.choiceIndex(model, editor.destinationId)
                onActivated: {
                    editor.destinationId = currentValue;
                    editor.listId = "";
                }
                Accessible.name: "Move to project"
            }
            ComboBox {
                objectName: "editTaskList"
                Layout.fillWidth: true
                model: window.listChoices(editor.destinationId)
                textRole: "name"
                valueRole: "id"
                currentIndex: window.choiceIndex(model, editor.listId)
                onActivated: editor.listId = currentValue
                Accessible.name: "Task list"
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: editScheduled
                    objectName: "editScheduled"
                    Layout.fillWidth: true
                    placeholderText: "Scheduled YYYY-MM-DD"
                    Accessible.name: "Scheduled date"
                }
                TextField {
                    id: editDue
                    objectName: "editDue"
                    Layout.fillWidth: true
                    placeholderText: "Due YYYY-MM-DD"
                    Accessible.name: "Due date"
                }
                ComboBox {
                    id: editPriority
                    objectName: "editPriority"
                    model: window.priorities
                    textRole: "name"
                    valueRole: "value"
                    currentIndex: editor.priority
                    onActivated: editor.priority = currentValue
                    Accessible.name: "Priority"
                }
            }
            Label {
                text: "Tags"
                visible: taskModel.tags.length > 0
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: taskModel.tags.length > 0 ? 64 : 0
                visible: taskModel.tags.length > 0
                contentWidth: availableWidth
                Flow {
                    width: parent.width
                    spacing: 6
                    Repeater {
                        model: taskModel.tags
                        CheckBox {
                            required property var modelData
                            objectName: "editTag_" + modelData.id
                            text: modelData.name
                            checked: editor.tagIds.indexOf(modelData.id) >= 0
                            onClicked: editor.setTag(modelData.id, checked)
                            Accessible.name: "Assign tag " + modelData.name
                        }
                    }
                }
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
            Label {
                Layout.fillWidth: true
                text: taskModel.error
                color: "#aa2727"
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    objectName: "cancelEditButton"
                    text: "Cancel"
                    onClicked: editor.close()
                }
                Button {
                    objectName: "saveEditButton"
                    text: "Save changes"
                    onClicked: if (taskModel.edit(editor.taskId, editTitle.text, editNote.text, editor.destinationId, editor.listId, editScheduled.text, editDue.text, editPriority.currentValue, editor.tagIds))
                        editor.close()
                }
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+N"
        onActivated: titleInput.forceActiveFocus()
    }
    Shortcut {
        sequence: "Refresh"
        onActivated: taskModel.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 18
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 4
                Label {
                    text: "YaTL"
                    font.pixelSize: 16
                    color: "#59675c"
                    font.bold: true
                }
                Label {
                    text: window.viewTitle()
                    font.pixelSize: 32
                    font.bold: true
                }
            }
            Item {
                Layout.fillWidth: true
            }
            CheckBox {
                objectName: "showArchived"
                visible: taskModel.view === "project"
                text: "Show archived"
                checked: taskModel.showArchived
                onToggled: taskModel.showArchived = checked
            }
            Label {
                text: "LOCAL · PRIVATE"
                font.pixelSize: 11
                color: "#59675c"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                objectName: "projectsViewButton"
                text: "Projects"
                checkable: true
                checked: taskModel.view === "project"
                onClicked: taskModel.view = "project"
            }
            Button {
                objectName: "todayViewButton"
                text: "Today"
                checkable: true
                checked: taskModel.view === "today"
                onClicked: taskModel.view = "today"
            }
            Button {
                objectName: "upcomingViewButton"
                text: "Upcoming"
                checkable: true
                checked: taskModel.view === "upcoming"
                onClicked: taskModel.view = "upcoming"
            }
            Button {
                objectName: "searchViewButton"
                text: "Search"
                checkable: true
                checked: taskModel.view === "search"
                onClicked: taskModel.view = "search"
            }
            TextField {
                id: searchInput
                objectName: "searchInput"
                visible: taskModel.view === "search"
                Layout.fillWidth: true
                placeholderText: "Search tasks, notes, projects, lists, and tags"
                text: taskModel.searchText
                onTextEdited: taskModel.searchText = text
                Accessible.name: "Search"
            }
            Item {
                Layout.fillWidth: taskModel.view !== "search"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: taskModel.view === "project"
            Rectangle {
                width: 12
                height: 24
                radius: 3
                color: taskModel.projectInfo.color || "#376548"
                Accessible.name: "Project color"
            }
            ComboBox {
                id: destination
                objectName: "projectSelector"
                Layout.fillWidth: true
                model: taskModel.projects
                textRole: "displayName"
                valueRole: "id"
                currentIndex: window.projectIndex(taskModel.projectId)
                onActivated: taskModel.projectId = currentValue
                Accessible.name: "Project"
            }
            Button {
                objectName: "projectUpButton"
                text: "↑"
                enabled: taskModel.projectId !== "" && !taskModel.projectInfo.archived && destination.currentIndex > 1
                Accessible.name: "Move project up"
                onClicked: taskModel.moveProject("up")
            }
            Button {
                objectName: "projectDownButton"
                text: "↓"
                enabled: taskModel.projectId !== "" && !taskModel.projectInfo.archived && destination.currentIndex < destination.count - 1
                Accessible.name: "Move project down"
                onClicked: taskModel.moveProject("down")
            }
            Button {
                objectName: "projectSettingsButton"
                text: "Settings"
                enabled: taskModel.projectId !== "" && !taskModel.projectInfo.archived
                onClicked: manageProject.showSettings()
            }
            Button {
                objectName: "archiveProjectButton"
                text: taskModel.projectInfo.archived ? "Restore" : "Archive"
                enabled: taskModel.projectId !== ""
                onClicked: taskModel.archiveProject(!taskModel.projectInfo.archived)
            }
            Button {
                objectName: "newProjectButton"
                text: "New project"
                onClicked: projectDialog.open()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: taskModel.view === "project" && taskModel.projectId !== ""
            ComboBox {
                id: listSelector
                objectName: "listSelector"
                Layout.fillWidth: true
                model: [
                    {
                        id: "*",
                        name: "All tasks"
                    }
                ].concat(window.listChoices(taskModel.projectId))
                textRole: "name"
                valueRole: "id"
                currentIndex: window.choiceIndex(model, taskModel.listFilter)
                onActivated: taskModel.listFilter = currentValue
                Accessible.name: "Filter by task list"
            }
            Button {
                objectName: "listUpButton"
                text: "↑"
                enabled: !taskModel.projectInfo.archived && listSelector.currentIndex > 2
                Accessible.name: "Move task list up"
                onClicked: taskModel.moveList("up")
            }
            Button {
                objectName: "listDownButton"
                text: "↓"
                enabled: !taskModel.projectInfo.archived && listSelector.currentIndex >= 2 && listSelector.currentIndex < listSelector.count - 1
                Accessible.name: "Move task list down"
                onClicked: taskModel.moveList("down")
            }
            Button {
                objectName: "newListButton"
                text: "New list"
                enabled: !taskModel.projectInfo.archived
                onClicked: listDialog.showList(false)
            }
            Button {
                objectName: "renameListButton"
                text: "Rename list"
                enabled: !taskModel.projectInfo.archived && taskModel.listFilter !== "*" && taskModel.listFilter !== ""
                onClicked: listDialog.showList(true)
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: taskModel.view === "project"
            Rectangle {
                width: 12
                height: 24
                radius: 6
                color: taskModel.tagFilter === "*" ? "#c7cec5" : (window.tagInfo(taskModel.tagFilter).color || "#59675c")
                Accessible.name: "Tag color"
            }
            ComboBox {
                id: tagSelector
                objectName: "tagSelector"
                Layout.fillWidth: true
                model: [
                    {
                        id: "*",
                        name: "All tags",
                        color: "#c7cec5"
                    }
                ].concat(taskModel.tags)
                textRole: "name"
                valueRole: "id"
                currentIndex: window.choiceIndex(model, taskModel.tagFilter)
                onActivated: taskModel.tagFilter = currentValue
                Accessible.name: "Filter by tag"
            }
            Button {
                objectName: "newTagButton"
                text: "New tag"
                onClicked: tagDialog.showTag("")
            }
            Button {
                objectName: "editTagButton"
                text: "Edit tag"
                enabled: taskModel.tagFilter !== "*"
                onClicked: tagDialog.showTag(taskModel.tagFilter)
            }
        }
        Label {
            visible: taskModel.view === "project" && !!taskModel.projectInfo.archived
            text: "Archived project · Restore to make changes"
            color: "#59675c"
        }
        RowLayout {
            Layout.fillWidth: true
            visible: taskModel.view === "project"
            enabled: !taskModel.projectInfo.archived
            TextField {
                id: titleInput
                objectName: "titleInput"
                Layout.fillWidth: true
                placeholderText: "What needs doing?"
                Accessible.name: "Task title"
                onAccepted: window.capture()
                focus: true
            }
            Button {
                objectName: "addButton"
                text: "Add task"
                onClicked: window.capture()
            }
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
            visible: taskModel.view === "project"
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
            Button {
                objectName: "archivedTasksButton"
                text: "Archived"
                checkable: true
                checked: taskModel.filter === "archived"
                onClicked: taskModel.filter = "archived"
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: taskModel.count + (taskModel.count === 1 ? " task" : " tasks")
                color: "#59675c"
            }
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
                required property string listId
                required property string scheduledDate
                required property string dueDate
                required property int priority
                required property string projectName
                required property string listName
                required property bool archived
                required property var tags
                required property int index
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
                        visible: !archived
                        enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                        objectName: "complete_" + taskId
                        text: completed ? "Reopen" : "Complete"
                        Accessible.name: (completed ? "Reopen " : "Complete ") + title
                        onClicked: completed ? taskModel.reopen(taskId) : taskModel.complete(taskId)
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            visible: tags.length > 0
                            text: tags.map(tag => tag.name).join(" · ")
                            font.pixelSize: 11
                            color: tags.length > 0 ? tags[0].color : "#59675c"
                        }
                        Label {
                            Layout.fillWidth: true
                            text: title
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            font.strikeout: completed
                            color: "#202a23"
                        }
                        Label {
                            visible: archived
                            text: "Archived"
                            font.pixelSize: 11
                            color: "#59675c"
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
                            visible: listId.length > 0 && taskModel.listFilter === "*"
                            text: window.listName(listId)
                            font.pixelSize: 11
                            color: taskModel.projectInfo.color || "#59675c"
                        }
                        Label {
                            visible: scheduledDate.length > 0 || dueDate.length > 0 || priority > 0
                            text: (priority > 0 ? window.priorityName(priority) + " · " : "") + (scheduledDate.length > 0 ? "Scheduled " + scheduledDate : "") + (scheduledDate.length > 0 && dueDate.length > 0 ? " · " : "") + (dueDate.length > 0 ? "Due " + dueDate : "")
                            font.pixelSize: 11
                            color: dueDate.length > 0 && dueDate < Qt.formatDate(new Date(), "yyyy-MM-dd") && !completed ? "#aa2727" : "#59675c"
                        }
                        Label {
                            visible: taskModel.view !== "project"
                            text: (projectName.length > 0 ? projectName : "Inbox") + (listName.length > 0 ? " · " + listName : "")
                            font.pixelSize: 11
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
                        objectName: "up_" + taskId
                        text: "↑"
                        implicitWidth: 32
                        visible: taskModel.view === "project" && !archived
                        enabled: taskModel.view === "project" && !taskModel.projectInfo.archived && index > 0
                        Accessible.name: "Move up " + title
                        onClicked: taskModel.moveTask(taskId, "up")
                    }
                    Button {
                        objectName: "down_" + taskId
                        text: "↓"
                        implicitWidth: 32
                        visible: taskModel.view === "project" && !archived
                        enabled: taskModel.view === "project" && !taskModel.projectInfo.archived && index < taskModel.count - 1
                        Accessible.name: "Move down " + title
                        onClicked: taskModel.moveTask(taskId, "down")
                    }
                    Button {
                        enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                        visible: !archived
                        objectName: "edit_" + taskId
                        text: "Edit"
                        Accessible.name: "Edit " + title
                        onClicked: editor.editTask(taskId, title, note, projectId, listId, scheduledDate, dueDate, priority, tags)
                    }
                    Button {
                        enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                        objectName: "archive_" + taskId
                        text: archived ? "Restore" : "Archive"
                        Accessible.name: (archived ? "Restore " : "Archive ") + title
                        onClicked: taskModel.archiveTask(taskId, !archived)
                    }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: taskModel.count === 0
                text: taskModel.view === "search" && taskModel.searchText.trim().length === 0 ? "Enter a search term." : (taskModel.view === "today" ? "Nothing planned for today." : (taskModel.view === "upcoming" ? "Nothing planned in the next 28 days." : (taskModel.filter === "archived" ? "Archived tasks will appear here." : (taskModel.filter === "completed" ? "Completed work will appear here." : "No open tasks here. Add your next task above."))))
                color: "#59675c"
            }
        }
        Label {
            text: "Ctrl+N to capture · Changes saved automatically"
            color: "#59675c"
            font.pixelSize: 12
        }
    }
}
