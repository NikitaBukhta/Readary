import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property alias title: titleText.text
    property alias addLabel: addButton.label
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
            text: qsTr("Characters")
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.titleMedium
            font.weight: Styles.fontWeight.bold
        }

        TextButton {
            id: addButton
            label: qsTr("+ Add")
            labelColor: Theme.primary
            labelSize: Styles.fontSize.body
            labelWeight: Styles.fontWeight.semibold
            onClicked: root.addRequested()
        }
    }
}
