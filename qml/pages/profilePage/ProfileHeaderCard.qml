import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property alias title: titleText.text
    property alias subtitle: subtitleText.text
    property alias avatarGlyph: avatarIcon.glyph

    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.lg

        Rectangle {
            id: avatar
            Layout.preferredWidth: Geometry.size.avatarLg
            Layout.preferredHeight: Geometry.size.avatarLg
            Layout.alignment: Qt.AlignVCenter
            radius: width / 2
            color: Theme.primarySoft

            IconGlyph {
                id: avatarIcon
                anchors.centerIn: parent
                glyph: "📚"
                size: Geometry.size.iconHuge
            }
        }

        ColumnLayout {
            id: text
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: Geometry.spacing.xxs

            Text {
                id: titleText
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.title
                font.weight: Styles.fontWeight.bold
                elide: Text.ElideRight
            }

            Text {
                id: subtitleText
                Layout.fillWidth: true
                color: Theme.textSecondary
                font.pixelSize: Styles.fontSize.body
                elide: Text.ElideRight
            }
        }
    }
}
