import QtQuick
import Library

Rectangle {
    id: root

    property alias label: labelText.text
    property bool selected: false

    signal clicked

    implicitWidth: labelText.implicitWidth + 2 * Geometry.spacing.lg
    implicitHeight: labelText.implicitHeight + 2 * Geometry.spacing.sm
    radius: Geometry.radius.pill
    color: root.selected ? Theme.primarySoft : Theme.surfaceVariant
    border.width: root.selected ? 1 : 0
    border.color: Theme.primary
    opacity: tapArea.pressed ? Styles.opacity.pressed : 1.0

    Behavior on opacity {
        NumberAnimation {
            duration: Styles.duration.fast
        }
    }

    Text {
        id: labelText
        anchors.centerIn: parent
        color: root.selected ? Theme.primary : Theme.textSecondary
        font.pixelSize: Styles.fontSize.bodySmall
        font.weight: root.selected ? Styles.fontWeight.semibold : Styles.fontWeight.regular
    }

    MouseArea {
        id: tapArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
