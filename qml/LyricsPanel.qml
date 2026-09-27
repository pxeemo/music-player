pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick

// The collapsible lyrics half of the now-playing screen.
//
// When expanded it shows the full lyric sheet with a slim handle on its right
// edge; clicking the handle collapses the panel down to just that handle and
// the artwork next to it fills the freed space. The width is animated here so
// the host page can simply lay it out in a Row and let it repartition.
Item {
    id: root

    property Song song
    property bool expanded: true
    property int activeLine: -1
    property real expandedWidth: 460

    readonly property real handleWidth: 46

    Behavior on width {
        NumberAnimation {
            duration: 320
            easing.type: Easing.InOutCubic
        }
    }

    clip: true
    width: expanded ? expandedWidth : handleWidth

    // ---- lyrics ----------------------------------------------------------
    Item {
        id: content

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.top: parent.top
        clip: true
        opacity: root.expanded ? 1 : 0
        width: Math.max(0, root.width - root.handleWidth)

        Behavior on opacity {
            NumberAnimation {
                duration: 200
            }
        }

        LyricsView {
            anchors.fill: parent
            activeLine: root.activeLine
            song: root.song
        }
    }

    // ---- collapse handle (always visible) --------------------------------
    Rectangle {
        id: handle

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.top: parent.top
        color: handleMouse.containsMouse ? "#1c1c28" : "#12121a"
        radius: 12
        width: root.handleWidth

        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#7d7d88"
                font.pixelSize: 15
                text: "\u266A"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#9b9ba4"
                font.pixelSize: 16
                text: root.expanded ? "\u276F" : "\u276E"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#4e4e58"
                font.pixelSize: 10
                text: "LYRICS"
            }
        }

        MouseArea {
            id: handleMouse

            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true

            onClicked: root.expanded = !root.expanded
        }
    }
}
