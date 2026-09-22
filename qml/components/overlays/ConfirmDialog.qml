import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Popup {
    id: root

    property alias title: titleText.text
    property alias message: messageText.text
    property alias confirmLabel: confirmButton.label
    property alias cancelLabel: cancelButton.label

    signal confirmed
    signal cancelled

    modal: true
    dim: true
    focus: true
    anchors.centerIn: Overlay.overlay
    padding: Geometry.spacing.xl
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min((parent ? parent.width : Geometry.window.defaultWidth) - 2 * Geometry.spacing.xxl, Geometry.window.contentMaxWidth)

    background: SurfaceCard {
        shadowOffset: Styles.elevation.heroOffset
        shadowBlur: Styles.elevation.heroBlur
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.45)
    }

    contentItem: ColumnLayout {
        spacing: Geometry.spacing.lg

        Text {
            id: titleText
            Layout.fillWidth: true
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.title
            font.weight: Styles.fontWeight.bold
            wrapMode: Text.WordWrap
        }

        Text {
            id: messageText
            Layout.fillWidth: true
            color: Theme.textSecondary
            font.pixelSize: Styles.fontSize.body
            wrapMode: Text.WordWrap
            lineHeight: 1.35
            lineHeightMode: Text.ProportionalHeight
        }

        RowLayout {
            id: actions
            Layout.fillWidth: true
            Layout.topMargin: Geometry.spacing.sm
            spacing: Geometry.spacing.md

            SecondaryButton {
                id: cancelButton
                Layout.fillWidth: true
                label: qsTr("Cancel")
                onClicked: {
                    root.close();
                    root.cancelled();
                }
            }

            PrimaryButton {
                id: confirmButton
                Layout.fillWidth: true
                label: qsTr("Confirm")
                onClicked: {
                    root.close();
                    root.confirmed();
                }
            }
        }
    }
}
