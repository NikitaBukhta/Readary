import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    // Any model exposing canLoadMore / canHide + loadMore() / hide().
    property var pagedModel

    readonly property bool _show: root.pagedModel && (root.pagedModel.canLoadMore || root.pagedModel.canHide)

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
            visible: root.pagedModel && root.pagedModel.canHide
            label: qsTr("Show less")
            labelColor: Theme.primary
            labelSize: Styles.fontSize.body
            labelWeight: Styles.fontWeight.semibold
            onClicked: root.pagedModel.hide()
        }

        TextButton {
            id: showMoreButton
            visible: root.pagedModel && root.pagedModel.canLoadMore
            label: qsTr("Show more")
            labelColor: Theme.primary
            labelSize: Styles.fontSize.body
            labelWeight: Styles.fontWeight.semibold
            onClicked: root.pagedModel.loadMore()
        }
    }
}
