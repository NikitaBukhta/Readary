import QtQuick
import Library

SurfaceCard {
    id: root

    property real contentPadding: Geometry.spacing.xl

    // First child drives the card's implicit size. Safe with `anchors.fill: parent`
    // because Layout.implicitHeight is computed from children, not from Layout.height.
    readonly property var _content: contentRoot.children.length > 0 ? contentRoot.children[0] : null

    implicitWidth: (_content ? _content.implicitWidth : 0) + 2 * root.contentPadding
    implicitHeight: (_content ? _content.implicitHeight : 0) + 2 * root.contentPadding

    Item {
        id: contentRoot
        anchors.fill: parent
        anchors.margins: root.contentPadding
    }

    default property alias content: contentRoot.data
}
