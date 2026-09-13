import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

// Labelled input: a caption above a filled, rounded box. One-way by design —
// the hosting form reads `value` when it submits rather than binding both ways.
ColumnLayout {
    id: root

    property string label: ""
    property string placeholder: ""
    // Grows the box and swaps the single-line field for a scrollable area.
    property bool multiline: false
    property bool numeric: false

    readonly property string value: root.multiline ? area.text : field.text

    // Escape hatch for a field the form fills in for the user (a PDF's page
    // count overwriting what was typed). Reading still goes through `value`.
    function setValue(text: string): void {
        if (root.multiline)
            area.text = text;
        else
            field.text = text;
    }

    spacing: Geometry.spacing.xs

    FieldLabel {
        id: caption
        Layout.fillWidth: true
        visible: root.label.length > 0
        text: root.label
    }

    Rectangle {
        id: box
        Layout.fillWidth: true
        implicitHeight: root.multiline ? Geometry.size.formTextAreaHeight : Geometry.size.pillButtonHeight
        radius: Geometry.radius.md
        color: Theme.searchBackground

        IntValidator {
            id: digits
            bottom: 0
            top: 99999
        }

        TextField {
            id: field
            visible: !root.multiline
            anchors.fill: parent
            anchors.leftMargin: Geometry.spacing.md
            anchors.rightMargin: Geometry.spacing.md
            placeholderText: root.placeholder
            placeholderTextColor: Theme.textMuted
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.body
            background: null
            verticalAlignment: TextInput.AlignVCenter
            selectByMouse: true
            inputMethodHints: root.numeric ? Qt.ImhDigitsOnly : Qt.ImhNone
            validator: root.numeric ? digits : null
        }

        ScrollView {
            id: areaScroll
            visible: root.multiline
            anchors.fill: parent
            anchors.margins: Geometry.spacing.sm
            clip: true

            TextArea {
                id: area
                placeholderText: root.placeholder
                placeholderTextColor: Theme.textMuted
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.body
                background: null
                wrapMode: TextEdit.Wrap
                selectByMouse: true
            }
        }
    }
}
