pragma ComponentBehavior: Bound
import QtQuick
import Library

// Journal of finished reading sessions, newest first, drawn as a timeline.
ListView {
    id: root

    property int sidePadding: 0

    readonly property var historyModel: BookController.readingHistoryModel

    signal deleteRequested(string sessionId)

    visible: root.historyModel && root.historyModel.totalCount > 0
    implicitHeight: contentHeight
    interactive: false
    // Each row draws its own slice of the timeline rail; spacing would cut it.
    spacing: 0
    model: root.historyModel

    header: ReadingHistoryListHeader {
        width: root.width
        sidePadding: root.sidePadding
    }

    delegate: ReadingHistoryDelegate {
        id: sessionDelegate

        required property var model
        required property int index

        width: root.width

        sessionData: sessionDelegate.model
        sidePadding: root.sidePadding
        railAbove: sessionDelegate.index > 0
        railBelow: sessionDelegate.index < root.count - 1

        onDeleteRequested: sessionId => root.deleteRequested(sessionId)
    }

    footer: PagedListToggle {
        width: root.width
        pagedModel: root.historyModel
    }
}
