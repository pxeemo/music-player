pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick

// The landing page: every song in the library, in catalogue order. Tapping a
// row opens its now-playing page on top of this one.
//
// The StackView is passed in as `stackView` rather than reached through the
// StackView.view attached property, which is null for a declarative
// initialItem.
Item {
    id: page

    property var stackView

    function formatTime(seconds) {
        var s = Math.max(0, seconds);
        var m = Math.floor(s / 60);
        var r = Math.floor(s % 60);
        return m + ":" + (r < 10 ? "0" : "") + r;
    }

    Rectangle {
        anchors.fill: parent
        color: "#08080c"
    }

    Text {
        id: heading

        color: "white"
        font.bold: true
        font.pixelSize: 34
        text: "Songs"
        x: 48
        y: 44
    }

    Text {
        anchors.left: heading.left
        anchors.top: heading.bottom
        anchors.topMargin: 4
        color: "#7d7d88"
        font.pixelSize: 14
        text: SongsModel.count + (SongsModel.count === 1 ? " track" : " tracks")
    }

    ListView {
        id: list

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: heading.bottom
        anchors.topMargin: 44
        clip: true
        model: SongsModel.songs
        spacing: 6

        delegate: Item {
            id: row

            required property int index
            required property var modelData

            height: 92
            width: ListView.view.width

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 40
                anchors.rightMargin: 40
                color: rowMouse.containsMouse ? "#15151f" : "transparent"
                radius: 14
            }

            Artwork {
                id: cover

                anchors.left: parent.left
                anchors.leftMargin: 56
                anchors.verticalCenter: parent.verticalCenter
                height: 64
                song: row.modelData
                width: 64
            }

            Column {
                id: labels

                anchors.left: cover.right
                anchors.leftMargin: 18
                anchors.right: duration.left
                anchors.rightMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    color: "white"
                    elide: Text.ElideRight
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    text: row.modelData ? row.modelData.title : ""
                    width: parent.width
                }

                Text {
                    color: "#8b8b96"
                    elide: Text.ElideRight
                    font.pixelSize: 13
                    text: row.modelData ? row.modelData.artist : ""
                    width: parent.width
                }
            }

            Text {
                id: duration

                anchors.right: parent.right
                anchors.rightMargin: 56
                anchors.verticalCenter: parent.verticalCenter
                color: "#6a6a76"
                font.family: "monospace"
                font.pixelSize: 13
                text: row.modelData ? page.formatTime(row.modelData.duration) : ""
            }

            MouseArea {
                id: rowMouse

                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

                onClicked: page.stackView.push(Qt.resolvedUrl("NowPlayingPage.qml"), {
                    "song": row.modelData,
                    "stackView": page.stackView
                })
            }
        }
    }
}
