import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property string label: ""
    property string iconGlyph: ""
    property url iconSource
    property bool active: false

    signal clicked()

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
            Layout.alignment: Qt.AlignHCenter
            glyph: root.iconGlyph
            source: root.iconSource
            color: root._contentColor
            size: Geometry.size.bottomNavIcon
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.label
            color: root._contentColor
            font.pixelSize: Styles.fontSize.caption
            font.weight: root.active ? Styles.fontWeight.semibold
                                     : Styles.fontWeight.medium
        }
    }
}
