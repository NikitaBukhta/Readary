import QtQuick
import Library

Item {
    id: root

    property var characterData: ({})
    property int sidePadding: 0

    signal openRequested(int characterId)

    implicitHeight: row.implicitHeight

    CharacterRow {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.sidePadding
        anchors.rightMargin: root.sidePadding

        name: root.characterData.name ?? ""
        role: root.characterData.role ?? ""
        onClicked: root.openRequested(root.characterData.id ?? 0)
    }
}
