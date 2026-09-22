import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property alias label: labelText.text
    property alias iconGlyph: icon.glyph
    property alias iconSource: icon.source
    property bool active: false

    signal clicked

    implicitHeight: Geometry.size.bottomNavHeight

    readonly property color _contentColor: active ? Theme.primary : Theme.textMuted

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 2

        IconGlyph {
            id: icon
            Layout.alignment: Qt.AlignHCenter
            color: root._contentColor
            size: Geometry.size.bottomNavIcon
        }

        Text {
            id: labelText
            Layout.alignment: Qt.AlignHCenter
            color: root._contentColor
            font.pixelSize: Styles.fontSize.caption
            font.weight: root.active ? Styles.fontWeight.semibold : Styles.fontWeight.medium
        }
    }
}
