pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

Rectangle {
    id: root

    readonly property var _navItems: [
        {
            id: NavigationController.MAIN_PAGE,
            label: qsTr("Library"),
            glyph: "📚",
            source: ""
        },
        {
            id: NavigationController.SEARCH_PAGE,
            label: qsTr("Search"),
            glyph: "🔍",
            source: ""
        },
        {
            id: NavigationController.GOALS_PAGE,
            label: qsTr("Goals"),
            glyph: "🎯",
            source: ""
        },
        {
            id: NavigationController.CHALLENGES_PAGE,
            label: qsTr("Challenges"),
            glyph: "🏆",
            source: ""
        },
        {
            id: NavigationController.PROFILE_PAGE,
            label: qsTr("Profile"),
            glyph: "👤",
            source: ""
        }
    ]
    property int currentId: NavigationController.currentPage

    implicitHeight: Geometry.size.bottomNavHeight
    color: Theme.surface

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: Theme.divider
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            model: root._navItems

            delegate: BottomNavItem {
                required property var modelData
                Layout.fillWidth: true
                Layout.fillHeight: true
                label: modelData.label ?? ""
                iconGlyph: modelData.glyph ?? ""
                iconSource: modelData.source ?? ""
                active: modelData.id === root.currentId
                onClicked: NavigationController.currentPage = modelData.id
            }
        }
    }
}
