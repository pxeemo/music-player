pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Controls

// The play queue. Shows the ordered list the Player will walk through, with the
// current track highlighted; tapping a row jumps straight to it.
Item {
    id: root

    signal songChosen(int index)

    function formatTime(seconds) {
        var s = Math.max(0, seconds);
        var m = Math.floor(s / 60);
        var r = Math.floor(s % 60);
        return m + ":" + (r < 10 ? "0" : "") + r;
    }

    Text {
        id: heading

        anchors.left: parent.left
        anchors.top: parent.top
        color: "#7d7d88"
        font.pixelSize: 13
        text: "Queue · " + Player.queueCount
    }

    ListView {
        id: list

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: heading.bottom
        anchors.topMargin: 10
        clip: true
        model: Player.queue
        spacing: 2

        delegate: Item {
            id: row

            required property int index
            required property var modelData

            height: 54
            width: ListView.view.width

            Rectangle {
                anchors.fill: parent
                color: row.index === Player.currentIndex ? "#20202c" : (rowMouse.containsMouse ? "#15151f" : "transparent")
                radius: 10
            }

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    color: row.index === Player.currentIndex ? "white" : "#c9c9d2"
                    elide: Text.ElideRight
                    font.pixelSize: 14
                    text: row.modelData ? row.modelData.title : ""
                    width: parent.width
                }

                Text {
                    color: "#7d7d88"
                    elide: Text.ElideRight
                    font.pixelSize: 12
                    text: row.modelData ? row.modelData.artist : ""
                    width: parent.width
                }
            }

            MouseArea {
                id: rowMouse

                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

                onClicked: root.songChosen(row.index)
            }
        }
    }
}
