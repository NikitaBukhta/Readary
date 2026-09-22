import QtQuick
import QtQuick.Layouts
import Library

PressableSurface {
    id: root

    property alias name: nameText.text
    property alias role: roleText.text
    property alias avatarSource: avatarImage.source

    implicitHeight: Geometry.size.characterRowHeight
    shadowOffset: Styles.elevation.subtleOffset
    shadowBlur: Styles.elevation.subtleBlur

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: Geometry.spacing.md
        anchors.rightMargin: Geometry.spacing.lg
        spacing: Geometry.spacing.md

        Rectangle {
            id: avatar
            Layout.preferredWidth: Geometry.size.avatarSm
            Layout.preferredHeight: Geometry.size.avatarSm
            Layout.alignment: Qt.AlignVCenter
            radius: width / 2
            color: Theme.primarySoft
            clip: true

            Image {
                id: avatarImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready
            }

            IconGlyph {
                id: avatarGlyph
                anchors.centerIn: parent
                visible: !avatarImage.visible
                glyph: "👥"
                color: Theme.primary
                size: Geometry.size.iconSm
            }
        }

        ColumnLayout {
            id: textColumn
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 0

            Text {
                id: nameText
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.bodyLarge
                font.weight: Styles.fontWeight.semibold
                elide: Text.ElideRight
            }

            Text {
                id: roleText
                Layout.fillWidth: true
                color: Theme.textSecondary
                font.pixelSize: Styles.fontSize.small
                elide: Text.ElideRight
                visible: text.length > 0
            }
        }
    }
}
