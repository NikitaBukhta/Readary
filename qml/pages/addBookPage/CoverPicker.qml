import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import Library

// Tap-to-pick cover slot. Holds the chosen file URL and nothing else; leaving it
// empty is fine, the book then falls back to the usual cover placeholder.
Item {
    id: root

    // What the user picked, and what the form submits.
    property url imageUrl
    // Display-only override: the first page of an attached PDF. It wins over a
    // hand-picked file because the controller applies the same precedence when
    // it stores the book.
    property url previewUrl

    readonly property url _shown: root.previewUrl.toString().length > 0 ? root.previewUrl : root.imageUrl

    implicitWidth: Geometry.size.coverPickerWidth
    implicitHeight: Geometry.size.coverPickerHeight

    Rectangle {
        id: slot
        anchors.fill: parent
        radius: Geometry.radius.md
        color: Theme.surfaceVariant
        clip: true

        Image {
            id: preview
            anchors.fill: parent
            source: root._shown
            fillMode: Image.PreserveAspectCrop
            visible: status === Image.Ready
        }

        ColumnLayout {
            id: hint
            anchors.centerIn: parent
            spacing: Geometry.spacing.xs
            visible: !preview.visible

            IconGlyph {
                id: hintIcon
                Layout.alignment: Qt.AlignHCenter
                glyph: "📷"
                color: Theme.textMuted
                size: Geometry.size.iconXl
            }

            Text {
                id: hintLabel
                Layout.alignment: Qt.AlignHCenter
                // "Book cover", not "Cover": the auto-translator renders the bare
                // word as a lid.
                text: qsTr("Book cover")
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.bodySmall
            }
        }
    }

    DashedOutline {
        id: outline
        anchors.fill: parent
        radius: slot.radius
        visible: !preview.visible
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: coverDialog.open()
    }

    FileDialog {
        id: coverDialog
        title: qsTr("Choose a cover")
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.webp *.bmp)")]
        onAccepted: root.imageUrl = coverDialog.selectedFile
    }
}
