import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property string title: qsTr("Characters")
    property string addLabel: qsTr("+ Add")
    property int sidePadding: 0

    signal addRequested

    implicitHeight: row.implicitHeight

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: root.sidePadding
        anchors.rightMargin: root.sidePadding
        spacing: Geometry.spacing.sm

        Text {
            id: titleText
            Layout.fillWidth: true
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.titleMedium
            font.weight: Styles.fontWeight.bold
        }

        TextButton {
            id: addButton
            label: root.addLabel
            labelColor: Theme.primary
            labelSize: Styles.fontSize.body
            labelWeight: Styles.fontWeight.semibold
            onClicked: root.addRequested()
        }
    }
}
