import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property alias title: titleText.text
    property alias author: authorText.text
    property int year: 0
    property string publisher: ""
    property int totalPages: 0
    property int rating: 0
    property int ratingTotal: 10
    property alias coverSource: coverImage.source
    property alias hasMenu: menuButton.visible

    signal backClicked
    signal menuClicked

    spacing: Geometry.spacing.lg

    Item {
        id: backRow
        Layout.fillWidth: true
        Layout.preferredHeight: backButton.implicitHeight

        TouchTarget {
            id: backButton
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.backClicked()

            IconGlyph {
                id: backIcon
                glyph: "←"
                color: Theme.textPrimary
                size: Geometry.size.iconLg
            }
        }

        TouchTarget {
            id: menuButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: false
            onClicked: root.menuClicked()

            IconGlyph {
                id: menuIcon
                glyph: "⋮"
                color: Theme.textPrimary
                size: Geometry.size.iconLg
            }
        }
    }

    RowLayout {
        id: heroRow
        Layout.fillWidth: true
        spacing: Geometry.spacing.lg

        SurfaceCard {
            id: cover
            Layout.preferredWidth: Geometry.size.bookDetailCoverWidth
            Layout.preferredHeight: Geometry.size.bookDetailCoverHeight
            Layout.alignment: Qt.AlignTop
            radius: Geometry.radius.md
            color: Theme.primarySoft
            clip: true
            shadowOffset: Styles.elevation.heroOffset
            shadowBlur: Styles.elevation.heroBlur

            Image {
                id: coverImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready
            }

            IconGlyph {
                id: coverPlaceholder
                anchors.centerIn: parent
                visible: !coverImage.visible
                glyph: "📖"
                color: Theme.primary
                size: Geometry.size.iconHuge
            }
        }

        ColumnLayout {
            id: info
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Geometry.spacing.xs

            Text {
                id: titleText
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleMedium
                font.weight: Styles.fontWeight.bold
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Text {
                id: authorText
                Layout.fillWidth: true
                color: Theme.textSecondary
                font.pixelSize: Styles.fontSize.body
                font.weight: Styles.fontWeight.medium
                elide: Text.ElideRight
            }

            Text {
                id: metaText
                Layout.fillWidth: true
                text: {
                    const parts = [];
                    if (root.year > 0)
                        parts.push(root.year);
                    if (root.publisher.length > 0)
                        parts.push(root.publisher);
                    if (root.totalPages > 0)
                        parts.push(qsTr("%n page(s)", "", root.totalPages));
                    return parts.join(" · ");
                }
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.bodySmall
                elide: Text.ElideRight
            }

            RowLayout {
                id: ratingRow
                Layout.fillWidth: true
                Layout.topMargin: Geometry.spacing.xs
                spacing: Geometry.spacing.sm
                visible: root.rating > 0

                StarRating {
                    id: stars
                    value: root.rating
                    total: root.ratingTotal
                    starSize: Styles.fontSize.body
                }

                Text {
                    id: ratingValueText
                    Layout.fillWidth: true
                    text: qsTr("%1/%2").arg(root.rating).arg(root.ratingTotal)
                    color: Theme.textSecondary
                    font.pixelSize: Styles.fontSize.bodySmall
                    font.weight: Styles.fontWeight.semibold
                }
            }
        }
    }
}
