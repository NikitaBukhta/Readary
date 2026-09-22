import QtQuick
import QtQuick.Layouts
import Library

PressableSurface {
    id: root

    property alias iconGlyph: icon.glyph
    property alias label: labelText.text
    // A section whose page does not exist yet is still listed, but dimmed and
    // inert — the row says what is coming without pretending to lead anywhere.
    property bool available: true

    implicitHeight: Geometry.size.actionButtonHeight
    enabled: root.available
    opacity: root.available ? 1.0 : Styles.opacity.disabled

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: Geometry.spacing.xl
        anchors.rightMargin: Geometry.spacing.xl
        spacing: Geometry.spacing.lg

        IconGlyph {
            id: icon
            Layout.alignment: Qt.AlignVCenter
            color: Theme.primary
            size: Geometry.size.iconLg
            tinted: true
        }

        Text {
            id: labelText
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.bodyLarge
            font.weight: Styles.fontWeight.semibold
            elide: Text.ElideRight
        }
    }
}
