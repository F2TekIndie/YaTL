import QtQuick
import QtTest
import "../../app/qml" as App

Item {
    App.QuickCapture {
        id: captureWindow
        TestCase {
            name: "QuickCaptureWorkflow"
            when: windowShown

            function test_validation_and_inbox_capture() {
                const input = findChild(captureWindow.contentItem, "quickTitleInput")
                const add = findChild(captureWindow.contentItem, "quickAddButton")
                verify(input !== null)
                verify(add !== null)
                const before = taskModel.count
                mouseClick(add)
                compare(taskModel.count, before)
                verify(taskModel.error.length > 0)
                input.text = "Captured from shortcut"
                mouseClick(add)
                tryCompare(taskModel, "count", before + 1)
                compare(taskModel.get(0).title, "Captured from shortcut")
            }
        }
    }
}
