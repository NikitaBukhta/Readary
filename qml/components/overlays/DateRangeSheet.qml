pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Popup {
    id: root

    property alias title: titleText.text
    property string startIso: ""
    property string endIso: ""

    signal applied(string fromIso, string toIso)

    property string _start: ""
    property string _end: ""
    property int _shownYear: new Date().getFullYear()
    property int _shownMonth: new Date().getMonth()

    function _pick(iso: string): void {
        if (root._start === "" || root._end !== "") {
            root._start = iso;
            root._end = "";
        } else if (iso < root._start) {
            root._end = root._start;
            root._start = iso;
        } else {
            root._end = iso;
        }
    }

    function _showMonth(offset: int): void {
        const shown = new Date(root._shownYear, root._shownMonth + offset, 1);
        root._shownYear = shown.getFullYear();
        root._shownMonth = shown.getMonth();
    }

    function _selectionText(): string {
        return root._start === "" ? qsTr("Tap the first and the last day") : Format.dateRange(root._start, root._end);
    }

    modal: true
    dim: true
    focus: true
    anchors.centerIn: Overlay.overlay
    padding: Geometry.spacing.xl
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min((parent ? parent.width : Geometry.window.defaultWidth) - 2 * Geometry.spacing.xxl, Geometry.window.contentMaxWidth)

    onAboutToShow: {
        root._start = root.startIso;
        root._end = root.endIso;
        const anchorIso = root._end || root._start;
        const anchor = anchorIso ? Format.dateOfIso(anchorIso) : new Date();
        root._shownYear = anchor.getFullYear();
        root._shownMonth = anchor.getMonth();
    }

    background: SurfaceCard {
        shadowOffset: Styles.elevation.heroOffset
        shadowBlur: Styles.elevation.heroBlur
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.45)
    }

    contentItem: ColumnLayout {
        id: content
        spacing: Geometry.spacing.md

        Text {
            id: titleText
            Layout.fillWidth: true
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.title
            font.weight: Styles.fontWeight.bold
        }

        Text {
            id: selectionText
            Layout.fillWidth: true
            text: root._selectionText()
            color: root._start !== "" ? Theme.primary : Theme.textSecondary
            font.pixelSize: Styles.fontSize.body
            font.weight: root._start !== "" ? Styles.fontWeight.semibold : Styles.fontWeight.regular
        }

        RowLayout {
            id: monthNavigation
            Layout.fillWidth: true
            Layout.topMargin: Geometry.spacing.sm

            TouchTarget {
                id: previousMonth
                onClicked: root._showMonth(-1)

                IconGlyph {
                    glyph: "‹"
                    color: Theme.primary
                    size: Geometry.size.iconLg
                }
            }

            Text {
                id: monthTitle
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Qt.locale().standaloneMonthName(root._shownMonth, Locale.LongFormat) + " " + root._shownYear
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.bodyLarge
                font.weight: Styles.fontWeight.semibold
            }

            TouchTarget {
                id: nextMonth
                onClicked: root._showMonth(1)

                IconGlyph {
                    glyph: "›"
                    color: Theme.primary
                    size: Geometry.size.iconLg
                }
            }
        }

        DayOfWeekRow {
            id: weekdays
            Layout.fillWidth: true
            locale: monthGrid.locale

            delegate: Text {
                required property string shortName

                text: shortName
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.caption
                horizontalAlignment: Text.AlignHCenter
            }
        }

        MonthGrid {
            id: monthGrid
            Layout.fillWidth: true
            Layout.preferredHeight: 6 * Geometry.size.avatarSm
            year: root._shownYear
            month: root._shownMonth
            locale: Qt.locale()
            spacing: 0

            delegate: Item {
                id: dayCell

                required property int day
                required property int month
                required property int year
                required property bool today

                readonly property string _iso: Format.isoDate(dayCell.year, dayCell.month + 1, dayCell.day)
                readonly property bool _isEnd: dayCell._iso === root._start || dayCell._iso === root._end
                readonly property bool _isBetween: root._end !== "" && dayCell._iso > root._start && dayCell._iso < root._end

                implicitHeight: Geometry.size.avatarSm
                opacity: dayCell.month === monthGrid.month ? 1.0 : Styles.opacity.disabled

                Rectangle {
                    id: band
                    anchors.verticalCenter: parent.verticalCenter
                    x: dayCell._iso === root._start ? parent.width / 2 : 0
                    width: dayCell._isBetween ? parent.width : (root._end !== "" && root._end !== root._start && dayCell._isEnd ? parent.width / 2 : 0)
                    height: Geometry.size.avatarSm
                    color: Theme.primarySoft
                }

                Rectangle {
                    id: marker
                    anchors.centerIn: parent
                    width: Geometry.size.avatarSm
                    height: Geometry.size.avatarSm
                    radius: width / 2
                    visible: dayCell._isEnd
                    color: Theme.primary
                }

                Text {
                    id: dayNumber
                    anchors.centerIn: parent
                    text: dayCell.day
                    color: dayCell._isEnd ? Theme.primaryContent : (dayCell.today ? Theme.primary : Theme.textPrimary)
                    font.pixelSize: Styles.fontSize.body
                    font.weight: dayCell._isEnd || dayCell.today ? Styles.fontWeight.bold : Styles.fontWeight.regular
                }

                MouseArea {
                    id: dayPress
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root._pick(dayCell._iso)
                }
            }
        }

        RowLayout {
            id: actions
            Layout.fillWidth: true
            Layout.topMargin: Geometry.spacing.sm
            spacing: Geometry.spacing.md

            SecondaryButton {
                id: cancelButton
                Layout.fillWidth: true
                label: qsTr("Cancel")
                onClicked: root.close()
            }

            PrimaryButton {
                id: applyButton
                Layout.fillWidth: true
                label: qsTr("Apply")
                enabled: root._start !== ""
                opacity: applyButton.enabled ? 1.0 : Styles.opacity.disabled
                onClicked: {
                    root.applied(root._start, root._end || root._start);
                    root.close();
                }
            }
        }
    }
}
