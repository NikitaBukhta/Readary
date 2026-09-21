import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property string title: ""
    property real minimum: 0
    property real average: 0
    property real maximum: 0
    // False when no session was ever timed. Without it a measured 0 p/h — see
    // BookStatisticsDTO::timedSessionCount — would render as "no data".
    property bool hasSpeed: false

    function _format(pagesPerHour: real): string {
        return root.hasSpeed ? Math.round(pagesPerHour).toString() : Format.blank;
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.lg

        SectionHeader {
            id: header
            Layout.fillWidth: true
            title: root.title
            titleSize: Styles.fontSize.title
        }

        RowLayout {
            id: columns
            Layout.fillWidth: true
            spacing: Geometry.spacing.lg

            StatColumn {
                id: slowest
                Layout.fillWidth: true
                value: root._format(root.minimum)
                label: qsTr("Min. p/h")
                alignment: Text.AlignLeft
            }

            StatColumn {
                id: typical
                Layout.fillWidth: true
                value: root._format(root.average)
                label: qsTr("Avg.")
                valueColor: Theme.primary
            }

            StatColumn {
                id: fastest
                Layout.fillWidth: true
                value: root._format(root.maximum)
                label: qsTr("Max.")
                alignment: Text.AlignRight
            }
        }
    }
}
