import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property var charactersModel

    readonly property bool _show: root.charactersModel && (root.charactersModel.canLoadMore || root.charactersModel.canHide)

    visible: root._show
    implicitHeight: root._show ? row.implicitHeight : 0

    RowLayout {
        id: row
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Geometry.spacing.sm
        spacing: Geometry.spacing.lg

        TextButton {
            id: showLessButton
            visible: root.charactersModel && root.charactersModel.canHide
            label: qsTr("Show less")
            labelColor: Theme.primary
            labelSize: Styles.fontSize.body
            labelWeight: Styles.fontWeight.semibold
            onClicked: root.charactersModel.hide()
        }

        TextButton {
            id: showMoreButton
            visible: root.charactersModel && root.charactersModel.canLoadMore
            label: qsTr("Show more")
            labelColor: Theme.primary
            labelSize: Styles.fontSize.body
            labelWeight: Styles.fontWeight.semibold
            onClicked: root.charactersModel.loadMore()
        }
    }
}
