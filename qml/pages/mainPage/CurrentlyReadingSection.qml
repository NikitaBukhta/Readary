pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property alias model: list.model
    property int sidePadding: Geometry.spacing.xxl
    property alias title: header.title

    signal bookOpened(real isbn)

    spacing: Geometry.spacing.md

    SectionHeader {
        id: header
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        title: qsTr("Currently reading")
        trailingText: list.count > 0 ? list.count.toString() : ""
    }

    // Off-screen sizer: feeds list its preferredHeight without depending on a delegate instance.
    ReadingBookCard {
        id: cardSizer
        visible: false
        enabled: false
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.preferredHeight: cardSizer.implicitHeight
        orientation: ListView.Horizontal
        spacing: Geometry.spacing.lg
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        // Side padding via header/footer keeps swipe momentum natural at the ends.
        header: Item {
            width: root.sidePadding
        }
        footer: Item {
            width: root.sidePadding
        }

        delegate: ReadingBookCard {
            required property var model
            required property int index
            title: model.name ?? ""
            author: model.author ?? ""
            coverSource: model.coverUrl ?? ""
            pagesRead: model.pagesRead ?? 0
            pagesTotal: model.totalPages ?? 0
            onClicked: root.bookOpened(model.isbn)
        }
    }
}
