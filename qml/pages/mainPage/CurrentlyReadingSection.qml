pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property var model
    property int sidePadding: Geometry.spacing.xxl

    signal bookOpened(int bookIndex)

    spacing: Geometry.spacing.md

    SectionHeader {
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        title: qsTr("Currently reading")
        trailingText: list.count > 0 ? list.count.toString() : ""
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.preferredHeight: Geometry.size.readingCoverHeight + 2 * Styles.fontSize.body + Geometry.spacing.lg
        orientation: ListView.Horizontal
        model: root.model
        spacing: Geometry.spacing.lg
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        // Side padding via header/footer keeps swipe physics natural.
        header: Item {
            width: root.sidePadding
        }
        footer: Item {
            width: root.sidePadding
        }

        delegate: ReadingBookCard {
            required property var model
            required property int index
            title: model.title
            author: model.author
            coverSource: model.coverSource
            pagesRead: model.pagesRead
            pagesTotal: model.pagesTotal
            onClicked: root.bookOpened(index)
        }
    }
}
