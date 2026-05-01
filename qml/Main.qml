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
    title: qsTr("DariszBooks")
    color: Theme.background

    Loader {
        id: pageLoader
        anchors.fill: parent
        source: NavigationController.currentPagePath
    }

    footer: BottomNavBar {
        Layout.fillWidth: true
    }
}
