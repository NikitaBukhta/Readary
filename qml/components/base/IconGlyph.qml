pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Library

Item {
    id: root

    property alias glyph: label.text
    property url source
    property int size: Geometry.size.iconMd
    property alias color: label.color
    // Emoji render as Twemoji SVGs, so `color` has no effect unless tinted.
    property alias tinted: image.layer.enabled

    readonly property url _emojiUrl: glyph.length > 0 ? EmojiResolver.iconUrl(glyph) : ""
    readonly property url _imageSource: _emojiUrl.toString().length > 0 ? _emojiUrl : root.source

    readonly property bool _empty: glyph.length === 0 && source.toString().length === 0

    implicitWidth: _empty ? 0 : size
    implicitHeight: _empty ? 0 : size

    Image {
        id: image
        anchors.fill: parent
        source: root._imageSource
        fillMode: Image.PreserveAspectFit
        visible: source.toString().length > 0
        sourceSize.width: root.size
        sourceSize.height: root.size

        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: root.color
        }
    }

    Label {
        id: label
        anchors.centerIn: parent
        color: Theme.textPrimary
        font.pixelSize: Math.round(root.size * 0.9)
        visible: !image.visible
    }
}
