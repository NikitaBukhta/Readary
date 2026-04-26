import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import Library

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property string iconGlyph: ""
    property url iconSource
    property color iconBackground: Theme.primarySoft
    property color iconColor: Theme.primary

    signal clicked()

    implicitHeight: Geometry.size.categoryRowHeight
    radius: Geometry.radius.lg
    color: Theme.surface

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: Theme.shadow
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 3
        shadowBlur: 0.5
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Geometry.spacing.lg
        anchors.rightMargin: Geometry.spacing.lg
        spacing: Geometry.spacing.lg

        Rectangle {
            id: iconBox
            Layout.preferredWidth: Geometry.size.categoryIconBox
            Layout.preferredHeight: Geometry.size.categoryIconBox
            radius: Geometry.radius.md
            color: root.iconBackground

            IconGlyph {
                anchors.centerIn: parent
                glyph: root.iconGlyph
                source: root.iconSource
                color: root.iconColor
                size: Geometry.size.iconLg
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.bodyLarge
                font.weight: Styles.fontWeight.semibold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.subtitle
                color: Theme.textSecondary
                font.pixelSize: Styles.fontSize.small
                elide: Text.ElideRight
                visible: text.length > 0
            }
        }

        Text {
            text: "›"
            color: Theme.textMuted
            font.pixelSize: Styles.fontSize.titleMedium
            font.weight: Styles.fontWeight.bold
        }
    }
}
