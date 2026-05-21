import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property string appTitle: ""
    property string userName: ""

    spacing: Geometry.spacing.xs

    RowLayout {
        id: welcomeRow
        Layout.fillWidth: true
        spacing: Geometry.spacing.xs

        Text {
            id: welcomeText
            // Emoji is rendered by IconGlyph (next sibling) so this Text only
            // contains plain text and follows the platform default font.
            text: root.userName.length > 0 ? qsTr("Welcome, %1").arg(root.userName) : qsTr("Welcome")
            color: Theme.textSecondary
            font.pixelSize: Styles.fontSize.body
            font.weight: Styles.fontWeight.medium
        }

        IconGlyph {
            id: welcomeEmoji
            glyph: "👋"
            color: Theme.textSecondary
            size: Styles.fontSize.body
        }

        Item {
            Layout.fillWidth: true
        }
    }

    Text {
        Layout.fillWidth: true
        text: root.appTitle
        color: Theme.textPrimary
        font.pixelSize: Styles.fontSize.titleLarge
        font.weight: Styles.fontWeight.bold
        elide: Text.ElideRight
    }
}
