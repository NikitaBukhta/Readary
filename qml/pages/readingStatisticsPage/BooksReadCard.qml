pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property alias title: header.title
    property alias emptyText: emptyText.text
    property var books: []

    signal bookClicked(var isbn)

    readonly property var _books: root.books ?? []

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.md

        SectionHeader {
            id: header
            Layout.fillWidth: true
            titleSize: Styles.fontSize.title
        }

        Text {
            id: emptyText
            Layout.fillWidth: true
            visible: root._books.length === 0
            color: Theme.textMuted
            font.pixelSize: Styles.fontSize.body
            wrapMode: Text.WordWrap
        }

        Repeater {
            id: bookRepeater
            model: root._books

            delegate: Item {
                id: bookRow

                required property var modelData
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
                            id: gained
                            visible: bookRow.modelData.pagesInPeriod > 0
                            text: qsTr("+%1").arg(bookRow.modelData.pagesInPeriod)
                            color: Theme.primary
                            font.pixelSize: Styles.fontSize.small
                            font.weight: Styles.fontWeight.semibold
                        }

                        Text {
                            id: position
                            text: bookRow._hasLength ? qsTr("%1 / %2").arg(bookRow.modelData.toPage).arg(bookRow.modelData.totalPages) : qsTr("Page %1").arg(bookRow.modelData.toPage)
                            color: Theme.textSecondary
                            font.pixelSize: Styles.fontSize.small
                        }
                    }

                    ProgressBar {
                        id: bookProgress
                        Layout.fillWidth: true
                        Layout.preferredHeight: Styles.progressBar.md
                        progress: bookRow._hasLength ? bookRow.modelData.toPage / bookRow.modelData.totalPages : 0
                        startProgress: bookRow._hasLength && bookRow.modelData.pagesInPeriod > 0 ? bookRow.modelData.fromPage / bookRow.modelData.totalPages : 0
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
