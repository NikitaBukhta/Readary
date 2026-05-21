import QtQuick
import QtQuick.Controls
import Library

Item {
    id: root

    property string glyph: ""
    property url source
    property int size: Geometry.size.iconMd
    property color color: Theme.textPrimary

    // Emoji codepoints are rendered as SVGs from the vendored Twemoji set so
    // they look identical on every platform (Qt's text engine on Android
    // can't render bundled color-emoji fonts — see EmojiResolver). Non-emoji
    // symbols (✓ ← ↻) fall through to the Label below, which uses the
    // platform default font.
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
    }

    Label {
        id: label
        anchors.centerIn: parent
        text: root.glyph
        color: root.color
        font.pixelSize: Math.round(root.size * 0.9)
        visible: !image.visible
    }
}
