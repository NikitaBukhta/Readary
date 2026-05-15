import QtQuick
import Library

Item {
    id: root

    property string glyph: ""
    property url source
    property int size: Geometry.size.iconMd
    property color color: Theme.textPrimary

    readonly property bool _empty: glyph.length === 0 && source.toString().length === 0

    implicitWidth: _empty ? 0 : size
    implicitHeight: _empty ? 0 : size

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
        // Per-OS emoji family — Qt won't auto-fallback once family is set.
        font.family: Qt.platform.os === "android" ? "Noto Color Emoji"
                   : Qt.platform.os === "osx" ? "Apple Color Emoji"
                   : "Segoe UI Emoji"
        visible: !image.visible
    }
}
