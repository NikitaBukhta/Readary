import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property string title: ""
    property string author: ""
    property url coverSource
    property color coverFallbackColor: Theme.primarySoft
    property int pagesRead: 0
    property int pagesTotal: 0

    readonly property real progress: pagesTotal > 0
                                     ? Math.min(1, pagesRead / pagesTotal) : 0

    signal clicked()

    implicitWidth: Geometry.size.readingCardWidth
    implicitHeight: coverContainer.height + meta.implicitHeight
                    + Geometry.spacing.md

    ColumnLayout {
        anchors.fill: parent
        spacing: Geometry.spacing.sm

        Item {
            id: coverContainer
            Layout.fillWidth: true
            Layout.preferredHeight: Geometry.size.readingCoverHeight

            Rectangle {
                id: coverFallback
                anchors.fill: parent
                radius: Geometry.radius.md
                color: root.coverFallbackColor
                visible: !cover.visible
            }

            // For rounded-corner clipping on the image wrap this Image with a
            // MultiEffect { maskEnabled: true; maskSource: coverFallback }.
            Image {
                id: cover
                anchors.fill: parent
                source: root.coverSource
                fillMode: Image.PreserveAspectCrop
                visible: source.toString().length > 0 && status === Image.Ready
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.clicked()
            }

            ProgressBar {
                id: coverProgress
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: Geometry.spacing.md
                anchors.rightMargin: Geometry.spacing.md
                anchors.bottomMargin: Geometry.spacing.md
                height: 5
                progress: root.progress
                trackColor: Qt.rgba(1, 1, 1, 0.45)
                progressColor: Theme.primary
            }

            Text {
                anchors.right: coverProgress.right
                anchors.bottom: coverProgress.top
                anchors.bottomMargin: Geometry.spacing.xs
                text: qsTr("%1/%2").arg(root.pagesRead).arg(root.pagesTotal)
                color: "#FFFFFF"
                font.pixelSize: Styles.fontSize.caption
                font.weight: Styles.fontWeight.semibold
                style: Text.Raised
                styleColor: Qt.rgba(0, 0, 0, 0.4)
            }
        }

        ColumnLayout {
            id: meta
            Layout.fillWidth: true
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.body
                font.weight: Styles.fontWeight.semibold
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                Layout.fillWidth: true
                text: root.author
                color: Theme.primary
                font.pixelSize: Styles.fontSize.small
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }
}
