import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import Library

PaddedCard {
    id: root

    property alias userRating: userTile.value
    property alias globalRating: globalTile.value
    property int total: 10

    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.lg

        RatingTile {
            id: userTile
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            label: qsTr("Your rating")
            total: root.total
            decimals: 0
            showStars: true
        }

        Rectangle {
            id: divider
            Layout.fillHeight: true
            Layout.preferredWidth: Math.max(1, Math.ceil(Screen.devicePixelRatio))
            color: Theme.divider
        }

        RatingTile {
            id: globalTile
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            label: qsTr("Global rating")
            total: root.total
            decimals: 1
            showStars: false
        }
    }
}
