import QtQuick
import Library

Item {
    id: root

    property string glyph: ""
    property url source
    property int size: Geometry.size.iconMd
    property color color: Theme.textPrimary

    implicitWidth: size
    implicitHeight: size

    Image {
        id: image
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectFit
        visible: source.toString().length > 0
        sourceSize.width: root.size
        sourceSize.height: root.size
    }

    Text {
        id: label
        anchors.centerIn: parent
        text: root.glyph
        color: root.color
        font.pixelSize: Math.round(root.size * 0.9)
        font.family: "Segoe UI Emoji"
        visible: !image.visible
    }
}
