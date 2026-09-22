pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import Library

// Optional PDF for the book being added. Picking one parses it through
// BookController and reports the numbers back so the form can overwrite itself:
// the file is harder evidence than anything typed by hand.
ColumnLayout {
    id: root

    property alias label: caption.text

    readonly property bool attached: root._fileName.length > 0

    signal parsed(var info)
    signal cleared

    property string _fileName: ""
    property int _pageCount: 0

    function detach(): void {
        BookController.clearStagedPdf();
        root._fileName = "";
        root._pageCount = 0;
        root.cleared();
    }

    spacing: Geometry.spacing.xs

    FieldLabel {
        id: caption
        Layout.fillWidth: true
        visible: root.label.length > 0
    }

    PressableSurface {
        id: slot
        Layout.fillWidth: true
        implicitHeight: Geometry.size.pillButtonHeight
        radius: Geometry.radius.md
        restColor: Theme.searchBackground
        pressedColor: Theme.primarySoft
        // Flat: it sits in a column of input boxes, not among cards.
        shadowOffset: 0
        shadowBlur: 0
        onClicked: pdfDialog.open()

        RowLayout {
            id: content
            anchors.fill: parent
            anchors.leftMargin: Geometry.spacing.md
            anchors.rightMargin: Geometry.spacing.sm
            spacing: Geometry.spacing.sm

            IconGlyph {
                id: fileIcon
                glyph: "📄"
                color: Theme.textMuted
                size: Geometry.size.iconMd
            }

            Text {
                id: nameText
                Layout.fillWidth: true
                text: root.attached ? root._fileName : qsTr("Attach a PDF")
                color: root.attached ? Theme.textPrimary : Theme.textMuted
                font.pixelSize: Styles.fontSize.body
                elide: Text.ElideMiddle
            }

            Text {
                id: pagesText
                visible: root._pageCount > 0
                text: qsTr("%n page(s)", "", root._pageCount)
                color: Theme.primary
                font.pixelSize: Styles.fontSize.bodySmall
                font.weight: Styles.fontWeight.semibold
            }

            IconButton {
                id: detachButton
                visible: root.attached
                diameter: Geometry.size.iconTile
                radius: Geometry.radius.sm
                restColor: Theme.primarySoft
                iconColor: Theme.primary
                iconTinted: true
                iconGlyph: "🗑"
                shadowOffset: 0
                shadowBlur: 0
                onClicked: root.detach()
            }
        }
    }

    FileDialog {
        id: pdfDialog
        title: qsTr("Choose a PDF")
        nameFilters: [qsTr("PDF documents (*.pdf)")]
        onAccepted: {
            const info = BookController.stagePdf(pdfDialog.selectedFile);
            if (!info.ok) {
                ToastService.show(BookController.errorMessage);
                root.detach();
                return;
            }
            root._fileName = pdfDialog.selectedFile.toString().split("/").pop();
            root._pageCount = info.pageCount ?? 0;
            root.parsed(info);
        }
    }
}
