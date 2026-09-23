pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property alias title: header.title
    // [{isbn, name, pagesRead, totalPages}], most recently read first.
    property var books: []

    signal bookClicked(var isbn)

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.md

        SectionHeader {
            id: header
            Layout.fillWidth: true
            titleSize: Styles.fontSize.title
        }

        Repeater {
            id: bookRepeater
            model: root.books ?? []

            // A plain Item with its own MouseArea rather than TouchTarget: the
            // row spans the card, and TouchTarget sizes itself from its content.
            delegate: Item {
                id: bookRow

                required property var modelData
                // An unknown length has nothing to fill a bar against.
                readonly property bool _hasLength: bookRow.modelData.totalPages > 0

                Layout.fillWidth: true
                implicitHeight: rowLayout.implicitHeight

                ColumnLayout {
                    id: rowLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    spacing: Geometry.spacing.xs

                    RowLayout {
                        id: rowHeading
                        Layout.fillWidth: true
                        spacing: Geometry.spacing.sm

                        Text {
                            id: bookName
                            Layout.fillWidth: true
                            text: bookRow.modelData.name
                            color: Theme.textPrimary
                            font.pixelSize: Styles.fontSize.body
                            font.weight: Styles.fontWeight.semibold
                            elide: Text.ElideRight
                        }

                        Text {
                            id: bookPages
                            text: bookRow._hasLength ? qsTr("%1 / %2").arg(bookRow.modelData.pagesRead).arg(bookRow.modelData.totalPages) : qsTr("Page %1").arg(bookRow.modelData.pagesRead)
                            color: Theme.textSecondary
                            font.pixelSize: Styles.fontSize.small
                        }
                    }

                    ProgressBar {
                        id: bookProgress
                        Layout.fillWidth: true
                        Layout.preferredHeight: Styles.progressBar.md
                        progress: bookRow._hasLength ? bookRow.modelData.pagesRead / bookRow.modelData.totalPages : 0
                        trackColor: Theme.primarySoft
                        progressColor: Theme.primary
                    }
                }

                MouseArea {
                    id: rowPress
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.bookClicked(bookRow.modelData.isbn)
                }
            }
        }
    }
}
