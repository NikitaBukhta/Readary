pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

RowLayout {
    id: root

    property int count: 0
    property var labels: []

    readonly property bool dense: root.count > Geometry.chart.maxBarLabels
    readonly property int step: Math.max(1, Math.ceil(root.count / Geometry.chart.maxBarLabels))

    Repeater {
        id: captionRepeater
        model: root.count

        delegate: Text {
            id: caption

            required property int index

            Layout.fillWidth: true
            Layout.preferredWidth: 0
            text: caption.index % root.step === 0 && root.labels && caption.index < root.labels.length ? root.labels[caption.index] : ""
            color: Theme.textSecondary
            font.pixelSize: Styles.fontSize.caption
            horizontalAlignment: Text.AlignHCenter
            elide: root.step > 1 ? Text.ElideNone : Text.ElideRight
        }
    }
}
