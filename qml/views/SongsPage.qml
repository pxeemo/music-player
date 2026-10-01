pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Controls

// The landing page: every track MusicLibrary found, as a filtered/sorted view.
// Tapping a row starts a queue at that track and opens the now-playing screen.
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
        horizontalAlignment: Text.AlignRight
        text: MusicLibrary.folder
        width: parent.width * 0.5
    }

    // ---- sort + search ---------------------------------------------------
    Row {
        id: controls

        anchors.left: parent.left
        anchors.leftMargin: 48
        anchors.right: parent.right
        anchors.rightMargin: 48
        anchors.top: subtitle.bottom
        anchors.topMargin: 22
        spacing: 12

        ComboBox {
            id: sortBox

            model: ["Modified date", "Modified date (reverse)", "Add date", "Add date (reverse)", "Title", "Title (reverse)", "Artist", "Artist (reverse)", "Album", "Album (reverse)"]
            width: 210

            // Both directions: show the current order, and apply taps.
            Binding {
                property: "currentIndex"
                target: sortBox
                value: MusicLibrary.sortOrder
            }

            onActivated: MusicLibrary.sortOrder = currentIndex

            palette.base: "#15151f"
            palette.button: "#15151f"
            palette.buttonText: "#d6d6de"
            palette.highlight: "#2a2a3a"
            palette.highlightedText: "white"
            palette.text: "#d6d6de"
            palette.window: "#15151f"
            palette.windowText: "#d6d6de"
        }

        TextField {
            id: search

            color: "#d6d6de"
            placeholderText: "Search title, artist or album"
            placeholderTextColor: "#6a6a76"
            selectByMouse: true
            width: 300

            onTextChanged: MusicLibrary.filterText = text

            palette.base: "#15151f"
            palette.highlight: "#2a2a3a"
            palette.highlightedText: "white"
            palette.text: "#d6d6de"
        }
    }

    ListView {
        id: list

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: controls.bottom
        anchors.topMargin: 18
        clip: true
        model: MusicLibrary.songs
        spacing: 6

        delegate: Item {
            id: row

            required property int index
            required property var modelData

            readonly property bool current: modelData !== null && modelData === Player.currentSong

            height: 76
            width: ListView.view.width

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 40
                anchors.rightMargin: 40
                color: row.current ? "#15151f" : (rowMouse.containsMouse ? "#15151f" : "transparent")
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

    Text {
        anchors.centerIn: parent
        color: "#5c5c68"
        font.pixelSize: 15
        text: "No matching songs"
        visible: !MusicLibrary.scanning && MusicLibrary.count === 0 && MusicLibrary.filterText.length > 0
    }
}
