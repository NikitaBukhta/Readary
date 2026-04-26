import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import Library

Rectangle {
    id: root

    property string label: qsTr("Monthly goal")
    property int current: 0
    property int total: 0
    property string unit: qsTr("books")

    readonly property real progress: total > 0 ? Math.min(1, current / total) : 0

    implicitHeight: 90
    radius: Geometry.radius.lg
    color: Theme.surface
    border.width: 0

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: Theme.shadow
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 4
        shadowBlur: 0.6
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Geometry.spacing.xl
        spacing: Geometry.spacing.lg

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Geometry.spacing.xs

            Text {
                text: root.label.toUpperCase()
                color: Theme.textSecondary
                font.pixelSize: Styles.fontSize.caption
                font.weight: Styles.fontWeight.semibold
                font.letterSpacing: 0.8
            }

            Text {
                text: qsTr("%1/%2 %3").arg(root.current).arg(root.total).arg(root.unit)
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleMedium
                font.weight: Styles.fontWeight.bold
            }
        }

        Item {
            Layout.preferredWidth: Geometry.size.goalRing
            Layout.preferredHeight: Geometry.size.goalRing

            ProgressRing {
                anchors.fill: parent
                progress: root.progress
            }

            Text {
                anchors.centerIn: parent
                text: qsTr("%1%").arg(Math.round(root.progress * 100))
                color: Theme.primary
                font.pixelSize: Styles.fontSize.bodySmall
                font.weight: Styles.fontWeight.bold
            }
        }
    }
}
