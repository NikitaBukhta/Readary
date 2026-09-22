pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import Library

Item {
    id: root

    property alias title: titleText.text
    property alias author: authorText.text
    property alias coverSource: cover.source
    property alias coverFallbackColor: coverFallback.color
    property int pagesRead: 0
    property int pagesTotal: 0

    readonly property real progress: pagesTotal > 0 ? Math.min(1, pagesRead / pagesTotal) : 0

    signal clicked

    implicitWidth: Geometry.size.readingCardWidth
    implicitHeight: layout.implicitHeight

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.sm

        Rectangle {
            id: background
            clip: true
            Layout.fillWidth: true
            Layout.preferredHeight: cardContent.implicitHeight
            radius: Geometry.radius.lg
            color: Theme.surface

            ColumnLayout {
                id: cardContent
                anchors.fill: parent
                spacing: Geometry.spacing.sm

                Item {
                    id: coverContainer
                    Layout.fillWidth: true
                    Layout.preferredHeight: Geometry.size.readingCoverHeight

                    // Alpha mask: rounded top corners for the cover content below.
                    Rectangle {
                        id: coverMask
                        anchors.fill: parent
                        color: "white"
                        topLeftRadius: Geometry.radius.lg
                        topRightRadius: Geometry.radius.lg
                        visible: false
                        layer.enabled: true
                    }

                    Rectangle {
                        id: coverFallback
                        anchors.fill: parent
                        color: Theme.primarySoft
                        visible: !cover.visible
                    }

                    Image {
                        id: cover
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectCrop
                        visible: source.toString().length > 0 && status === Image.Ready
                    }

                    layer.enabled: true
                    layer.effect: MultiEffect {
                        maskEnabled: true
                        maskSource: coverMask
                    }
                }

                ColumnLayout {
                    id: progressColumn
                    Layout.fillWidth: true
                    Layout.leftMargin: Geometry.spacing.md
                    Layout.rightMargin: Geometry.spacing.md
                    Layout.bottomMargin: Geometry.spacing.md
                    spacing: Geometry.spacing.xs

                    ProgressBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Styles.progressBar.md
                        progress: root.progress
                        trackColor: Theme.primarySoft
                        progressColor: Theme.primary
                    }

                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("%1/%2").arg(root.pagesRead).arg(root.pagesTotal)
                        color: Theme.textMuted
                        font.pixelSize: Styles.fontSize.caption
                        font.weight: Styles.fontWeight.semibold
                    }
                }
            }
        }

        ColumnLayout {
            id: meta
            Layout.fillWidth: true
            spacing: 2

            Text {
                id: titleText
                color: Theme.textPrimary
                elide: Text.ElideRight
                font.pixelSize: Styles.fontSize.body
                font.weight: Styles.fontWeight.semibold
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1

                Layout.fillWidth: true
            }

            Text {
                id: authorText
                color: Theme.primary
                elide: Text.ElideRight
                font.pixelSize: Styles.fontSize.small
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1

                Layout.fillWidth: true
            }
        }
    }
}
