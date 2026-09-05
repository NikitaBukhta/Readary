import QtQuick
import Library

PressableSurface {
    id: root

    property string iconGlyph: ""
    property url iconSource
    property color iconColor: Theme.primaryContent
    property bool iconTinted: false
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
        glyph: root.iconGlyph
        source: root.iconSource
        color: root.iconColor
        tinted: root.iconTinted
        size: Geometry.size.iconMd
    }
}
