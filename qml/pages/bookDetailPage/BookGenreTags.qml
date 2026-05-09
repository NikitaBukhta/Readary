pragma ComponentBehavior: Bound
import QtQuick
import Library

Flow {
    id: root

    property var tags: []

    visible: root.tags.length > 0
    spacing: Geometry.spacing.sm

    Repeater {
        id: tagsRepeater
        model: root.tags

        delegate: TagPill {
            id: tag
            required property string modelData
            label: modelData
        }
    }
}
