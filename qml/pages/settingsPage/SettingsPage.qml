pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    readonly property int _sidePadding: Geometry.spacing.xxl

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: root._sidePadding
            spacing: Geometry.spacing.md

            Text {
                id: titleText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Settings")
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleMedium
                font.weight: Styles.fontWeight.bold
                elide: Text.ElideRight
            }
        }

        SectionHeader {
            id: languageSection
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.xl
            title: qsTr("Language")
        }

        ColumnLayout {
            id: languageList
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.md
            spacing: Geometry.spacing.sm

            Repeater {
                id: languageRepeater
                model: SettingsController.languageModel.available

                delegate: PressableSurface {
                    id: row
                    required property int modelData

                    readonly property bool selected: SettingsController.languageModel.current === row.modelData

                    Layout.fillWidth: true
                    implicitHeight: Geometry.size.actionButtonHeight
                    radius: Geometry.radius.md
                    restColor: row.selected ? Theme.primarySoft : Theme.surface

                    onClicked: SettingsController.languageModel.current = row.modelData

                    RowLayout {
                        id: rowContent
                        anchors.fill: parent
                        anchors.leftMargin: Geometry.spacing.lg
                        anchors.rightMargin: Geometry.spacing.lg
                        spacing: Geometry.spacing.md

                        Text {
                            id: rowLabel
                            Layout.fillWidth: true
                            text: SettingsController.languageModel.label(row.modelData)
                            color: Theme.textPrimary
                            font.pixelSize: Styles.fontSize.bodyLarge
                            font.weight: row.selected ? Styles.fontWeight.semibold : Styles.fontWeight.regular
                            elide: Text.ElideRight
                        }

                        IconGlyph {
                            id: rowMark
                            visible: row.selected
                            glyph: "✓"
                            color: Theme.primary
                            size: Geometry.size.iconMd
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
