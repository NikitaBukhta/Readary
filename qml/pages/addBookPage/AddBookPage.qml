pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    readonly property int _sidePadding: Geometry.spacing.xxl

    function _applyPdf(info: var): void {
        if (info.pageCount > 0)
            pagesField.setValue(String(info.pageCount));
        if (info.title.length > 0)
            titleField.setValue(info.title);
        if (info.author.length > 0)
            authorField.setValue(info.author);
        if (info.isbn.length > 0)
            isbnField.setValue(info.isbn);

        coverPicker.previewUrl = info.coverPreview ?? "";
        ToastService.show(qsTr("Filled in from the PDF"));
    }

    function _submit(): void {
        const added = BookController.addCustomBook({
            "name": titleField.value,
            "author": authorField.value,
            "year": parseInt(yearField.value) || 0,
            "publisher": publisherField.value,
            "totalPages": parseInt(pagesField.value) || 0,
            "isbnText": isbnField.value,
            "description": descriptionField.value,
            "coverUrl": coverPicker.imageUrl.toString(),
            "genres": genrePicker.value
        });
        if (!added)
            ToastService.show(BookController.errorMessage);
    }

    Component.onDestruction: BookController.clearStagedPdf()

    ColumnLayout {
        id: page
        anchors.fill: parent
        spacing: 0

        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: root._sidePadding
            spacing: Geometry.spacing.sm

            TouchTarget {
                id: backButton
                Layout.alignment: Qt.AlignVCenter
                onClicked: NavigationController.goBack()

                IconGlyph {
                    id: backIcon
                    glyph: "←"
                    color: Theme.primary
                    size: Geometry.size.iconLg
                }
            }

            Text {
                id: titleText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Add book")
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleMedium
                font.weight: Styles.fontWeight.bold
                elide: Text.ElideRight
            }
        }

        Flickable {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Geometry.spacing.lg
            contentWidth: width
            contentHeight: form.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            ColumnLayout {
                id: form
                width: scroll.width
                spacing: Geometry.spacing.lg

                CoverPicker {
                    id: coverPicker
                    Layout.alignment: Qt.AlignHCenter
                }

                FormField {
                    id: titleField
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Book title")
                    placeholder: qsTr("Enter the book title")
                }

                FormField {
                    id: authorField
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Author")
                    placeholder: qsTr("Author name")
                }

                FormField {
                    id: yearField
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Year of publication")
                    placeholder: String(new Date().getFullYear())
                    numeric: true
                }

                FormField {
                    id: publisherField
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Publisher")
                    placeholder: qsTr("Publisher name")
                }

                FormField {
                    id: pagesField
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Number of pages")
                    placeholder: "0"
                    numeric: true
                }

                FormField {
                    id: isbnField
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("ISBN")
                    placeholder: qsTr("Optional — read from the PDF when it has one")
                }

                PdfPicker {
                    id: pdfPicker
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("PDF")
                    onParsed: info => root._applyPdf(info)
                    onCleared: coverPicker.previewUrl = ""
                }

                GenrePicker {
                    id: genrePicker
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Genre")
                }

                FormField {
                    id: descriptionField
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Description")
                    placeholder: qsTr("Short description...")
                    multiline: true
                }

                PrimaryButton {
                    id: submitButton
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    Layout.topMargin: Geometry.spacing.sm
                    Layout.bottomMargin: Geometry.spacing.xxl
                    label: qsTr("Add book")
                    onClicked: root._submit()
                }
            }
        }
    }
}
