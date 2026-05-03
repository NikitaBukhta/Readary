import QtQuick
import QtQuick.Effects
import Library

Rectangle {
    id: root

    property real shadowOffset: Styles.elevation.cardOffset
    property real shadowBlur: Styles.elevation.cardBlur

    radius: Geometry.radius.lg
    color: Theme.surface

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: Theme.shadow
        shadowHorizontalOffset: 0
        shadowVerticalOffset: root.shadowOffset
        shadowBlur: root.shadowBlur
    }
}
