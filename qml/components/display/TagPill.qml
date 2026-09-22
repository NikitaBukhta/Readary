import QtQuick
import Library

Rectangle {
    id: root

    property alias label: labelText.text

    implicitWidth: labelText.implicitWidth + 2 * Geometry.spacing.lg
    implicitHeight: labelText.implicitHeight + 2 * Geometry.spacing.sm
    radius: Geometry.radius.pill
    color: Theme.primarySoft

    Text {
        id: labelText
        anchors.centerIn: parent
        color: Theme.primary
        font.pixelSize: Styles.fontSize.bodySmall
        font.weight: Styles.fontWeight.semibold
    }
}
