import QtQuick
import Library

Item {
    id: root

    property int activeCount: 0

    signal clicked

    implicitWidth: Geometry.size.searchHeight
    implicitHeight: Geometry.size.searchHeight

    Rectangle {
        id: pill
        anchors.fill: parent
        radius: Geometry.radius.pill
        color: root.activeCount > 0 ? Theme.primarySoft : Theme.searchBackground
        opacity: tapArea.pressed ? Styles.opacity.pressed : 1.0

        Behavior on opacity {
            NumberAnimation {
                duration: Styles.duration.fast
            }
        }
    }

    IconGlyph {
        id: icon
        anchors.centerIn: parent
        size: Geometry.size.iconMd
        glyph: "⚙️"
        color: root.activeCount > 0 ? Theme.primary : Theme.textMuted
    }

    Rectangle {
        id: badge
        visible: root.activeCount > 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: Geometry.spacing.xs
        anchors.topMargin: Geometry.spacing.xs
        width: Geometry.size.iconSm
        height: Geometry.size.iconSm
        radius: width / 2
        color: Theme.primary

        Text {
            id: badgeText
            anchors.centerIn: parent
            text: String(root.activeCount)
            color: Theme.primaryContent
            font.pixelSize: Styles.fontSize.caption
            font.weight: Styles.fontWeight.bold
        }
    }

    MouseArea {
        id: tapArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
