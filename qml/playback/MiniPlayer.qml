pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick

// A compact playback bar shown under the songs list. Tapping it opens the
// now-playing screen; its transport buttons work in place.
Rectangle {
    id: root

    property var stackView
    property Song song: Player.currentSong

    color: "#0f0f16"

    function openNowPlaying() {
        if (root.stackView && root.stackView.depth === 1)
            root.stackView.push(Qt.resolvedUrl("../views/NowPlayingPage.qml"), {
                "stackView": root.stackView
            });
    }

    // ---- progress line ----
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        color: "#20202c"
        height: 2
        width: parent.width
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        color: "white"
        height: 2
        width: parent.width * (Player.duration > 0 ? Math.min(1, Player.position / Player.duration) : 0)
    }

    // ---- whole bar opens now playing ----
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.openNowPlaying()
    }

    Artwork {
        id: cover

        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        height: 48
        song: root.song
        width: 48
    }

    Row {
        id: controls

        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 24

        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: "#cfcfd8"
            font.pixelSize: 15
            text: Player.playing ? "Pause" : "Play"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Player.toggle()
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: "#cfcfd8"
            font.pixelSize: 15
            text: "Next"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Player.next()
            }
        }
    }

    Column {
        anchors.left: cover.right
        anchors.leftMargin: 14
        anchors.right: controls.left
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Text {
            color: "white"
            elide: Text.ElideRight
            font.pixelSize: 15
            font.weight: Font.DemiBold
            text: root.song ? root.song.title : ""
            width: parent.width
        }

        Text {
            color: "#8b8b96"
            elide: Text.ElideRight
            font.pixelSize: 12
            text: root.song ? root.song.artist : ""
            width: parent.width
        }
    }
}
