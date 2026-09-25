import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property alias iconGlyph: icon.glyph
    property alias value: figure.value
    property alias label: figure.label
    property alias valueColor: figure.valueColor
    property alias iconTinted: icon.tinted

    contentPadding: Geometry.spacing.lg

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.xs

        IconGlyph {
            id: icon
            Layout.alignment: Qt.AlignHCenter
            color: Theme.primary
            size: Geometry.size.iconMd
            tinted: true
        }

        StatColumn {
            id: figure
            Layout.fillWidth: true
            labelColor: Theme.textSecondary
        }
    }
}
