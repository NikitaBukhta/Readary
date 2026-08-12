import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

RowLayout {
    id: root

    property int minValue: 0
    property int maxValue: 0
    property string minPlaceholder: qsTr("Any")
    property string maxPlaceholder: qsTr("Any")

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
        placeholder: root.minPlaceholder
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
        placeholder: root.maxPlaceholder
        onEdited: value => root.maxEdited(value)
    }
}
