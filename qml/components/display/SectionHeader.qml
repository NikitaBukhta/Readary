import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property string title: ""
    property string trailingText: ""
    property color trailingColor: Theme.primary
    // Page sections use titleMedium; a header inside a card sits one step down.
    property int titleSize: Styles.fontSize.titleMedium

    implicitHeight: titleLabel.implicitHeight
    implicitWidth: layout.implicitWidth

    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.sm

        Text {
            id: titleLabel
            Layout.fillWidth: true
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: root.titleSize
            font.weight: Styles.fontWeight.bold
            elide: Text.ElideRight
        }

        Text {
            id: trailingLabel
            visible: root.trailingText.length > 0
            text: root.trailingText
            color: root.trailingColor
            font.pixelSize: Styles.fontSize.body
            font.weight: Styles.fontWeight.semibold
        }
    }
}
