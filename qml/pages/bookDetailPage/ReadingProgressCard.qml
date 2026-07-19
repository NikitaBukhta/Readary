import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property int pagesRead: 0
    property int pagesTotal: 0
    property string actionLabel: qsTr("Continue reading")

    readonly property real progress: pagesTotal > 0 ? Math.min(1, pagesRead / pagesTotal) : 0
    readonly property bool _hasPages: root.pagesTotal > 0

    signal startReadingRequested

    function beginReading() {
        progressTimer.phase = ReadingPhase.Running;
    }

    function stopReading() {
        progressTimer.phase = ReadingPhase.Stopped;
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.md

        ColumnLayout {
            id: progressBlock
            Layout.fillWidth: true
            visible: root._hasPages
            spacing: Geometry.spacing.md

            RowLayout {
                id: progressLabelRow
                Layout.fillWidth: true

                Text {
                    id: progressLabel
                    Layout.fillWidth: true
                    text: qsTr("Progress")
                    color: Theme.textPrimary
                    font.pixelSize: Styles.fontSize.bodyLarge
                    font.weight: Styles.fontWeight.semibold
                }

                Text {
                    id: progressPercent
                    text: qsTr("%1%").arg(Math.round(root.progress * 100))
                    color: Theme.primary
                    font.pixelSize: Styles.fontSize.bodyLarge
                    font.weight: Styles.fontWeight.bold
                }
            }

            ProgressBar {
                id: bar
                Layout.fillWidth: true
                Layout.preferredHeight: Styles.progressBar.lg
                progress: root.progress
                trackColor: Theme.primarySoft
                progressColor: Theme.primary
            }

            Text {
                id: pagesText
                Layout.fillWidth: true
                text: qsTr("%1 of %n page(s)", "", root.pagesTotal).arg(root.pagesRead)
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.small
            }
        }

        PrimaryButton {
            id: actionButton
            visible: !progressTimer.active
            Layout.fillWidth: true
            Layout.topMargin: root._hasPages ? Geometry.spacing.sm : 0
            label: root.actionLabel
            iconGlyph: "▶"
            onClicked: root.startReadingRequested()
        }

        ReadingProgressTimer {
            id: progressTimer
            visible: progressTimer.active
            Layout.fillWidth: true
            Layout.topMargin: root._hasPages ? Geometry.spacing.sm : 0
            currentPage: root.pagesRead
            pagesTotal: root.pagesTotal
            onEndSessionConfirmed: (pageNumber, durationSeconds) => {
                BookController.updateReadingProgress(pageNumber, durationSeconds);
            }
        }
    }
}
