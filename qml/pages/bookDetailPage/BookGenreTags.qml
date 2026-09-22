pragma ComponentBehavior: Bound
import QtQuick
import Library

Flow {
    id: root

    property alias tags: tagsRepeater.model

    visible: root.tags.length > 0
    spacing: Geometry.spacing.sm

    Repeater {
        id: tagsRepeater
        model: []

        delegate: TagPill {
            id: tag
            required property string modelData
            label: modelData
        }
    }
}
