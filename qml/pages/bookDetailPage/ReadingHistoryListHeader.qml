import QtQuick
import Library

Item {
    id: root

    property alias title: header.title
    property int sidePadding: 0

    implicitHeight: header.implicitHeight + Geometry.spacing.sm

    SectionHeader {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.sidePadding
        anchors.rightMargin: root.sidePadding
        title: qsTr("Reading history")
    }
}
