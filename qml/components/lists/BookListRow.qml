import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import Library

Rectangle {
    id: root

    property alias name: nameText.text
    property alias author: authorText.text
    property string type: ""
    property int year: 0
    property alias coverSource: cover.source
    property alias coverFallbackColor: coverFallback.color

    signal clicked

    implicitHeight: Geometry.size.bookRowHeight
    radius: Geometry.radius.lg
    color: Theme.surface

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: Theme.shadow
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 3
        shadowBlur: 0.5
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Geometry.spacing.md
        spacing: Geometry.spacing.lg

        Item {
            id: coverContainer
            Layout.preferredWidth: Geometry.size.bookRowCoverWidth
            Layout.preferredHeight: Geometry.size.bookRowCoverHeight
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                id: coverFallback
                anchors.fill: parent
                radius: Geometry.radius.md
                color: Theme.primarySoft
                visible: !cover.visible
            }

            Image {
                id: cover
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                visible: source.toString().length > 0 && status === Image.Ready
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: Geometry.spacing.xxs

            Text {
                id: nameText
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.bodyLarge
                font.weight: Styles.fontWeight.semibold
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                id: authorText
                Layout.fillWidth: true
                color: Theme.textSecondary
                font.pixelSize: Styles.fontSize.body
                elide: Text.ElideRight
                maximumLineCount: 1
                visible: text.length > 0
            }

            Text {
                Layout.fillWidth: true
                text: {
                    const parts = [];
                    if (root.type.length > 0)
                        parts.push(root.type);
                    if (root.year > 0)
                        parts.push(root.year);
                    return parts.join(" · ");
                }
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.small
                elide: Text.ElideRight
                maximumLineCount: 1
                visible: text.length > 0
            }
        }
    }
}
