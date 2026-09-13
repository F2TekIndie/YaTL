import QtQuick
import QtQuick.Controls

Row { id: control
    property var options: []
    property int currentIndex: 0
    spacing: 2
    Repeater {
        model: control.options
        delegate: AppButton {
            required property var modelData
            required property int index
            objectName: (modelData && modelData.objectName) ? modelData.objectName : ("segment_" + index)
            text: modelData.name || modelData
            checkable: true
            checked: index === control.currentIndex
            onClicked: control.currentIndex = index
            Accessible.role: Accessible.RadioButton
        }
    }
}
