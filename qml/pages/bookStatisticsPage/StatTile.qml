import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property string iconGlyph: ""
    property string value: ""
    property string label: ""
    property color valueColor: Theme.textPrimary

    contentPadding: Geometry.spacing.lg

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.xs

        IconGlyph {
            id: icon
            Layout.alignment: Qt.AlignHCenter
            glyph: root.iconGlyph
            color: Theme.primary
            size: Geometry.size.iconMd
            tinted: true
        }

        StatColumn {
            id: figure
            Layout.fillWidth: true
            value: root.value
            label: root.label
            valueColor: root.valueColor
            labelColor: Theme.textSecondary
        }
    }
}
