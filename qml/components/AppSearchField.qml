import QtQuick
import QtQuick.Controls
import Library

FocusScope {
    id: root

    property alias text: input.text
    property string placeholderText: qsTr("Search...")

    signal textEdited(string text)
    signal accepted(string text)

    // Android only: dropping focus commits the input method's pre-edit (the last
    // word typed can still be uncommitted, leaving `input.text` stale) and
    // closes the virtual keyboard. On desktop it would only cost the caret.
    function submit(): void {
        if (Qt.platform.os === "android") {
            input.focus = false;
        }
        root.accepted(input.text);
    }

    implicitHeight: Geometry.size.searchHeight
    implicitWidth: 280

    Rectangle {
        id: pill
        anchors.fill: parent
        radius: Geometry.radius.pill
        color: Theme.searchBackground
        border.width: 0
    }

    TouchTarget {
        id: searchButton
        anchors.left: parent.left
        anchors.leftMargin: Geometry.spacing.sm
        anchors.verticalCenter: parent.verticalCenter
        onClicked: root.submit()

        IconGlyph {
            id: icon
            size: Geometry.size.iconMd
            glyph: "🔍"
            color: Theme.textMuted
        }
    }

    TextField {
        id: input
        anchors.left: searchButton.right
        anchors.leftMargin: Geometry.spacing.xs
        anchors.right: parent.right
        anchors.rightMargin: Geometry.spacing.lg
        anchors.verticalCenter: parent.verticalCenter

        placeholderText: root.placeholderText
        placeholderTextColor: Theme.textMuted
        color: Theme.textPrimary
        font.pixelSize: Styles.fontSize.bodyLarge
        selectByMouse: true
        background: null
        verticalAlignment: TextInput.AlignVCenter

        // The Android IME's action key is a plain newline the field swallows
        // unless hinted, and predictive text holds Enter inside the composing
        // region.
        EnterKey.type: Qt.EnterKeySearch
        inputMethodHints: Qt.ImhNoPredictiveText

        onTextEdited: root.textEdited(text)
        onAccepted: root.submit()

        Keys.onReturnPressed: event => {
            root.submit();
            event.accepted = true;
        }
        Keys.onEnterPressed: event => {
            root.submit();
            event.accepted = true;
        }
    }
}
