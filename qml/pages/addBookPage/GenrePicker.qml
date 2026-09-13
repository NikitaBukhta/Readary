pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

// Preset genres plus a free-text slot behind "Other". Chip labels are translated
// for display while the stored id stays English: rows in the `genres` table are
// language-independent keys shared with imported books.
ColumnLayout {
    id: root

    property string label: ""

    readonly property var value: {
        const typed = otherField.value.trim();
        return root._otherOpen && typed.length > 0 ? root._selected.concat([typed]) : root._selected;
    }

    readonly property var _presets: [
        {
            id: "Fantasy",
            label: qsTr("Fantasy")
        },
        {
            id: "Detective",
            label: qsTr("Detective")
        },
        {
            id: "Romance",
            label: qsTr("Romance")
        },
        {
            id: "Sci-Fi",
            label: qsTr("Sci-Fi")
        },
        {
            id: "Prose",
            label: qsTr("Prose")
        }
    ]

    property var _selected: []
    property bool _otherOpen: false

    // Reassigns the whole list rather than mutating it — a var property does not
    // notify on in-place changes, so `value` would go stale.
    function _toggle(genreId: string): void {
        const next = root._selected.slice();
        const at = next.indexOf(genreId);
        if (at === -1)
            next.push(genreId);
        else
            next.splice(at, 1);
        root._selected = next;
    }

    spacing: Geometry.spacing.xs

    FieldLabel {
        id: caption
        Layout.fillWidth: true
        text: root.label
    }

    Flow {
        id: chipFlow
        Layout.fillWidth: true
        spacing: Geometry.spacing.sm

        Repeater {
            id: presetRepeater
            model: root._presets

            delegate: FilterChip {
                required property var modelData

                label: modelData.label
                selected: root._selected.includes(modelData.id)
                onClicked: root._toggle(modelData.id)
            }
        }

        FilterChip {
            id: otherChip
            label: qsTr("Other")
            selected: root._otherOpen
            onClicked: root._otherOpen = !root._otherOpen
        }
    }

    FormField {
        id: otherField
        Layout.fillWidth: true
        Layout.topMargin: Geometry.spacing.xs
        visible: root._otherOpen
        placeholder: qsTr("Genre name")
    }
}
