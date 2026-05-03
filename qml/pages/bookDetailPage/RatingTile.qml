import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property string label: ""
    property real value: 0
    property int total: 10
    property int decimals: 0
    property bool showStars: false

    readonly property bool _hasRating: root.value > 0

    spacing: Geometry.spacing.xs

    Text {
        id: labelText
        Layout.fillWidth: true
        visible: root.label.length > 0
        text: root.label.toLocaleUpperCase()
        color: Theme.textSecondary
        font.pixelSize: Styles.fontSize.caption
        font.weight: Styles.fontWeight.semibold
        font.letterSpacing: 0.8
        horizontalAlignment: Text.AlignHCenter
    }

    RowLayout {
        id: ratedRow
        Layout.alignment: Qt.AlignHCenter
        visible: root._hasRating
        spacing: Geometry.spacing.xs

        Text {
            id: valueText
            text: root.value.toFixed(root.decimals)
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.titleLarge
            font.weight: Styles.fontWeight.bold
        }

        Text {
            id: totalText
            text: "/ " + root.total
            color: Theme.textMuted
            font.pixelSize: Styles.fontSize.body
            font.weight: Styles.fontWeight.medium
        }
    }

    Text {
        id: notRatedText
        Layout.alignment: Qt.AlignHCenter
        visible: !root._hasRating
        text: qsTr("Not rated")
        color: Theme.textMuted
        font.pixelSize: Styles.fontSize.body
        font.weight: Styles.fontWeight.medium
    }

    StarRating {
        id: stars
        Layout.alignment: Qt.AlignHCenter
        visible: root.showStars && root._hasRating
        value: root.value
        total: root.total
        starSize: Styles.fontSize.body
    }
}
