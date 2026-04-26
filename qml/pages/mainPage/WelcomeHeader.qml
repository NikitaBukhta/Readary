import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property string appTitle: ""
    property string userName: ""

    spacing: Geometry.spacing.xs

    Text {
        Layout.fillWidth: true
        text: root.userName.length > 0
              ? qsTr("Welcome, %1 👋").arg(root.userName)
              : qsTr("Welcome 👋")
        color: Theme.textSecondary
        font.pixelSize: Styles.fontSize.body
        font.weight: Styles.fontWeight.medium
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
