import QtQuick
import QtQml
import QtQuick.Controls
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property int phase: ReadingPhase.Stopped
    property int seconds: 0
    property int currentPage: 0
    property int pagesTotal: 0
    property string description: qsTr("E-book does not open automatically. The timer tracks reading duration.")

    readonly property bool active: phase !== ReadingPhase.Stopped

    property bool _confirming: false
    property int _pageInput: 0
    property int _phaseBeforeConfirm: ReadingPhase.Stopped
    property string _bookIsbnAtCreation: ""

    signal endSessionConfirmed(int pageNumber, int durationSeconds)

    function _formatDuration(s) {
        const h = Math.floor(s / 3600);
        const m = Math.floor((s % 3600) / 60);
        const sec = s % 60;
        const pad = n => n < 10 ? "0" + n : "" + n;
        return pad(h) + ":" + pad(m) + ":" + pad(sec);
    }

    function _togglePhase() {
        phase = (phase === ReadingPhase.Running) ? ReadingPhase.Paused : ReadingPhase.Running;
    }

    function _openConfirm() {
        _phaseBeforeConfirm = phase;
        if (phase === ReadingPhase.Running)
            phase = ReadingPhase.Paused;
        _pageInput = currentPage;
        _confirming = true;
    }

    function _cancelConfirm() {
        _confirming = false;
        phase = _phaseBeforeConfirm;
    }

    function _saveConfirm() {
        if (_pageInput < currentPage) {
            ToastService.show(qsTr("Cannot save page lower than current (%1)").arg(currentPage));
            return;
        }
        endSessionConfirmed(_pageInput, seconds);
        phase = ReadingPhase.Stopped;
    }

    // Persist only on state changes: the cache stores lastSyncAt, so a Running
    // session's elapsed time is reconstructed on restore — no periodic flushing
    // needed. Writing here (start/pause/resume/stop) means a crash mid-session
    // still recovers, because the Running phase was stamped when it began.
    function _persistState() {
        if (_bookIsbnAtCreation.length === 0)
            return;
        if (phase === ReadingPhase.Stopped)
            BookController.clearReadingSession(_bookIsbnAtCreation);
        else
            BookController.saveReadingSession(_bookIsbnAtCreation, seconds, phase);
    }

    onPhaseChanged: {
        if (phase === ReadingPhase.Stopped) {
            seconds = 0;
            _confirming = false;
        } else if (phase === ReadingPhase.Running) {
            BookController.setBookStatus(BookStatus.InProgress);
        }
        _persistState();
    }

    Component.onCompleted: {
        const isbn = BookController.currentBookIsbn;
        _bookIsbnAtCreation = isbn > 0 ? "" + isbn : "";
        if (_bookIsbnAtCreation.length === 0)
            return;

        const cached = BookController.takeReadingSession(_bookIsbnAtCreation);
        if (cached.seconds !== undefined) {
            seconds = cached.seconds;
            phase = cached.phase;
        }
    }

    Component.onDestruction: _persistState()

    Timer {
        id: tick
        interval: 1000
        repeat: true
        running: root.phase === ReadingPhase.Running
        onTriggered: root.seconds += 1
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.md

        RowLayout {
            id: header
            Layout.fillWidth: true
            spacing: Geometry.spacing.md

            IconGlyph {
                id: stopwatchIcon
                Layout.alignment: Qt.AlignVCenter
                glyph: "⏱"
                color: Theme.primary
                size: Geometry.size.iconLg
            }

            ColumnLayout {
                id: timeColumn
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                Text {
                    id: sessionLabel
                    Layout.fillWidth: true
                    text: qsTr("Reading session").toLocaleUpperCase()
                    color: Theme.textSecondary
                    font.pixelSize: Styles.fontSize.caption
                    font.weight: Styles.fontWeight.semibold
                    font.letterSpacing: 0.8
                }

                Text {
                    id: timeText
                    Layout.fillWidth: true
                    text: root._formatDuration(root.seconds)
                    color: Theme.textPrimary
                    font.pixelSize: Styles.fontSize.titleMedium
                    font.weight: Styles.fontWeight.bold
                }
            }

            IconButton {
                id: toggleButton
                Layout.alignment: Qt.AlignVCenter
                iconGlyph: root.phase === ReadingPhase.Running ? "⏸" : "▶"
                iconColor: Theme.primaryContent
                restColor: Theme.primary
                onClicked: root._togglePhase()
            }

            IconButton {
                id: resetButton
                Layout.alignment: Qt.AlignVCenter
                iconGlyph: "🔄"
                iconColor: Theme.textSecondary
                restColor: Theme.surfaceVariant
                onClicked: root.phase = ReadingPhase.Stopped
            }
        }

        Text {
            id: descriptionText
            Layout.fillWidth: true
            visible: root.description.length > 0
            text: root.description
            color: Theme.textSecondary
            font.pixelSize: Styles.fontSize.bodySmall
            wrapMode: Text.WordWrap
        }

        SecondaryButton {
            id: endButton
            Layout.fillWidth: true
            label: qsTr("End session and save progress")
            iconGlyph: "🔖"
            onClicked: root._openConfirm()
        }

        ColumnLayout {
            id: confirmForm
            Layout.fillWidth: true
            Layout.topMargin: Geometry.spacing.md
            visible: root._confirming
            spacing: Geometry.spacing.md

            Text {
                id: confirmTitle
                Layout.fillWidth: true
                text: qsTr("Where did you stop?")
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.bodyLarge
                font.weight: Styles.fontWeight.semibold
            }

            RowLayout {
                id: pageRow
                Layout.fillWidth: true
                spacing: Geometry.spacing.sm

                TextField {
                    id: pageInput
                    Layout.fillWidth: true
                    text: root._pageInput
                    color: Theme.textPrimary
                    font.pixelSize: Styles.fontSize.titleLarge
                    font.weight: Styles.fontWeight.bold
                    background: null
                    selectByMouse: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator {
                        bottom: 0
                    }
                    onTextEdited: {
                        const n = parseInt(text, 10);
                        if (!isNaN(n)) {
                            if (n > root.pagesTotal) {
                                ToastService.show(qsTr("Cannot save page higher than total (%1)").arg(root.pagesTotal));
                            }

                            const max = root.pagesTotal > 0 ? root.pagesTotal : n;
                            root._pageInput = Math.max(0, Math.min(n, max));
                        }
                    }
                }

                Text {
                    id: pageTotalText
                    visible: root.pagesTotal > 0
                    text: "/ " + root.pagesTotal
                    color: Theme.textMuted
                    font.pixelSize: Styles.fontSize.titleLarge
                    font.weight: Styles.fontWeight.medium
                }
            }

            RowLayout {
                id: confirmActions
                Layout.fillWidth: true
                spacing: Geometry.spacing.md

                TextButton {
                    id: cancelButton
                    Layout.alignment: Qt.AlignVCenter
                    label: qsTr("Cancel")
                    onClicked: root._cancelConfirm()
                }

                Item {
                    id: spacer
                    Layout.fillWidth: true
                }

                PrimaryButton {
                    id: saveButton
                    Layout.preferredWidth: Geometry.size.dialogPrimaryWidth
                    label: qsTr("Save")
                    onClicked: root._saveConfirm()
                }
            }
        }
    }
}
