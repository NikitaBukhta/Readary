import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property alias label: caption.text
    property alias placeholder: field.placeholderText
    property bool multiline: false
    property bool numeric: false

    readonly property string value: root.multiline ? area.text : field.text

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
