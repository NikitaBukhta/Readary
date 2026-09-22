pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    readonly property int _sidePadding: Geometry.spacing.xxl
    readonly property var _stats: ProfileController.statistics

    // `page` is the NavigationController page a row opens; 0 is no page at all
    // (Page starts at MainPage = 1), which ProfileMenuRow renders as inert.
    readonly property var _sections: [
        {
            glyph: "📊",
            label: qsTr("Reading statistics"),
            page: 0
        },
        {
            glyph: "⚙️",
            label: qsTr("Settings"),
            page: NavigationController.SettingsPage
        },
        {
            glyph: "🔄",
            label: qsTr("Sync"),
            page: 0
        },
        {
            glyph: "🛡️",
            label: qsTr("Privacy"),
            page: 0
        },
        {
            glyph: "📵",
            label: qsTr("Offline mode"),
            page: 0
        }
    ]

    // The page lives behind Main.qml's Loader, so it is rebuilt on every open
    // while the controller's figures are not — a book finished elsewhere since
    // would otherwise still be missing here. The controller stays quiet when
    // nothing actually changed.
    Component.onCompleted: ProfileController.refresh()

    function _levelTitle(level) {
        switch (level) {
        case ProfileController.Bibliophile:
            return qsTr("Bibliophile");
        case ProfileController.Bookworm:
            return qsTr("Bookworm");
        case ProfileController.Reader:
            return qsTr("Reader");
        default:
            return qsTr("New reader");
        }
    }

    ColumnLayout {
        id: page
        anchors.fill: parent
        spacing: 0

        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: root._sidePadding
            spacing: Geometry.spacing.md

            Text {
                id: titleText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Profile")
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleLarge
                font.weight: Styles.fontWeight.bold
                elide: Text.ElideRight
            }
        }

        Flickable {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Geometry.spacing.xl
            contentWidth: width
            contentHeight: content.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                id: scrollBar
                policy: ScrollBar.AlwaysOff
            }

            // Inset once here rather than per card — every child spans the
            // same column.
            ColumnLayout {
                id: content
                x: root._sidePadding
                width: scroll.width - (2 * root._sidePadding)
                spacing: Geometry.spacing.lg

                ProfileHeaderCard {
                    id: headerCard
                    Layout.fillWidth: true
                    title: root._levelTitle(ProfileController.readerLevel)
                    subtitle: qsTr("%n book(s) read", "", root._stats.booksFinished)
                }

                RowLayout {
                    id: tiles
                    Layout.fillWidth: true
                    spacing: Geometry.spacing.md

                    StatTile {
                        id: finishedTile
                        Layout.fillWidth: true
                        iconTinted: false
                        iconGlyph: "📖"
                        value: root._stats.booksFinished.toString()
                        label: qsTr("Read")
                    }

                    StatTile {
                        id: timeTile
                        Layout.fillWidth: true
                        iconTinted: false
                        iconGlyph: "⏱️"
                        // The timer is the only source of this figure, so a
                        // library read without it honestly has none.
                        value: root._stats.totalSeconds > 0 ? Format.duration(root._stats.totalSeconds) : Format.blank
                        label: qsTr("Reading")
                    }

                    StatTile {
                        id: pagesTile
                        Layout.fillWidth: true
                        iconTinted: false
                        iconGlyph: "📄"
                        value: Format.compact(root._stats.pagesRead)
                        label: qsTr("Pages")
                    }
                }

                ColumnLayout {
                    id: sections
                    Layout.fillWidth: true
                    Layout.bottomMargin: Geometry.spacing.xxl
                    spacing: Geometry.spacing.md

                    Repeater {
                        id: sectionRepeater
                        model: root._sections

                        delegate: ProfileMenuRow {
                            required property var modelData

                            Layout.fillWidth: true
                            iconGlyph: modelData.glyph
                            label: modelData.label
                            available: modelData.page > 0
                            onClicked: NavigationController.currentPage = modelData.page
                        }
                    }
                }
            }
        }
    }
}
