import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as App

ApplicationWindow {
    id: window
    width: 880
    height: 620
    minimumWidth: 640
    minimumHeight: 500
    visible: true
    title: "YaTL — " + viewTitle()
    color: App.AppTheme.background
    palette.window: App.AppTheme.background
    palette.windowText: App.AppTheme.onSurface
    palette.text: App.AppTheme.onSurface
    palette.base: App.AppTheme.surface
    palette.button: App.AppTheme.surfaceContainerHigh
    palette.buttonText: App.AppTheme.onSurface
    palette.highlight: App.AppTheme.primary
    palette.highlightedText: App.AppTheme.onPrimary

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
    function listOrderIndex(id) {
        const choices = taskModel.listsFor(taskModel.projectId);
        for (let i = 1; i < choices.length; ++i)
            if (choices[i].id === id) return i - 1;
        return -1;
    }
    function viewTitle() {
        if (taskModel.view === "today")
            return "Today";
        if (taskModel.view === "upcoming")
            return "Upcoming";
        if (taskModel.view === "search")
            return "Search";
        return taskModel.projectId === "" ? "Inbox" : (taskModel.projectInfo.name || destination.currentText);
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
    function selectedDefaultProjectId() {
        const name = defaultProject.currentText;
        for (let item of taskModel.projects)
            if (item.displayName === name) return item.id;
        return "";
    }
    function contrastText(hex) {
        if (!hex || hex.length < 7) return App.AppTheme.onSurface;
        const r = parseInt(hex.slice(1, 3), 16) / 255;
        const g = parseInt(hex.slice(3, 5), 16) / 255;
        const b = parseInt(hex.slice(5, 7), 16) / 255;
        const lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
        return lum > 0.58 ? App.AppTheme.onSurface : App.AppTheme.onPrimary;
    }
    function userTint(hex) {
        if (!hex || hex.length < 7) return App.AppTheme.surfaceContainerHigh;
        const r = parseInt(hex.slice(1, 3), 16) / 255;
        const g = parseInt(hex.slice(3, 5), 16) / 255;
        const b = parseInt(hex.slice(5, 7), 16) / 255;
        return Qt.rgba(r, g, b, 0.16);
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
    property var recurrences: [
        { id: "none", name: "Does not repeat" },
        { id: "daily", name: "Daily" },
        { id: "weekdays", name: "Weekdays" },
        { id: "weekly", name: "Weekly" },
        { id: "monthly", name: "Monthly" }
    ]
    function recurrenceName(value) {
        for (let item of recurrences)
            if (item.id === value) return item.name
        return value
    }
    App.AppDialog {
        id: applicationSettings
        objectName: "applicationSettings"
        title: "Settings"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 560)
        height: Math.min(window.height - 40, 560)
        modal: true
        property string defaultProjectIdChoice: ""
        function showSettings() {
            taskModel.clearError();
            defaultProjectIdChoice = taskModel.settings.defaultProjectId || ""
            defaultProject.currentIndex = defaultProject.model.findIndex(item => item.id === defaultProjectIdChoice)
            notificationsEnabled.checked = taskModel.settings.notificationsEnabled
            notificationDays.value = taskModel.settings.notificationDaysBefore || 0
            dmsShowNext.checked = taskModel.settings.dmsShowNextTask
            dmsUseDefault.checked = taskModel.settings.dmsUseDefaultProject
            open()
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            App.AppSectionHeader {
                title: "Settings"
                subtitle: "Defaults and desktop integration"
                Layout.fillWidth: true
            }
            Label { text: "Default capture project"; font.bold: true }
            App.AppComboBox {
                id: defaultProject
                objectName: "defaultProjectSetting"
                Layout.fillWidth: true
                model: taskModel.projects.filter(item => !item.archived)
                textRole: "displayName"
                valueRole: "id"
                onActivated: applicationSettings.defaultProjectIdChoice = currentValue
                onCurrentIndexChanged: if (currentIndex >= 0 && currentIndex < count)
                    applicationSettings.defaultProjectIdChoice = model[currentIndex].id
                Accessible.name: "Default capture project"
            }
            Label { text: "Notifications"; font.bold: true }
            App.AppSwitch {
                id: notificationsEnabled
                objectName: "notificationsEnabledSetting"
                text: "Enable desktop notifications"
            }
            RowLayout {
                enabled: notificationsEnabled.checked
                Label { text: "Notify before the task date" }
                    App.AppSpinBox {
                    id: notificationDays
                    objectName: "notificationDaysSetting"
                    from: 0
                    to: 30
                    editable: true
                    Accessible.name: "Notification days before"
                }
                Label { text: notificationDays.value === 1 ? "day" : "days" }
            }
            Label { text: "DankMaterialShell"; font.bold: true }
            App.AppSwitch {
                id: dmsShowNext
                objectName: "dmsShowNextSetting"
                text: "Show the next task in the bar"
            }
            App.AppSwitch {
                id: dmsUseDefault
                objectName: "dmsUseDefaultSetting"
                text: "Send DMS quick add to the default project"
            }
            Label { text: "niri setup"; font.bold: true }
            Label {
                Layout.fillWidth: true
                text: "Include /usr/local/share/yatl/niri/yatl.kdl in ~/.config/niri/config.kdl. Run niri validate before reloading. YaTL never edits your compositor configuration."
                wrapMode: Text.Wrap
            }
            Label {
                Layout.fillWidth: true
                text: taskModel.error
                color: App.AppTheme.error
                wrapMode: Text.Wrap
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                App.AppButton { text: "Cancel"; variant: "text"; onClicked: applicationSettings.close() }
                App.AppButton {
                    objectName: "saveApplicationSettings"
                    text: "Save settings"
                    onClicked: if (taskModel.saveSettings(applicationSettings.defaultProjectIdChoice,
                                                           notificationsEnabled.checked,
                                                           notificationDays.value,
                                                           dmsShowNext.checked,
                                                           dmsUseDefault.checked))
                        applicationSettings.close()
                }
            }
        }
    }
    App.AppDialog {
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
            tagColor.text = tag.color || App.AppTheme.primary;
            open();
        }
        onOpened: tagName.forceActiveFocus()
        onClosed: taskModel.clearError()
        ColumnLayout {
            anchors.fill: parent
            App.AppSectionHeader {
                title: tagDialog.tagId.length > 0 ? "Edit tag" : "New tag"
                subtitle: "A readable color indicator"
                Layout.fillWidth: true
            }
            Label {
                text: "Tag name"
            }
            App.AppTextField {
                id: tagName
                objectName: "tagName"
                Layout.fillWidth: true
                Accessible.name: "Tag name"
            }
            Label {
                text: "Color (#RRGGBB)"
            }
            App.AppTextField {
                id: tagColor
                objectName: "tagColor"
                Layout.fillWidth: true
                Accessible.name: "Tag color"
            }
            Label {
                Layout.fillWidth: true
                text: taskModel.error
                color: App.AppTheme.error
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                App.AppButton {
                    text: "Cancel"
                    variant: "text"
                    onClicked: tagDialog.close()
                }
                App.AppButton {
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
    App.AppDialog {
        id: manageProject
        objectName: "manageProjectDialog"
        title: "Project settings"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 440)
        modal: true
        function showSettings() {
            taskModel.clearError();
            managedName.text = taskModel.projectInfo.name;
            managedColor.text = taskModel.projectInfo.color;
            open();
        }
        onClosed: taskModel.clearError()
        ColumnLayout {
            anchors.fill: parent
            App.AppSectionHeader {
                title: "Project settings"
                subtitle: "Name and color indicator"
                Layout.fillWidth: true
            }
            Label {
                text: "Project name"
            }
            App.AppTextField {
                id: managedName
                objectName: "managedName"
                Layout.fillWidth: true
                Accessible.name: "Project name"
            }
            Label {
                text: "Color (#RRGGBB)"
            }
            App.AppTextField {
                id: managedColor
                objectName: "managedColor"
                Layout.fillWidth: true
                Accessible.name: "Project color"
            }
            Label {
                text: taskModel.error
                color: App.AppTheme.error
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                App.AppButton {
                    text: "Cancel"
                    variant: "text"
                    onClicked: manageProject.close()
                }
                App.AppButton {
                    objectName: "saveProjectButton"
                    text: "Save project"
                    onClicked: if (taskModel.editProject(managedName.text, managedColor.text))
                        manageProject.close()
                }
            }
        }
    }
    App.AppDialog {
        id: listDialog
        objectName: "listDialog"
        property bool renaming: false
        title: renaming ? "Rename list" : "New list"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 420)
        modal: true
        function showList(rename) {
            taskModel.clearError();
            renaming = rename;
            listName.text = rename ? listSelector.currentText : "";
            open();
        }
        onClosed: taskModel.clearError()
        ColumnLayout {
            anchors.fill: parent
            App.AppTextField {
                id: listName
                objectName: "listName"
                Layout.fillWidth: true
                placeholderText: "List name"
                Accessible.name: "List name"
            }
            Label {
                text: taskModel.error
                color: App.AppTheme.error
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                App.AppButton {
                    text: "Cancel"
                    variant: "text"
                    onClicked: listDialog.close()
                }
                App.AppButton {
                    objectName: "saveListButton"
                    text: "Save list"
                    onClicked: if (listDialog.renaming ? taskModel.renameList(listName.text) : taskModel.addList(listName.text))
                        listDialog.close()
                }
            }
        }
    }
    App.AppDialog {
        id: projectDialog
        objectName: "projectDialog"
        title: "New project"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 420)
        modal: true
        onOpened: projectName.forceActiveFocus()
        ColumnLayout {
            anchors.fill: parent
            App.AppSectionHeader {
                title: "New project"
                subtitle: "Projects keep related tasks together"
                Layout.fillWidth: true
            }
            App.AppTextField {
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
                color: App.AppTheme.error
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                App.AppButton {
                    text: "Cancel"
                    variant: "text"
                    onClicked: projectDialog.close()
                }
                App.AppButton {
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
    App.AppDialog {
        id: editor
        objectName: "taskEditor"
        property string taskId: ""
        property string destinationId: ""
        property string listId: ""
        property int priority: 0
        property string recurrence: "none"
        property var tagIds: []
        title: "Edit task"
        anchors.centerIn: parent
        width: Math.min(window.width - 40, 560)
        height: Math.min(window.height - 40, 540)
        modal: true
        function editTask(id, taskTitle, taskNote, project, taskListId, scheduled, due, taskPriority, taskTags, taskRecurrence) {
            taskModel.clearError();
            taskId = id;
            editTitle.text = taskTitle;
            editNote.text = taskNote;
            destinationId = project;
            listId = taskListId;
            editScheduled.text = scheduled;
            editDue.text = due;
            priority = taskPriority;
            recurrence = taskRecurrence;
            tagIds = taskTags.map(tag => tag.id);
            open();
        }
        onClosed: taskModel.clearError()
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
            App.AppSectionHeader {
                title: "Task"
                subtitle: "Title and notes"
                Layout.fillWidth: true
            }
            App.AppTextField {
                id: editTitle
                objectName: "editTitle"
                Layout.fillWidth: true
                Accessible.name: "Task title"
            }
            App.AppSectionHeader {
                title: "Organization"
                subtitle: "Project and list"
                Layout.fillWidth: true
            }
            App.AppComboBox {
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
            App.AppComboBox {
                objectName: "editTaskList"
                Layout.fillWidth: true
                model: window.listChoices(editor.destinationId)
                textRole: "name"
                valueRole: "id"
                currentIndex: window.choiceIndex(model, editor.listId)
                onActivated: editor.listId = currentValue
                Accessible.name: "Task list"
            }
            App.AppSectionHeader {
                title: "Planning"
                subtitle: "Dates, priority, and recurrence"
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                App.AppDateField {
                    id: editScheduled
                    objectName: "editScheduled"
                    Layout.fillWidth: true
                    Accessible.name: "Scheduled date"
                }
                App.AppDateField {
                    id: editDue
                    objectName: "editDue"
                    Layout.fillWidth: true
                    Accessible.name: "Due date"
                }
                App.AppComboBox {
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
            App.AppComboBox {
                id: editRecurrence
                objectName: "editRecurrence"
                Layout.fillWidth: true
                model: window.recurrences
                textRole: "name"
                valueRole: "id"
                currentIndex: window.choiceIndex(model, editor.recurrence)
                onActivated: editor.recurrence = currentValue
                Accessible.name: "Recurrence"
            }
            Label {
                text: "Tags"
                visible: taskModel.tags.length > 0
            }
            App.AppSectionHeader {
                title: "Tags"
                subtitle: "Optional labels"
                visible: taskModel.tags.length > 0
                Layout.fillWidth: true
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
                        App.AppSwitch {
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
                ColumnLayout {
                    width: parent.width
                    App.AppSectionHeader {
                        title: "Notes"
                        subtitle: "Additional context"
                        Layout.fillWidth: true
                    }
                    TextArea {
                        id: editNote
                        objectName: "editNote"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        placeholderText: "Notes"
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.Wrap
                        color: App.AppTheme.onSurface
                        placeholderTextColor: App.AppTheme.onSurfaceVariant
                        background: Rectangle {
                            color: App.AppTheme.surface
                            radius: App.AppTheme.controlRadius
                            border.color: editNote.activeFocus ? App.AppTheme.primary : App.AppTheme.outline
                            border.width: editNote.activeFocus ? 2 : 1
                        }
                        Accessible.name: "Task notes"
                    }
                }
            }
            Label {
                Layout.fillWidth: true
                text: taskModel.error
                color: App.AppTheme.error
                wrapMode: Text.Wrap
            }
            RowLayout {
                Item {
                    Layout.fillWidth: true
                }
                App.AppButton {
                    objectName: "cancelEditButton"
                    text: "Cancel"
                    onClicked: editor.close()
                }
                App.AppButton {
                    objectName: "saveEditButton"
                    text: "Save changes"
                    onClicked: if (taskModel.edit(editor.taskId, editTitle.text, editNote.text, editor.destinationId, editor.listId, editScheduled.text, editDue.text, editPriority.currentValue, editor.tagIds, editRecurrence.currentValue))
                        editor.close()
                }
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+N"
        onActivated: {
            if (taskModel.view !== "project") taskModel.view = "project";
            titleInput.forceActiveFocus();
        }
    }
    Shortcut {
        sequence: "Refresh"
        onActivated: taskModel.refresh()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            id: sidebar
            Layout.fillHeight: true
            Layout.preferredWidth: Math.min(App.AppTheme.sidebarWidth, Math.max(196, window.width * 0.24))
            color: App.AppTheme.surfaceContainerLow
            border.color: App.AppTheme.outlineVariant
            border.width: 1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: App.AppTheme.space4
                spacing: App.AppTheme.space1
                Label {
                    text: "YaTL"
                    color: App.AppTheme.primary
                    font.pixelSize: App.AppTheme.titleSize
                    font.bold: true
                    Layout.leftMargin: App.AppTheme.space2
                    Layout.bottomMargin: App.AppTheme.space3
                }
                App.AppNavigationItem {
                    objectName: "todayViewButton"
                    text: "Today"
                    selected: taskModel.view === "today"
                    Layout.fillWidth: true
                    onClicked: taskModel.view = "today"
                }
                App.AppNavigationItem {
                    objectName: "upcomingViewButton"
                    text: "Upcoming"
                    selected: taskModel.view === "upcoming"
                    Layout.fillWidth: true
                    onClicked: taskModel.view = "upcoming"
                }
                App.AppNavigationItem {
                    objectName: "searchViewButton"
                    text: "Search"
                    selected: taskModel.view === "search"
                    Layout.fillWidth: true
                    onClicked: taskModel.view = "search"
                }
                App.AppNavigationItem {
                    objectName: "sidebarInbox"
                    text: "Inbox"
                    selected: taskModel.view === "project" && taskModel.projectId === ""
                    Layout.fillWidth: true
                    onClicked: { taskModel.view = "project"; taskModel.projectId = ""; }
                }
                App.AppNavigationItem {
                    objectName: "projectsViewButton"
                    text: "Projects"
                    selected: taskModel.view === "project" && taskModel.projectId !== ""
                    Layout.fillWidth: true
                    onClicked: taskModel.view = "project"
                }
                ScrollView {
                    id: projectNavigation
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    ColumnLayout {
                        width: projectNavigation.availableWidth
                        Label {
                            text: "Projects"
                            color: App.AppTheme.onSurfaceVariant
                            font.pixelSize: App.AppTheme.bodySize
                            font.bold: true
                            Layout.fillWidth: true
                            Layout.leftMargin: App.AppTheme.space2
                            Layout.topMargin: App.AppTheme.space4
                            Layout.bottomMargin: App.AppTheme.space1
                        }
                        Repeater {
                            model: taskModel.projects.filter(item => item.id !== "" && (!item.archived || taskModel.showArchived))
                            delegate: App.AppNavigationItem {
                                required property var modelData
                                text: modelData.displayName
                                selected: taskModel.view === "project" && taskModel.projectId === modelData.id
                                Layout.fillWidth: true
                                onClicked: { taskModel.view = "project"; taskModel.projectId = modelData.id; }
                            }
                        }
                    }
                }
                App.AppButton {
                    objectName: "sidebarNewProjectButton"
                    text: "+ New project"
                    Layout.fillWidth: true
                    onClicked: projectDialog.open()
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: App.AppTheme.space6
            spacing: App.AppTheme.space4
        RowLayout {
            Layout.fillWidth: true
            App.AppSectionHeader {
                title: "YaTL"
                subtitle: window.viewTitle()
            }
            Item {
                Layout.fillWidth: true
            }
            App.AppSwitch {
                objectName: "showArchived"
                visible: taskModel.view === "project"
                text: "Show archived"
                checked: taskModel.showArchived
                onToggled: taskModel.showArchived = checked
            }
            Label {
                text: "LOCAL · PRIVATE"
                font.pixelSize: 11
                color: App.AppTheme.onSurfaceVariant
            }
            App.AppButton {
                objectName: "applicationSettingsButton"
                text: "Settings"
                variant: "tonal"
                onClicked: applicationSettings.showSettings()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            App.AppButton {
                objectName: "legacyProjectsViewButton"
                text: "Projects"
                checkable: true
                checked: taskModel.view === "project"
                visible: false
                onClicked: taskModel.view = "project"
            }
            App.AppButton {
                objectName: "legacyTodayViewButton"
                text: "Today"
                checkable: true
                checked: taskModel.view === "today"
                visible: false
                onClicked: taskModel.view = "today"
            }
            App.AppButton {
                objectName: "legacyUpcomingViewButton"
                text: "Upcoming"
                checkable: true
                checked: taskModel.view === "upcoming"
                visible: false
                onClicked: taskModel.view = "upcoming"
            }
            App.AppButton {
                objectName: "legacySearchViewButton"
                text: "Search"
                checkable: true
                checked: taskModel.view === "search"
                visible: false
                onClicked: taskModel.view = "search"
            }
            App.AppTextField {
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
            z: 4
            Rectangle {
                width: 12
                height: 24
                radius: 3
                color: taskModel.projectInfo.color || App.AppTheme.primary
                Accessible.name: "Project color"
            }
            App.AppComboBox {
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
            App.AppIconButton {
                objectName: "projectUpButton"
                text: "↑"
                enabled: taskModel.projectId !== "" && !taskModel.projectInfo.archived && destination.currentIndex > 1
                Accessible.name: "Move project up"
                onClicked: taskModel.moveProject("up")
            }
            App.AppIconButton {
                objectName: "projectDownButton"
                text: "↓"
                enabled: taskModel.projectId !== "" && !taskModel.projectInfo.archived && destination.currentIndex < destination.count - 1
                Accessible.name: "Move project down"
                onClicked: taskModel.moveProject("down")
            }
            App.AppButton {
                objectName: "projectSettingsButton"
                text: "Settings"
                enabled: taskModel.projectId !== "" && !taskModel.projectInfo.archived
                onClicked: manageProject.showSettings()
            }
            App.AppButton {
                objectName: "archiveProjectButton"
                text: taskModel.projectInfo.archived ? "Restore" : "Archive"
                variant: taskModel.projectInfo.archived ? "tonal" : "destructive"
                enabled: taskModel.projectId !== ""
                onClicked: taskModel.archiveProject(!taskModel.projectInfo.archived)
            }
            App.AppButton {
                objectName: "newProjectButton"
                text: "New project"
                onClicked: projectDialog.open()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: taskModel.view === "project" && taskModel.projectId !== ""
            z: 1
            App.AppComboBox {
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
            App.AppIconButton {
                objectName: "listUpButton"
                text: "↑"
                enabled: !taskModel.projectInfo.archived && window.listOrderIndex(taskModel.listFilter) > 0
                Accessible.name: "Move task list up"
                onClicked: taskModel.moveList("up")
            }
            App.AppIconButton {
                objectName: "listDownButton"
                text: "↓"
                enabled: !taskModel.projectInfo.archived && window.listOrderIndex(taskModel.listFilter) >= 0 && window.listOrderIndex(taskModel.listFilter) < taskModel.listsFor(taskModel.projectId).length - 2
                Accessible.name: "Move task list down"
                onClicked: taskModel.moveList("down")
            }
            App.AppButton {
                objectName: "newListButton"
                text: "New list"
                z: 2
                enabled: taskModel.view === "project" && taskModel.projectId !== ""
                onClicked: if (!taskModel.projectInfo.archived) listDialog.showList(false)
            }
            App.AppButton {
                objectName: "renameListButton"
                text: "Rename list"
                enabled: !taskModel.projectInfo.archived && taskModel.listFilter !== "*" && taskModel.listFilter !== ""
                onClicked: listDialog.showList(true)
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: taskModel.view === "project"
            z: 3
            Rectangle {
                width: 12
                height: 24
                radius: 6
                color: taskModel.tagFilter === "*" ? App.AppTheme.surfaceContainerHighest : (window.tagInfo(taskModel.tagFilter).color || App.AppTheme.primary)
                Accessible.name: "Tag color"
            }
            App.AppComboBox {
                id: tagSelector
                objectName: "tagSelector"
                Layout.fillWidth: true
                model: [
                    {
                        id: "*",
                        name: "All tags",
                        color: App.AppTheme.surfaceContainerHighest
                    }
                ].concat(taskModel.tags)
                textRole: "name"
                valueRole: "id"
                currentIndex: window.choiceIndex(model, taskModel.tagFilter)
                onActivated: taskModel.tagFilter = currentValue
                Accessible.name: "Filter by tag"
            }
            App.AppButton {
                objectName: "newTagButton"
                text: "New tag"
                onClicked: tagDialog.showTag("")
            }
            App.AppButton {
                objectName: "editTagButton"
                text: "Edit tag"
                enabled: taskModel.tagFilter !== "*"
                onClicked: tagDialog.showTag(taskModel.tagFilter)
            }
        }
        Label {
            visible: taskModel.view === "project" && !!taskModel.projectInfo.archived
            text: "Archived project · Restore to make changes"
            color: App.AppTheme.onSurfaceVariant
        }
        RowLayout {
            Layout.fillWidth: true
            visible: taskModel.view === "project"
            enabled: !taskModel.projectInfo.archived
            z: 5
            App.AppTextField {
                id: titleInput
                objectName: "titleInput"
                Layout.fillWidth: true
                placeholderText: "What needs doing?"
                Accessible.name: "Task title"
                onAccepted: window.capture()
                focus: true
            }
            App.AppButton {
                objectName: "addButton"
                text: "Add task"
                onClicked: window.capture()
            }
        }
        Label {
            visible: taskModel.view === "project"
            text: "Capturing to " + (taskModel.projectId === "" ? "Inbox" : (taskModel.projectInfo.name || destination.currentText))
            color: App.AppTheme.onSurfaceVariant
            font.pixelSize: 12
            Layout.leftMargin: App.AppTheme.space2
        }
        Label {
            objectName: "errorLabel"
            Layout.fillWidth: true
            visible: taskModel.error.length > 0
            text: taskModel.error
            color: App.AppTheme.error
            wrapMode: Text.Wrap
            Accessible.role: Accessible.AlertMessage
        }
        RowLayout {
            visible: taskModel.view === "project"
            App.AppButton {
                objectName: "inboxButton"
                text: "Open"
                variant: taskModel.filter === "open" ? "tonal" : "text"
                checkable: true
                checked: taskModel.filter === "open"
                onClicked: taskModel.filter = "open"
            }
            App.AppButton {
                objectName: "completedButton"
                text: "Completed"
                variant: taskModel.filter === "completed" ? "tonal" : "text"
                checkable: true
                checked: taskModel.filter === "completed"
                onClicked: taskModel.filter = "completed"
            }
            App.AppButton {
                objectName: "archivedTasksButton"
                text: "Archived"
                variant: taskModel.filter === "archived" ? "tonal" : "text"
                checkable: true
                checked: taskModel.filter === "archived"
                onClicked: taskModel.filter = "archived"
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: taskModel.count + (taskModel.count === 1 ? " task" : " tasks")
                color: App.AppTheme.onSurfaceVariant
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
            delegate: App.AppCard {
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
                required property string recurrence
                required property int index
                interactive: true
                z: 3
                width: taskList.width
                height: row.implicitHeight + 24
                color: App.AppTheme.surface
                border.color: App.AppTheme.outlineVariant
                RowLayout {
                    id: row
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 12
                    App.AppButton {
                        visible: !archived
                        enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                        variant: "tonal"
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
                            color: App.AppTheme.onSurface
                            leftPadding: tags.length > 0 ? 8 : 0
                            rightPadding: tags.length > 0 ? 8 : 0
                            background: Rectangle {
                                visible: tags.length > 0
                                radius: 10
                                color: tags.length > 0 ? window.userTint(tags[0].color) : "transparent"
                                border.color: tags.length > 0 ? tags[0].color : "transparent"
                                border.width: tags.length > 0 ? 1 : 0
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            text: title
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            font.strikeout: completed
                            color: App.AppTheme.onSurface
                        }
                        Label {
                            visible: archived
                            text: "Archived"
                            font.pixelSize: 11
                            color: App.AppTheme.onSurfaceVariant
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: note.length > 0
                            text: note
                            textFormat: Text.PlainText
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            wrapMode: Text.Wrap
                            color: App.AppTheme.onSurfaceVariant
                        }
                        Label {
                            visible: listId.length > 0 && taskModel.listFilter === "*"
                            text: window.listName(listId)
                            font.pixelSize: 11
                            color: App.AppTheme.onSurfaceVariant
                        }
                        Label {
                            visible: scheduledDate.length > 0 || dueDate.length > 0 || priority > 0
                            text: (priority > 0 ? window.priorityName(priority) + " · " : "") + (scheduledDate.length > 0 ? "Scheduled " + scheduledDate : "") + (scheduledDate.length > 0 && dueDate.length > 0 ? " · " : "") + (dueDate.length > 0 ? "Due " + dueDate : "")
                            font.pixelSize: 11
                            color: dueDate.length > 0 && dueDate < Qt.formatDate(new Date(), "yyyy-MM-dd") && !completed ? App.AppTheme.error : App.AppTheme.onSurfaceVariant
                        }
                        Label {
                            visible: recurrence !== "none"
                            text: "Repeats " + window.recurrenceName(recurrence).toLowerCase()
                            font.pixelSize: 11
                            color: App.AppTheme.onSurfaceVariant
                        }
                        Label {
                            visible: taskModel.view !== "project"
                            text: (projectName.length > 0 ? projectName : "Inbox") + (listName.length > 0 ? " · " + listName : "")
                            font.pixelSize: 11
                            color: App.AppTheme.onSurfaceVariant
                        }
                        Label {
                            visible: completed
                            text: completed ? "Completed " + new Date(completedAt).toLocaleString(Qt.locale(), Locale.ShortFormat) : ""
                            font.pixelSize: 11
                            color: App.AppTheme.onSurfaceVariant
                        }
                    }
                    App.AppIconButton {
                        objectName: "up_" + taskId
                        text: "↑"
                        implicitWidth: 32
                        visible: taskModel.view === "project" && !archived
                        enabled: taskModel.view === "project" && !taskModel.projectInfo.archived && index > 0
                        Accessible.name: "Move up " + title
                        onClicked: taskModel.moveTask(taskId, "up")
                    }
                    App.AppIconButton {
                        objectName: "down_" + taskId
                        text: "↓"
                        implicitWidth: 32
                        visible: taskModel.view === "project" && !archived
                        enabled: taskModel.view === "project" && !taskModel.projectInfo.archived && index < taskModel.count - 1
                        Accessible.name: "Move down " + title
                        onClicked: taskModel.moveTask(taskId, "down")
                    }
                    App.AppButton {
                        enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                        visible: !archived
                        variant: "text"
                        objectName: "edit_" + taskId
                        text: "Edit"
                        Accessible.name: "Edit " + title
                        onClicked: editor.editTask(taskId, title, note, projectId, listId, scheduledDate, dueDate, priority, tags, recurrence)
                    }
                    App.AppButton {
                        enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                        variant: archived ? "tonal" : "destructive"
                        z: 10
                        objectName: "archive_" + taskId
                        text: archived ? "Restore" : "Archive"
                        Accessible.name: (archived ? "Restore " : "Archive ") + title
                        onClicked: taskModel.archiveTask(taskId, !archived)
                    }
                    App.AppIconButton {
                        objectName: "menu_" + taskId
                        iconText: "⋯"
                        Accessible.name: "More actions for " + title
                        onClicked: taskMenu.open()
                    }
                    App.AppMenu {
                        id: taskMenu
                        MenuItem {
                            text: "Edit"
                            enabled: !archived && (taskModel.view !== "project" || !taskModel.projectInfo.archived)
                            onTriggered: editor.editTask(taskId, title, note, projectId, listId, scheduledDate, dueDate, priority, tags, recurrence)
                        }
                        MenuItem {
                            text: archived ? "Restore" : "Archive"
                            enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                            onTriggered: taskModel.archiveTask(taskId, !archived)
                        }
                        MenuItem {
                            text: completed ? "Reopen" : "Complete"
                            visible: !archived
                            enabled: taskModel.view !== "project" || !taskModel.projectInfo.archived
                            onTriggered: completed ? taskModel.reopen(taskId) : taskModel.complete(taskId)
                        }
                    }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: taskModel.count === 0
                text: taskModel.view === "search" && taskModel.searchText.trim().length === 0 ? "Enter a search term." : (taskModel.view === "today" ? "Nothing planned for today." : (taskModel.view === "upcoming" ? "Nothing planned in the next 28 days." : (taskModel.filter === "archived" ? "Archived tasks will appear here." : (taskModel.filter === "completed" ? "Completed work will appear here." : "No open tasks here. Add your next task above."))))
                color: App.AppTheme.onSurfaceVariant
            }
        }
        Label {
            text: "Ctrl+N to capture · Changes saved automatically"
            color: App.AppTheme.onSurfaceVariant
            font.pixelSize: 12
        }
    }
    }
}
