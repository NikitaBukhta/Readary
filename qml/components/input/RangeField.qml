import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

RowLayout {
    id: root

    property int minValue: 0
    property int maxValue: 0
    property alias minPlaceholder: minBox.placeholder
    property alias maxPlaceholder: maxBox.placeholder

    signal minEdited(int value)
    signal maxEdited(int value)

    spacing: Geometry.spacing.md

    component NumberBox: Rectangle {
        id: box

        property alias text: field.text
        property string placeholder: ""

        signal edited(int value)

        Layout.fillWidth: true
        implicitHeight: Geometry.size.pillButtonHeight
        radius: Geometry.radius.md
        color: Theme.searchBackground

        TextField {
            id: field
            anchors.fill: parent
            anchors.leftMargin: Geometry.spacing.md
            anchors.rightMargin: Geometry.spacing.md
            placeholderText: box.placeholder
            placeholderTextColor: Theme.textMuted
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.body
            background: null
            verticalAlignment: TextInput.AlignVCenter
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 0
                top: 99999
            }
            selectByMouse: true

            onTextEdited: box.edited(text.length > 0 ? parseInt(text) : 0)
        }
    }

    NumberBox {
        id: minBox
        text: root.minValue > 0 ? String(root.minValue) : ""
        placeholder: qsTr("Any")
        onEdited: value => root.minEdited(value)
    }

    Text {
        id: separator
        text: "–"
        color: Theme.textMuted
        font.pixelSize: Styles.fontSize.body
        Layout.alignment: Qt.AlignVCenter
    }

    NumberBox {
        id: maxBox
        text: root.maxValue > 0 ? String(root.maxValue) : ""
        placeholder: qsTr("Any")
        onEdited: value => root.maxEdited(value)
    }
}
