pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Popup {
    id: root

    property alias actions: repeater.model

    signal triggered(string actionId)

    modal: true
    dim: false
    focus: true
    padding: Geometry.spacing.sm
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    implicitWidth: Geometry.size.menuWidth

    background: SurfaceCard {
        radius: Geometry.radius.md
        shadowOffset: Styles.elevation.heroOffset
        shadowBlur: Styles.elevation.heroBlur
    }

    contentItem: ColumnLayout {
        id: items
        spacing: 0

        Repeater {
            id: repeater
            model: []

            delegate: ColumnLayout {
                id: row

                required property var modelData

                Layout.fillWidth: true
                spacing: 0

                Rectangle {
                    id: separator
                    Layout.fillWidth: true
                    Layout.topMargin: Geometry.spacing.xs
                    Layout.bottomMargin: Geometry.spacing.xs
                    // Groups the destructive actions off from the rest without
                    // needing a colour the palettes do not have.
                    visible: row.modelData.separatorBefore ?? false
                    implicitHeight: Geometry.size.borderWidth
                    color: Theme.divider
                }

                PressableSurface {
                    id: item
                    Layout.fillWidth: true
                    implicitHeight: Geometry.size.menuItemHeight
                    radius: Geometry.radius.sm
                    restColor: Theme.surface
                    pressedColor: Theme.primarySoft
                    // Flat: the menu's own card already carries the elevation.
                    shadowOffset: 0
                    shadowBlur: 0
                    onClicked: {
                        root.close();
                        root.triggered(row.modelData.id);
                    }

                    RowLayout {
                        id: content
                        anchors.fill: parent
                        anchors.leftMargin: Geometry.spacing.md
                        anchors.rightMargin: Geometry.spacing.md
                        spacing: Geometry.spacing.sm

                        IconGlyph {
                            id: glyph
                            glyph: row.modelData.glyph ?? ""
                            color: Theme.textPrimary
                            size: Geometry.size.iconMd
                        }

                        Text {
                            id: labelText
                            Layout.fillWidth: true
                            text: row.modelData.label ?? ""
                            color: Theme.textPrimary
                            font.pixelSize: Styles.fontSize.body
                            font.weight: Styles.fontWeight.medium
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
