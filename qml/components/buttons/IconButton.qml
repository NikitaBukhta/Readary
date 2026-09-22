import QtQuick
import Library

PressableSurface {
    id: root

    property alias iconGlyph: glyph.glyph
    property alias iconSource: glyph.source
    property alias iconColor: glyph.color
    property alias iconTinted: glyph.tinted
    property int diameter: Geometry.size.avatarSm

    implicitWidth: diameter
    implicitHeight: diameter
    radius: width / 2
    restColor: Theme.primary
    pressedColor: restColor // press feedback via opacity, not bg tint
    shadowOffset: Styles.elevation.subtleOffset
    shadowBlur: Styles.elevation.subtleBlur
    opacity: root.pressed ? Styles.opacity.pressed : 1.0

    IconGlyph {
        id: glyph
        anchors.centerIn: parent
        color: Theme.primaryContent
        size: Geometry.size.iconMd
    }
}
