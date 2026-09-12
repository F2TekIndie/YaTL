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
        }
    }
}
