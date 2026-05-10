import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

ApplicationWindow {
    id: root

    width: Geometry.window.defaultWidth
    height: Geometry.window.defaultHeight
    minimumWidth: Geometry.window.minimumWidth
    minimumHeight: Geometry.window.minimumHeight
    visible: true
    title: "DarisszeBooks"
    color: Theme.background

    readonly property var _retranslatableLoaders: [pageLoader, bottomNavBarLoader]

    Loader {
        id: pageLoader
        anchors.fill: parent
        source: NavigationController.currentPagePath
    }

    Toast {
        id: toast
        anchors {
            top: parent.top
            topMargin: Geometry.spacing.lg
            horizontalCenter: parent.horizontalCenter
        }
        width: Math.min(parent.width - 2 * Geometry.spacing.lg, Geometry.window.contentMaxWidth)
        z: 1000
    }

    Component {
        id: bottomNavBarComponent
        BottomNavBar {
            Layout.fillWidth: true
        }
    }

    footer: Loader {
        id: bottomNavBarLoader
        Layout.fillWidth: true
        sourceComponent: bottomNavBarComponent
    }

    Connections {
        target: ToastService
        function onRequested(message) {
            toast.show(message);
        }
    }

    Connections {
        target: SettingsController.languageModel
        function onCurrentChanged() {
            // reload current view;
            for (let i = 0; i < root._retranslatableLoaders.length; ++i) {
                root._retranslatableLoaders[i].active = false;
                root._retranslatableLoaders[i].active = true;
            }
        }
    }
}
