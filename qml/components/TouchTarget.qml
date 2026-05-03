import QtQuick
import Library

Item {
    id: root

    default property alias content: container.children
    property int padding: Geometry.spacing.sm

    signal clicked

    implicitWidth: container.implicitWidth + 2 * root.padding
    implicitHeight: container.implicitHeight + 2 * root.padding

    Item {
        id: container
        anchors.centerIn: parent
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
