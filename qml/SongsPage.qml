pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Controls

// The landing page: every track MusicLibrary found, in the order it scanned
// them. Tapping a row starts a library queue at that track and opens the
// now-playing screen.
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
        id: subtitle

        anchors.left: heading.left
        anchors.top: heading.bottom
        anchors.topMargin: 4
        color: "#7d7d88"
        font.pixelSize: 14
        text: MusicLibrary.scanning ? ("Scanning " + MusicLibrary.scanned + " / " + MusicLibrary.total) : (MusicLibrary.count + (MusicLibrary.count === 1 ? " track" : " tracks"))
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 48
        anchors.top: heading.top
        color: "#5c5c68"
        elide: Text.ElideLeft
        font.pixelSize: 12
        text: MusicLibrary.folder
        width: parent.width * 0.5
        horizontalAlignment: Text.AlignRight
    }

    ListView {
        id: list

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: heading.bottom
        anchors.topMargin: 44
        clip: true
        model: MusicLibrary.songs
        spacing: 6

        delegate: Item {
            id: row

            required property int index
            required property var modelData

            height: 76
            width: ListView.view.width

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 40
                anchors.rightMargin: 40
                color: row.index === Player.currentIndex && Player.currentSong ? "#15151f" : (rowMouse.containsMouse ? "#15151f" : "transparent")
                radius: 14
            }

            Artwork {
                id: cover

                anchors.left: parent.left
                anchors.leftMargin: 56
                anchors.verticalCenter: parent.verticalCenter
                height: 52
                song: row.modelData
                width: 52
            }

            Column {
                anchors.left: cover.right
                anchors.leftMargin: 18
                anchors.right: duration.left
                anchors.rightMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3

                Text {
                    color: "white"
                    elide: Text.ElideRight
                    font.pixelSize: 17
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

                onClicked: {
                    Player.playFromLibrary(row.index);
                    page.stackView.push(Qt.resolvedUrl("NowPlayingPage.qml"), {
                        "stackView": page.stackView
                    });
                }
            }
        }
    }
}
