import QtQuick
import QtQuick.Layouts
import Library

// A figure over its caption. The statistics page stacks these three across a
// card; anything that needs "big number, small label" can use it.
ColumnLayout {
    id: root

    property string value: ""
    property string label: ""
    property color valueColor: Theme.textPrimary
    property color labelColor: Theme.textMuted
    property int alignment: Text.AlignHCenter

    spacing: Geometry.spacing.xxs

    Text {
        id: valueText
        Layout.fillWidth: true
        text: root.value
        color: root.valueColor
        font.pixelSize: Styles.fontSize.titleMedium
        font.weight: Styles.fontWeight.bold
        horizontalAlignment: root.alignment
        // Shrinking beats eliding a number: three of these across a 360px
        // window leave about 64px each, which "1h 30m" overruns.
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: Styles.fontSize.small
        elide: Text.ElideRight
    }

    Text {
        id: labelText
        Layout.fillWidth: true
        text: root.label
        color: root.labelColor
        font.pixelSize: Styles.fontSize.small
        horizontalAlignment: root.alignment
        elide: Text.ElideRight
    }
}
