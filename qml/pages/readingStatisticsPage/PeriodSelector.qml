pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

RowLayout {
    id: root

    property int period: ReadingStatisticsController.AllTime
    property alias rangeText: rangeCaption.text

    signal periodPicked(int period)
    signal customRequested

    readonly property var _options: [
        {
            period: ReadingStatisticsController.Day,
            label: qsTr("Day")
        },
        {
            period: ReadingStatisticsController.Week,
            label: qsTr("Week")
        },
        {
            period: ReadingStatisticsController.Month,
            label: qsTr("Month")
        },
        {
            period: ReadingStatisticsController.Year,
            label: qsTr("Year")
        },
        {
            period: ReadingStatisticsController.AllTime,
            label: qsTr("All time")
        },
        {
            period: ReadingStatisticsController.Custom,
            label: qsTr("Custom"),
            separatorBefore: true
        }
    ]

    readonly property string _currentLabel: root._options.find(option => option.period === root.period)?.label ?? ""

    function _pick(actionId: string): void {
        const picked = Number(actionId);
        if (picked === ReadingStatisticsController.Custom)
            root.customRequested();
        else
            root.periodPicked(picked);
    }

    spacing: Geometry.spacing.md

    FilterChip {
        id: trigger
        selected: true
        label: root._currentLabel + "  ▾"
        onClicked: periodMenu.open()

        ActionMenu {
            id: periodMenu
            y: trigger.height + Geometry.spacing.xs
            actions: root._options.map(option => ({
                        id: String(option.period),
                        label: option.label,
                        selected: option.period === root.period,
                        separatorBefore: option.separatorBefore ?? false
                    }))
            onTriggered: actionId => root._pick(actionId)
        }
    }

    Text {
        id: rangeCaption
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        visible: rangeCaption.text.length > 0
        color: Theme.textSecondary
        font.pixelSize: Styles.fontSize.small
        elide: Text.ElideRight
    }
}
