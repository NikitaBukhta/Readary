import QtQuick
import QtQuick.Layouts
import Library

// A figure over its caption. The statistics page stacks these three across a
// card; anything that needs "big number, small label" can use it.
ColumnLayout {
    id: root

    property alias value: valueText.text
    property alias label: labelText.text
    property alias valueColor: valueText.color
    property alias labelColor: labelText.color
    property alias alignment: valueText.horizontalAlignment

    spacing: Geometry.spacing.xxs

    Text {
        id: valueText
        Layout.fillWidth: true
        color: Theme.textPrimary
        font.pixelSize: Styles.fontSize.titleMedium
        font.weight: Styles.fontWeight.bold
        horizontalAlignment: Text.AlignHCenter
        // Shrinking beats eliding a number: three of these across a 360px
        // window leave about 64px each, which "1h 30m" overruns.
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: Styles.fontSize.small
        elide: Text.ElideRight
    }

    Text {
        id: labelText
        Layout.fillWidth: true
        color: Theme.textMuted
        font.pixelSize: Styles.fontSize.small
        horizontalAlignment: root.alignment
        elide: Text.ElideRight
    }
}
