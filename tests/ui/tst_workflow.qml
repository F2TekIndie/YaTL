import QtQuick
import QtTest
import "../../app/qml" as App

Item {
    App.Main {
        id: appWindow
        TestCase {
            id: testCase
            name: "InboxWorkflow"
            when: windowShown

            function test_captureValidationCompleteHistory() {
                const input = findChild(appWindow.contentItem, "titleInput");
                const add = findChild(appWindow.contentItem, "addButton");
                verify(input !== null);
                mouseClick(add);
                compare(taskModel.count, 0);
                verify(taskModel.error.length > 0);
                verify(findChild(appWindow.contentItem, "errorLabel").visible);

                appWindow.requestActivate();
                tryCompare(appWindow, "active", true);
                input.forceActiveFocus();
                tryCompare(input, "activeFocus", true);
                input.text = "Verify the complete loop";
                keyClick(Qt.Key_Return);
                tryCompare(taskModel, "count", 1);
                compare(input.text, "");
                compare(taskModel.error, "");
                const list = findChild(appWindow.contentItem, "taskList");
                tryVerify(function () {
                    return list.itemAtIndex(0) !== null;
                });
                const row = list.itemAtIndex(0);
                compare(row.title, "Verify the complete loop");
                grabImage(appWindow.contentItem).save("ui-inbox.png");
                mouseClick(findChild(row, "complete_" + row.taskId));
                tryCompare(taskModel, "count", 0);

                mouseClick(findChild(appWindow.contentItem, "completedButton"));
                tryCompare(taskModel, "count", 1);
                tryVerify(function () {
                    return list.itemAtIndex(0) !== null;
                });
                compare(list.itemAtIndex(0).title, "Verify the complete loop");
                verify(list.itemAtIndex(0).completed);
                mouseClick(findChild(appWindow.contentItem, "inboxButton"));
                tryCompare(taskModel, "count", 0);
            }

            function selectProject(combo, index) {
                mouseClick(combo);
                tryCompare(combo.popup, "opened", true);
                keyClick(Qt.Key_Home);
                for (let i = 0; i < index; ++i)
                    keyClick(Qt.Key_Down);
                keyClick(Qt.Key_Return);
                tryCompare(combo.popup, "visible", false);
            }

            function test_projectEditMoveReopen() {
                appWindow.requestActivate();
                tryCompare(appWindow, "active", true);
                mouseClick(findChild(appWindow, "newProjectButton"));
                const dialog = findChild(appWindow, "projectDialog");
                tryCompare(dialog, "opened", true);
                findChild(appWindow, "projectName").text = "Release";
                mouseClick(findChild(appWindow, "createProjectButton"));
                tryCompare(dialog, "visible", false);
                const project = taskModel.projectId;
                verify(project.length > 0);
                compare(taskModel.count, 0);
                findChild(appWindow, "titleInput").text = "Draft";
                mouseClick(findChild(appWindow, "addButton"));
                tryCompare(taskModel, "count", 1);
                const list = findChild(appWindow, "taskList");
                tryVerify(function () {
                    return list.itemAtIndex(0) !== null;
                });
                const id = list.itemAtIndex(0).taskId;
                mouseClick(findChild(list.itemAtIndex(0), "edit_" + id));
                const editor = findChild(appWindow, "taskEditor");
                tryCompare(editor, "opened", true);
                waitForRendering(editor.contentItem);
                grabImage(appWindow.contentItem).save("ui-editor.png");
                findChild(appWindow, "editTitle").text = " ";
                mouseClick(findChild(appWindow, "saveEditButton"));
                verify(editor.opened);
                verify(taskModel.error.length > 0);
                findChild(appWindow, "editTitle").text = "Publish";
                findChild(appWindow, "editNote").text = "Review and ship";
                mouseClick(findChild(appWindow, "saveEditButton"));
                tryCompare(editor, "visible", false);
                tryVerify(function () {
                    return list.itemAtIndex(0) !== null && list.itemAtIndex(0).title === "Publish";
                });
                compare(list.itemAtIndex(0).note, "Review and ship");
                grabImage(appWindow.contentItem).save("ui-project.png");

                mouseClick(findChild(list.itemAtIndex(0), "complete_" + id));
                tryCompare(taskModel, "count", 0);
                mouseClick(findChild(appWindow, "completedButton"));
                tryCompare(taskModel, "count", 1);
                tryVerify(function () {
                    return list.itemAtIndex(0) !== null;
                });
                mouseClick(findChild(list.itemAtIndex(0), "complete_" + id));
                tryCompare(taskModel, "count", 0);
                mouseClick(findChild(appWindow, "inboxButton"));
                tryCompare(taskModel, "count", 1);
                tryVerify(function () {
                    return list.itemAtIndex(0) !== null;
                });
                mouseClick(findChild(list.itemAtIndex(0), "edit_" + id));
                tryCompare(editor, "opened", true);
                selectProject(findChild(appWindow, "editDestination"), 0);
                mouseClick(findChild(appWindow, "saveEditButton"));
                tryCompare(editor, "visible", false);
                tryCompare(taskModel, "count", 0);
                selectProject(findChild(appWindow, "projectSelector"), 0);
                tryCompare(taskModel, "count", 1);
                tryVerify(function () {
                    return list.itemAtIndex(0) !== null;
                });
                compare(list.itemAtIndex(0).taskId, id);
                compare(list.itemAtIndex(0).projectId, "");
            }

            function test_planningViewsSearchAndValidation() {
                appWindow.requestActivate();
                tryCompare(appWindow, "active", true);
                taskModel.view = "project";
                mouseClick(findChild(appWindow, "newProjectButton"));
                const projectDialog = findChild(appWindow, "projectDialog");
                tryCompare(projectDialog, "opened", true);
                findChild(appWindow, "projectName").text = "Planning";
                mouseClick(findChild(appWindow, "createProjectButton"));
                tryCompare(projectDialog, "visible", false);

                mouseClick(findChild(appWindow, "newListButton"));
                const listDialog = findChild(appWindow, "listDialog");
                tryCompare(listDialog, "opened", true);
                findChild(appWindow, "listName").text = "Milestones";
                mouseClick(findChild(appWindow, "saveListButton"));
                tryCompare(listDialog, "visible", false);

                findChild(appWindow, "titleInput").text = "Prepare planning release";
                mouseClick(findChild(appWindow, "addButton"));
                tryCompare(taskModel, "count", 1);
                const tasks = findChild(appWindow, "taskList");
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null;
                });
                const id = tasks.itemAtIndex(0).taskId;
                mouseClick(findChild(tasks.itemAtIndex(0), "edit_" + id));
                const editor = findChild(appWindow, "taskEditor");
                tryCompare(editor, "opened", true);

                const today = Qt.formatDate(new Date(), "yyyy-MM-dd");
                const tomorrowDate = new Date();
                tomorrowDate.setDate(tomorrowDate.getDate() + 1);
                const tomorrow = Qt.formatDate(tomorrowDate, "yyyy-MM-dd");
                findChild(appWindow, "editNote").text = "Visible through note search";
                findChild(appWindow, "editScheduled").text = tomorrow;
                findChild(appWindow, "editDue").text = today;
                selectProject(findChild(appWindow, "editPriority"), 3);
                mouseClick(findChild(appWindow, "saveEditButton"));
                verify(editor.opened);
                verify(taskModel.error.length > 0);

                findChild(appWindow, "editScheduled").text = today;
                findChild(appWindow, "editDue").text = tomorrow;
                mouseClick(findChild(appWindow, "saveEditButton"));
                tryCompare(editor, "visible", false);
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null;
                });
                compare(tasks.itemAtIndex(0).scheduledDate, today);
                compare(tasks.itemAtIndex(0).dueDate, tomorrow);
                compare(tasks.itemAtIndex(0).priority, 3);

                mouseClick(findChild(appWindow, "todayViewButton"));
                tryCompare(taskModel, "view", "today");
                tryCompare(taskModel, "count", 1);
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null;
                });
                compare(tasks.itemAtIndex(0).taskId, id);
                compare(tasks.itemAtIndex(0).projectName, "Planning");
                compare(tasks.itemAtIndex(0).listName, "Milestones");
                tryVerify(function () {
                    return !findChild(tasks.itemAtIndex(0), "up_" + id).visible;
                });
                waitForRendering(appWindow.contentItem);
                grabImage(appWindow.contentItem).save("ui-planning.png");

                mouseClick(findChild(appWindow, "upcomingViewButton"));
                tryCompare(taskModel, "view", "upcoming");
                tryCompare(taskModel, "count", 1);
                compare(tasks.itemAtIndex(0).taskId, id);

                mouseClick(findChild(appWindow, "searchViewButton"));
                tryCompare(taskModel, "view", "search");
                const search = findChild(appWindow, "searchInput");
                search.forceActiveFocus();
                tryCompare(search, "activeFocus", true);
                taskModel.searchText = "Milestones";
                tryCompare(taskModel, "count", 1);
                compare(tasks.itemAtIndex(0).taskId, id);
                compare(search.text, "Milestones");
                taskModel.searchText = "Visible through note search";
                tryCompare(taskModel, "count", 1);
                mouseClick(findChild(tasks.itemAtIndex(0), "complete_" + id));
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null && tasks.itemAtIndex(0).completed;
                });

                taskModel.view = "project";
                tryCompare(taskModel, "view", "project");
            }

            function test_projectSettingsListsOrderAndArchive() {
                appWindow.requestActivate();
                tryCompare(appWindow, "active", true);
                mouseClick(findChild(appWindow, "newProjectButton"));
                const projectDialog = findChild(appWindow, "projectDialog");
                tryCompare(projectDialog, "opened", true);
                findChild(appWindow, "projectName").text = "Managed";
                mouseClick(findChild(appWindow, "createProjectButton"));
                tryCompare(projectDialog, "visible", false);
                const projectId = taskModel.projectId;
                verify(projectId.length > 0);

                mouseClick(findChild(appWindow, "projectSettingsButton"));
                const settings = findChild(appWindow, "manageProjectDialog");
                tryCompare(settings, "opened", true);
                findChild(appWindow, "managedName").text = "Managed project";
                findChild(appWindow, "managedColor").text = "#663399";
                mouseClick(findChild(appWindow, "saveProjectButton"));
                tryCompare(settings, "visible", false);
                compare(taskModel.projectInfo.name, "Managed project");
                compare(taskModel.projectInfo.color, "#663399");

                mouseClick(findChild(appWindow, "newListButton"));
                const listDialog = findChild(appWindow, "listDialog");
                tryCompare(listDialog, "opened", true);
                findChild(appWindow, "listName").text = "Review";
                mouseClick(findChild(appWindow, "saveListButton"));
                tryCompare(listDialog, "visible", false);
                verify(taskModel.listFilter !== "*");
                verify(taskModel.listFilter.length > 0);
                const listId = taskModel.listFilter;
                mouseClick(findChild(appWindow, "renameListButton"));
                tryCompare(listDialog, "opened", true);
                findChild(appWindow, "listName").text = "Ready";
                mouseClick(findChild(appWindow, "saveListButton"));
                tryCompare(listDialog, "visible", false);
                compare(findChild(appWindow, "listSelector").currentText, "Ready");

                const title = findChild(appWindow, "titleInput");
                const add = findChild(appWindow, "addButton");
                title.text = "First";
                mouseClick(add);
                title.text = "Second";
                mouseClick(add);
                tryCompare(taskModel, "count", 2);
                const tasks = findChild(appWindow, "taskList");
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null;
                });
                compare(tasks.itemAtIndex(0).title, "Second");
                const secondId = tasks.itemAtIndex(0).taskId;
                mouseClick(findChild(tasks.itemAtIndex(0), "down_" + secondId));
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null && tasks.itemAtIndex(0).title === "First";
                });
                compare(tasks.itemAtIndex(1).title, "Second");
                compare(tasks.itemAtIndex(0).listId, listId);
                grabImage(appWindow.contentItem).save("ui-managed-project.png");

                mouseClick(findChild(appWindow, "archiveProjectButton"));
                tryCompare(taskModel, "projectId", "");
                verify(appWindow.projectIndex(projectId) < 0);
                const archivedToggle = findChild(appWindow, "showArchived");
                mouseClick(archivedToggle);
                tryCompare(taskModel, "showArchived", true);
                const selector = findChild(appWindow, "projectSelector");
                selectProject(selector, appWindow.projectIndex(projectId));
                tryCompare(taskModel, "projectId", projectId);
                verify(taskModel.projectInfo.archived);
                verify(!add.enabled);
                mouseClick(findChild(appWindow, "archiveProjectButton"));
                tryCompare(taskModel.projectInfo, "archived", false);
                verify(add.enabled);
            }

            function test_taskArchiveAndOrganizationOrdering() {
                appWindow.requestActivate();
                tryCompare(appWindow, "active", true);
                taskModel.view = "project";
                verify(taskModel.addProject("Order one"));
                const firstProject = taskModel.projectId;
                verify(taskModel.addProject("Order two"));
                const secondProject = taskModel.projectId;
                verify(taskModel.addProject("Order three"));
                const movedProject = taskModel.projectId;
                const originalProjectIndex = appWindow.projectIndex(movedProject);
                mouseClick(findChild(appWindow, "projectUpButton"));
                mouseClick(findChild(appWindow, "projectUpButton"));
                compare(appWindow.projectIndex(movedProject), originalProjectIndex - 2);
                compare(taskModel.projects[appWindow.projectIndex(movedProject) + 1].id, firstProject);
                compare(taskModel.projects[appWindow.projectIndex(movedProject) + 2].id, secondProject);

                verify(taskModel.addList("Order list one"));
                const firstList = taskModel.listFilter;
                verify(taskModel.addList("Order list two"));
                const secondList = taskModel.listFilter;
                verify(taskModel.addList("Order list three"));
                const movedList = taskModel.listFilter;
                mouseClick(findChild(appWindow, "listUpButton"));
                mouseClick(findChild(appWindow, "listUpButton"));
                const orderedLists = taskModel.listsFor(movedProject);
                compare(orderedLists[1].id, movedList);
                compare(orderedLists[2].id, firstList);
                compare(orderedLists[3].id, secondList);

                const title = findChild(appWindow, "titleInput");
                title.text = "Archived workflow target";
                mouseClick(findChild(appWindow, "addButton"));
                tryCompare(taskModel, "count", 1);
                const tasks = findChild(appWindow, "taskList");
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null;
                });
                const id = tasks.itemAtIndex(0).taskId;
                mouseClick(findChild(tasks.itemAtIndex(0), "archive_" + id));
                tryCompare(taskModel, "count", 0);
                mouseClick(findChild(appWindow, "archivedTasksButton"));
                tryCompare(taskModel, "filter", "archived");
                tryCompare(taskModel, "count", 1);
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null && tasks.itemAtIndex(0).archived;
                });
                verify(!findChild(tasks.itemAtIndex(0), "complete_" + id).visible);
                verify(!findChild(tasks.itemAtIndex(0), "edit_" + id).visible);
                waitForRendering(appWindow.contentItem);
                grabImage(appWindow.contentItem).save("ui-archive-order.png");

                mouseClick(findChild(appWindow, "searchViewButton"));
                taskModel.searchText = "Archived workflow target";
                tryCompare(taskModel, "count", 1);
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null && tasks.itemAtIndex(0).archived;
                });
                mouseClick(findChild(tasks.itemAtIndex(0), "archive_" + id));
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null && !tasks.itemAtIndex(0).archived;
                });
                taskModel.view = "project";
                taskModel.filter = "open";
                tryCompare(taskModel, "count", 1);
                compare(tasks.itemAtIndex(0).taskId, id);
            }

            function test_tagsCreateAssignFilterEditAndSearch() {
                appWindow.requestActivate();
                tryCompare(appWindow, "active", true);
                taskModel.view = "project";
                verify(taskModel.addProject("Tags project"));

                const newTag = findChild(appWindow, "newTagButton");
                mouseClick(newTag);
                const dialog = findChild(appWindow, "tagDialog");
                tryCompare(dialog, "opened", true);
                findChild(appWindow, "tagName").text = "Work";
                findChild(appWindow, "tagColor").text = "#A1B2C3";
                mouseClick(findChild(appWindow, "saveTagButton"));
                tryCompare(dialog, "visible", false);
                const workId = taskModel.tagFilter;
                compare(taskModel.tags.length, 1);
                compare(taskModel.tags[0].color, "#a1b2c3");

                mouseClick(newTag);
                tryCompare(dialog, "opened", true);
                findChild(appWindow, "tagName").text = "Urgent";
                findChild(appWindow, "tagColor").text = "#AA2727";
                mouseClick(findChild(appWindow, "saveTagButton"));
                tryCompare(dialog, "visible", false);
                const urgentId = taskModel.tagFilter;
                compare(taskModel.tags.length, 2);

                const selector = findChild(appWindow, "tagSelector");
                taskModel.tagFilter = "*";
                tryCompare(taskModel, "tagFilter", "*");
                tryCompare(selector, "currentIndex", 0);
                findChild(appWindow, "titleInput").text = "Tagged visual workflow";
                mouseClick(findChild(appWindow, "addButton"));
                tryCompare(taskModel, "count", 1);
                const tasks = findChild(appWindow, "taskList");
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null;
                });
                const taskId = tasks.itemAtIndex(0).taskId;
                mouseClick(findChild(tasks.itemAtIndex(0), "edit_" + taskId));
                const editor = findChild(appWindow, "taskEditor");
                tryCompare(editor, "opened", true);
                waitForRendering(editor.contentItem);
                let workCheck = null;
                let urgentCheck = null;
                tryVerify(function () {
                    workCheck = findChild(editor.contentItem, "editTag_" + workId);
                    urgentCheck = findChild(editor.contentItem, "editTag_" + urgentId);
                    return workCheck !== null && urgentCheck !== null;
                });
                mouseClick(workCheck);
                mouseClick(urgentCheck);
                mouseClick(findChild(appWindow, "saveEditButton"));
                tryCompare(editor, "visible", false);
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null && tasks.itemAtIndex(0).tags.length === 2;
                });
                compare(tasks.itemAtIndex(0).tags[0].name, "Urgent");
                compare(tasks.itemAtIndex(0).tags[1].name, "Work");
                compare(tasks.itemAtIndex(0).scheduledDate, "");
                compare(tasks.itemAtIndex(0).dueDate, "");

                selectProject(selector, appWindow.choiceIndex(selector.model, workId));
                tryCompare(taskModel, "tagFilter", workId);
                tryCompare(taskModel, "count", 1);
                mouseClick(findChild(appWindow, "editTagButton"));
                tryCompare(dialog, "opened", true);
                findChild(appWindow, "tagName").text = "Focus";
                findChild(appWindow, "tagColor").text = "#334455";
                mouseClick(findChild(appWindow, "saveTagButton"));
                tryCompare(dialog, "visible", false);
                tryVerify(function () {
                    return tasks.itemAtIndex(0) !== null && tasks.itemAtIndex(0).tags[0].name === "Focus";
                });
                compare(selector.currentText, "Focus");
                waitForRendering(appWindow.contentItem);
                grabImage(appWindow.contentItem).save("ui-tags.png");

                mouseClick(findChild(appWindow, "searchViewButton"));
                taskModel.searchText = "focus";
                tryCompare(taskModel, "count", 1);
                compare(tasks.itemAtIndex(0).taskId, taskId);
                taskModel.view = "project";
                taskModel.tagFilter = "*";
            }
        }
    }
}
