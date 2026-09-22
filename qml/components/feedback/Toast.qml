import QtQuick
import QtQuick.Layouts
import Library

SurfaceCard {
    id: root

    property alias message: messageText.text
    property alias duration: hideTimer.interval

    property bool _showing: false

    function show(text) {
        root.message = text;
        _showing = true;
        hideTimer.restart();
    }

    function hide() {
        _showing = false;
    }

    radius: Geometry.radius.lg
    shadowOffset: Styles.elevation.heroOffset
    shadowBlur: Styles.elevation.heroBlur

    implicitHeight: layout.implicitHeight + 2 * Geometry.spacing.md

    visible: opacity > 0.01
    opacity: _showing ? 1 : 0
    Behavior on opacity {
        NumberAnimation {
            duration: Styles.duration.normal
        }
    }

    Timer {
        id: hideTimer
        interval: 3000
        onTriggered: root._showing = false
    }

    RowLayout {
        id: layout
        anchors {
            fill: parent
            leftMargin: Geometry.spacing.lg
            rightMargin: Geometry.spacing.lg
            topMargin: Geometry.spacing.md
            bottomMargin: Geometry.spacing.md
        }
        spacing: Geometry.spacing.md

        Rectangle {
            id: iconCircle
            Layout.preferredWidth: Geometry.size.iconLg
            Layout.preferredHeight: Geometry.size.iconLg
            Layout.alignment: Qt.AlignVCenter
            radius: width / 2
            color: Theme.primary

            Text {
                id: iconGlyph
                anchors.centerIn: parent
                text: "!"
                color: Theme.primaryContent
                font.pixelSize: Styles.fontSize.bodyLarge
                font.weight: Styles.fontWeight.bold
            }
        }

        Text {
            id: messageText
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.body
            wrapMode: Text.WordWrap
        }
    }
}
