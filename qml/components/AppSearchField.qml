import QtQuick
import QtQuick.Controls
import Library

FocusScope {
    id: root

    property alias text: input.text
    property string placeholderText: qsTr("Search...")

    signal textEdited(string text)
    signal accepted(string text)

    implicitHeight: Geometry.size.searchHeight
    implicitWidth: 280

    Rectangle {
        id: pill
        anchors.fill: parent
        radius: Geometry.radius.pill
        color: Theme.searchBackground
        border.width: 0
    }

    IconGlyph {
        id: icon
        anchors.left: parent.left
        anchors.leftMargin: Geometry.spacing.lg
        anchors.verticalCenter: parent.verticalCenter
        size: Geometry.size.iconMd
        glyph: "🔍"
        color: Theme.textMuted
    }

    TextField {
        id: input
        anchors.left: icon.right
        anchors.leftMargin: Geometry.spacing.md
        anchors.right: parent.right
        anchors.rightMargin: Geometry.spacing.lg
        anchors.verticalCenter: parent.verticalCenter

        placeholderText: root.placeholderText
        placeholderTextColor: Theme.textMuted
        color: Theme.textPrimary
        font.pixelSize: Styles.fontSize.bodyLarge
        selectByMouse: true
        background: null
        verticalAlignment: TextInput.AlignVCenter

        onTextEdited: root.textEdited(text)
        onAccepted: root.accepted(text)
    }
}
