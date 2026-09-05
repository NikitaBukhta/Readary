import QtQuick
import QtQuick.Layouts
import Library

// One journal entry on a vertical timeline. The rail spans the full row height
// so consecutive rows form one continuous line — the hosting list must not add
// spacing between them.
Item {
    id: root

    property var startedAt: null
    property int pagesFrom: 0
    property int pagesTo: 0
    property int pagesRead: 0
    property int durationSeconds: 0
    // No dangling ends: the first row hides the segment above its dot, the last
    // row the one below.
    property bool railAbove: true
    property bool railBelow: true

    signal deleteRequested

    readonly property int _verticalPadding: Geometry.spacing.sm

    implicitHeight: 2 * root._verticalPadding + Math.max(textColumn.implicitHeight, deleteButton.implicitHeight)

    Item {
        id: gutter
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Geometry.size.timelineGutter

        Rectangle {
            id: railTop
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.bottom: dot.top
            width: Geometry.size.timelineRail
            color: Theme.primarySoft
            visible: root.railAbove
        }

        Rectangle {
            id: dot
            // On the stamp's line, not the row's middle: the rail reads as a
            // series of events rather than a list of blocks.
            y: root._verticalPadding + (stampText.height - height) / 2
            anchors.horizontalCenter: parent.horizontalCenter
            width: Geometry.size.timelineDot
            height: width
            radius: width / 2
            color: Theme.primary
        }

        Rectangle {
            id: railBottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: dot.bottom
            anchors.bottom: parent.bottom
            width: Geometry.size.timelineRail
            color: Theme.primarySoft
            visible: root.railBelow
        }
    }

    ColumnLayout {
        id: textColumn
        anchors.left: gutter.right
        anchors.leftMargin: Geometry.spacing.sm
        anchors.right: durationText.left
        anchors.rightMargin: Geometry.spacing.md
        // Pinned to the top, not centred: when the delete button is the taller
        // side, a centred column slides the stamp out from under its dot.
        anchors.top: parent.top
        anchors.topMargin: root._verticalPadding
        spacing: 0

        Text {
            id: stampText
            Layout.fillWidth: true
            text: Format.stamp(root.startedAt)
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.bodyLarge
            font.weight: Styles.fontWeight.bold
            elide: Text.ElideRight
        }

        RowLayout {
            id: pagesRow
            Layout.fillWidth: true
            spacing: Geometry.spacing.xs

            Text {
                id: pagesText
                text: qsTr("p. %1 → %2").arg(root.pagesFrom).arg(root.pagesTo)
                color: Theme.textSecondary
                font.pixelSize: Styles.fontSize.small
                elide: Text.ElideRight
            }

            Text {
                id: deltaText
                visible: root.pagesRead > 0
                text: qsTr("· +%1").arg(root.pagesRead)
                color: Theme.primary
                font.pixelSize: Styles.fontSize.small
                font.weight: Styles.fontWeight.semibold
            }

            Item {
                id: pagesSpacer
                Layout.fillWidth: true
            }
        }
    }

    Text {
        id: durationText
        anchors.right: deleteButton.left
        anchors.rightMargin: Geometry.spacing.md
        anchors.verticalCenter: parent.verticalCenter
        text: Format.duration(root.durationSeconds)
        color: Theme.primary
        font.pixelSize: Styles.fontSize.body
        font.weight: Styles.fontWeight.semibold
    }

    IconButton {
        id: deleteButton
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        diameter: Geometry.size.iconTile
        // A tile, not a circle — and flat, since the row has no card.
        radius: Geometry.radius.sm
        restColor: Theme.primarySoft
        iconColor: Theme.primary
        iconTinted: true
        iconGlyph: "🗑"
        shadowOffset: 0
        shadowBlur: 0
        onClicked: root.deleteRequested()
    }
}
