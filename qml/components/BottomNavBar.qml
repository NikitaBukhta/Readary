pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

Rectangle {
    id: root

    // Each item: { id: string, label: string, glyph: string, source: url }
    property var items: []
    property string currentId: ""

    signal itemSelected(string id)

    implicitHeight: Geometry.size.bottomNavHeight
    color: Theme.surface

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: Theme.divider
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            model: root.items

            delegate: BottomNavItem {
                required property var modelData
                Layout.fillWidth: true
                Layout.fillHeight: true
                label: modelData.label ?? ""
                iconGlyph: modelData.glyph ?? ""
                iconSource: modelData.source ?? ""
                active: modelData.id === root.currentId
                onClicked: root.itemSelected(modelData.id)
            }
        }
    }
}
