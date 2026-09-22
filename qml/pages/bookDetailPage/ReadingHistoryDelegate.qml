import QtQuick
import Library

Item {
    id: root

    property var sessionData: ({})
    property int sidePadding: 0
    property alias railAbove: row.railAbove
    property alias railBelow: row.railBelow

    // A string: a QML signal cannot carry qint64, and an int would clip the id.
    signal deleteRequested(string sessionId)

    implicitHeight: row.implicitHeight

    ReadingHistoryRow {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.sidePadding
        anchors.rightMargin: root.sidePadding

        startedAt: root.sessionData.startedAt ?? null
        pagesFrom: root.sessionData.pagesFrom ?? 0
        pagesTo: root.sessionData.pagesTo ?? 0
        pagesRead: root.sessionData.pagesRead ?? 0
        durationSeconds: root.sessionData.durationSeconds ?? 0

        onDeleteRequested: root.deleteRequested(String(root.sessionData.id ?? 0))
    }
}
