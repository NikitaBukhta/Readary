pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Popup {
    id: root

    property bool showLibraryOnlyCriteria: true

    modal: true
    dim: true
    focus: true
    anchors.centerIn: Overlay.overlay
    padding: Geometry.spacing.xl
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min((parent ? parent.width : Geometry.window.defaultWidth) - 2 * Geometry.spacing.xxl, Geometry.window.contentMaxWidth)
    height: Math.min((parent ? parent.height : Geometry.window.defaultHeight) * 0.85, 720)

    onAboutToShow: BookFilterController.syncDraft()

    background: SurfaceCard {
        shadowOffset: Styles.elevation.heroOffset
        shadowBlur: Styles.elevation.heroBlur
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.45)
    }

    component SectionLabel: Text {
        color: Theme.textPrimary
        font.pixelSize: Styles.fontSize.body
        font.weight: Styles.fontWeight.semibold
        Layout.fillWidth: true
        Layout.topMargin: Geometry.spacing.sm
    }

    component TextCriterion: Rectangle {
        id: textBox

        property alias text: field.text
        property string placeholder: ""

        signal edited(string value)

        Layout.fillWidth: true
        implicitHeight: Geometry.size.pillButtonHeight
        radius: Geometry.radius.md
        color: Theme.searchBackground

        TextField {
            id: field
            anchors.fill: parent
            anchors.leftMargin: Geometry.spacing.md
            anchors.rightMargin: Geometry.spacing.md
            placeholderText: textBox.placeholder
            placeholderTextColor: Theme.textMuted
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.body
            background: null
            verticalAlignment: TextInput.AlignVCenter
            selectByMouse: true
            onTextEdited: textBox.edited(text)
        }
    }

    contentItem: ColumnLayout {
        id: content
        spacing: Geometry.spacing.md

        RowLayout {
            id: header
            Layout.fillWidth: true

            Text {
                id: titleText
                Layout.fillWidth: true
                text: qsTr("Filters")
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.title
                font.weight: Styles.fontWeight.bold
            }

            Text {
                id: countText
                visible: BookFilterController.draftCount > 0
                text: qsTr("%n active", "", BookFilterController.draftCount)
                color: Theme.primary
                font.pixelSize: Styles.fontSize.bodySmall
                font.weight: Styles.fontWeight.semibold
            }
        }

        Flickable {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: criteria.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            ColumnLayout {
                id: criteria
                width: scroll.width
                spacing: Geometry.spacing.sm

                SectionLabel {
                    id: languageLabel
                    text: qsTr("Language")
                    visible: languageFlow.visible
                }

                Flow {
                    id: languageFlow
                    Layout.fillWidth: true
                    spacing: Geometry.spacing.sm
                    visible: BookFilterController.availableLanguages.length > 0

                    Repeater {
                        id: languageRepeater
                        model: BookFilterController.availableLanguages
                        delegate: FilterChip {
                            required property string modelData
                            label: BookFilterController.languageLabel(modelData)
                            selected: BookFilterController.selectedLanguages.includes(modelData)
                            onClicked: BookFilterController.toggleLanguage(modelData)
                        }
                    }
                }

                SectionLabel {
                    id: genreLabel
                    text: qsTr("Genre")
                    visible: genreFlow.visible
                }

                Flow {
                    id: genreFlow
                    Layout.fillWidth: true
                    spacing: Geometry.spacing.sm
                    visible: BookFilterController.availableGenres.length > 0

                    Repeater {
                        id: genreRepeater
                        model: BookFilterController.availableGenres
                        delegate: FilterChip {
                            required property string modelData
                            label: modelData
                            selected: BookFilterController.selectedGenres.includes(modelData)
                            onClicked: BookFilterController.toggleGenre(modelData)
                        }
                    }
                }

                SectionLabel {
                    id: authorLabel
                    text: qsTr("Author")
                }

                TextCriterion {
                    id: authorField
                    text: BookFilterController.author
                    placeholder: qsTr("Any author")
                    onEdited: value => BookFilterController.author = value
                }

                SectionLabel {
                    id: publisherLabel
                    text: qsTr("Publisher")
                }

                TextCriterion {
                    id: publisherField
                    text: BookFilterController.publisher
                    placeholder: qsTr("Any publisher")
                    onEdited: value => BookFilterController.publisher = value
                }

                SectionLabel {
                    id: pagesLabel
                    text: qsTr("Pages")
                }

                RangeField {
                    id: pagesRange
                    Layout.fillWidth: true
                    minValue: BookFilterController.minPages
                    maxValue: BookFilterController.maxPages
                    onMinEdited: value => BookFilterController.minPages = value
                    onMaxEdited: value => BookFilterController.maxPages = value
                }

                SectionLabel {
                    id: yearLabel
                    text: qsTr("Year")
                }

                RangeField {
                    id: yearRange
                    Layout.fillWidth: true
                    minValue: BookFilterController.minYear
                    maxValue: BookFilterController.maxYear
                    onMinEdited: value => BookFilterController.minYear = value
                    onMaxEdited: value => BookFilterController.maxYear = value
                }

                SectionLabel {
                    id: ratingLabel
                    text: qsTr("Minimum rating")
                }

                Flow {
                    id: ratingFlow
                    Layout.fillWidth: true
                    spacing: Geometry.spacing.sm

                    Repeater {
                        id: ratingRepeater
                        model: [0, 3, 4, 4.5]
                        delegate: FilterChip {
                            required property var modelData
                            label: modelData === 0 ? qsTr("Any") : "★ " + modelData
                            selected: BookFilterController.minRating === modelData
                            onClicked: BookFilterController.minRating = modelData
                        }
                    }
                }

                SectionLabel {
                    id: typeLabel
                    text: qsTr("Edition")
                    visible: typeFlow.visible
                }

                Flow {
                    id: typeFlow
                    Layout.fillWidth: true
                    spacing: Geometry.spacing.sm
                    visible: root.showLibraryOnlyCriteria && BookFilterController.availableTypes.length > 0

                    Repeater {
                        id: typeRepeater
                        model: BookFilterController.availableTypes
                        delegate: FilterChip {
                            required property string modelData
                            label: modelData
                            selected: BookFilterController.selectedTypes.includes(modelData)
                            onClicked: BookFilterController.toggleType(modelData)
                        }
                    }
                }
            }
        }

        RowLayout {
            id: actions
            Layout.fillWidth: true
            Layout.topMargin: Geometry.spacing.sm
            spacing: Geometry.spacing.md

            TextButton {
                id: resetButton
                label: qsTr("Reset")
                onClicked: {
                    BookFilterController.reset();
                    root.close();
                }
            }

            Item {
                id: spacer
                Layout.fillWidth: true
            }

            PrimaryButton {
                id: applyButton
                Layout.preferredWidth: Geometry.size.dialogPrimaryWidth
                label: qsTr("Apply")
                onClicked: {
                    BookFilterController.apply();
                    root.close();
                }
            }
        }
    }
}
