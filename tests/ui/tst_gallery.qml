import QtQuick
import QtTest
import "../../app/qml" as App

Item {
    width: 800
    height: 600
    App.AppCard {
        objectName: "galleryCard"
        App.AppSectionHeader {
            title: "Controls"
            subtitle: "Theme gallery"
        }
    }
    App.AppButton {
        objectName: "galleryButton"
        text: "Action"
        checkable: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 70
    }
    App.AppIconButton {
        objectName: "galleryIconButton"
        iconText: "⋯"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 120
    }
    App.AppTextField {
        objectName: "galleryTextField"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 170
        width: 220
    }
    App.AppComboBox {
        objectName: "galleryComboBox"
        model: ["One", "Two"]
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 220
        width: 220
    }
    App.AppSwitch {
        objectName: "gallerySwitch"
        text: "Enabled"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 270
    }
    App.AppSegmentedControl {
        objectName: "gallerySegments"
        options: [
            {
                name: "Open",
                objectName: "galleryOpenSegment"
            },
            {
                name: "Done",
                objectName: "galleryDoneSegment"
            }
        ]
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 320
    }
    App.AppBadge {
        objectName: "galleryBadge"
        label: "Badge"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 380
    }
    App.AppNavigationItem {
        objectName: "galleryNav"
        text: "Today"
        selected: true
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 430
        width: 180
    }
    App.AppDateField {
        objectName: "galleryDate"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 480
    }
    App.AppSwipeActionRow {
        objectName: "gallerySwipeRow"
        interactive: true
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 540
        width: 300
        height: 48
        Rectangle {
            anchors.fill: parent
            color: App.AppTheme.surfaceContainerHigh
        }
    }
    TestCase {
        name: "ComponentGallery"
        when: windowShown
        function test_components_exist_and_theme_is_live() {
            for (const name of ["galleryCard", "galleryButton", "galleryIconButton", "galleryTextField", "galleryComboBox", "gallerySwitch", "gallerySegments", "galleryBadge", "galleryNav", "galleryDate", "gallerySwipeRow"])
                verify(findChild(parent, name) !== null, name);
            verify(findChild(parent, "galleryDoneSegment") !== null);
            verify(App.AppTheme.primary.length > 0);
            verify(App.AppTheme.surface.length > 0);
            verify(App.AppTheme.foreground.length > 0);
            verify(App.AppTheme.foreground !== App.AppTheme.surface);
            compare(findChild(parent, "galleryButton").implicitHeight >= App.AppTheme.minimumInteractiveHeight, true);
        }
        function test_interaction_states_and_accessibility() {
            const button = findChild(parent, "galleryButton");
            button.forceActiveFocus();
            verify(button.activeFocus);
            button.checked = true;
            verify(button.checked);
            button.enabled = false;
            verify(!button.enabled);
            compare(button.Accessible.name, "Action");

            const switchControl = findChild(parent, "gallerySwitch");
            switchControl.checked = true;
            verify(switchControl.checked);

            const segments = findChild(parent, "gallerySegments");
            segments.currentIndex = 1;
            compare(segments.currentIndex, 1);

            const date = findChild(parent, "galleryDate");
            date.text = "2026-09-13";
            compare(date.text, "2026-09-13");

            const swipe = findChild(parent, "gallerySwipeRow");
            swipe.reveal = 0;
            mouseDrag(swipe, swipe.width - 20, swipe.height / 2, -120, 0);
            tryCompare(swipe, "reveal", 1);
            mouseDrag(swipe, 20, swipe.height / 2, 120, 0);
            tryCompare(swipe, "reveal", 0);
        }
        function test_gallery_reference_sizes() {
            const sizes = [[640, 500], [880, 620], [1280, 800]];
            for (const size of sizes) {
                parent.width = size[0];
                parent.height = size[1];
                waitForRendering(parent);
                grabImage(parent).save("build/gallery-" + size[0] + "x" + size[1] + ".png");
            }
        }
    }
}
