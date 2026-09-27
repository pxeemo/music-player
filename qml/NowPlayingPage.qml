pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick

// The now-playing screen: cover on the left, the collapsible lyric panel on
// the right. Pushed by SongsPage with the tapped Song.
Item {
    id: page

    property Song song
    property var stackView
    property bool lyricsExpanded: true

    readonly property int activeLine: page.song ? page.song.activeLine(PlaybackClock.position) : -1

    function formatTime(seconds) {
        var s = Math.max(0, seconds);
        var m = Math.floor(s / 60);
        var r = Math.floor(s % 60);
        return m + ":" + (r < 10 ? "0" : "") + r;
    }

    Component.onCompleted: {
        PlaybackClock.seek(0);
        PlaybackClock.playing = true;
    }

    // Leaving the screen (popped back to the list) stops playback. On
    // destruction covers every way the page can go away.
    Component.onDestruction: PlaybackClock.playing = false

    Binding {
        property: "duration"
        target: PlaybackClock
        value: page.song ? Math.max(page.song.duration + 2.0, 1.0) : 1.0
    }

    Rectangle {
        anchors.fill: parent
        color: "#08080c"
    }

    // A wash of the song's colour behind everything.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                color: page.song ? page.song.colorB : "#08080c"
                position: 0.0
            }
            GradientStop {
                color: "#08080c"
                position: 0.55
            }
        }
        opacity: 0.22
    }

    // ---- back ------------------------------------------------------------
    Text {
        id: back

        color: "#9b9ba4"
        font.pixelSize: 15
        text: "\u2039  Songs"
        x: 48
        y: 40

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: page.stackView.pop()
        }
    }

    // ---- artwork | lyrics ------------------------------------------------
    Row {
        id: layout

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 44
        anchors.left: parent.left
        anchors.leftMargin: 48
        anchors.right: parent.right
        anchors.rightMargin: 48
        anchors.top: parent.top
        anchors.topMargin: 96
        spacing: 36

        Item {
            id: left

            height: parent.height
            width: parent.width - panel.width - parent.spacing

            Column {
                id: info

                anchors.centerIn: parent
                spacing: 20
                width: parent.width

                Artwork {
                    id: cover

                    height: width
                    song: page.song
                    width: Math.min(left.width * 0.72, left.height * 0.5)
                    x: (parent.width - width) / 2
                }

                Text {
                    color: "white"
                    elide: Text.ElideRight
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    text: page.song ? page.song.title : ""
                    width: parent.width
                }

                Text {
                    color: "#a0a0ac"
                    elide: Text.ElideRight
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    text: page.song ? page.song.artist : ""
                    width: parent.width
                }

                // ---- transport ----
                Item {
                    height: controls.height
                    width: parent.width

                    Column {
                        id: controls

                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 10
                        width: Math.min(parent.width, 340)

                        Item {
                            id: seekbar

                            height: 20
                            width: parent.width

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                color: "#23232e"
                                height: 4
                                radius: 2
                                width: parent.width
                            }

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                color: "white"
                                height: 4
                                radius: 2
                                width: parent.width * (PlaybackClock.duration > 0 ? Math.min(1, PlaybackClock.position / PlaybackClock.duration) : 0)
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor

                                onClicked: if (PlaybackClock.duration > 0)
                                    PlaybackClock.seek(mouse.x / width * PlaybackClock.duration)
                            }
                        }

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 14

                            Text {
                                color: "#cfcfd8"
                                font.pixelSize: 15
                                text: PlaybackClock.playing ? "Pause" : "Play"

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: PlaybackClock.toggle()
                                }
                            }

                            Text {
                                color: "#5c5c68"
                                font.family: "monospace"
                                font.pixelSize: 13
                                text: page.formatTime(PlaybackClock.position) + " / " + page.formatTime(PlaybackClock.duration)
                            }
                        }
                    }
                }

                Rectangle {
                    id: lyricButton

                    color: handleLyricButtonMouse.containsMouse ? "#1c1c28" : (page.lyricsExpanded ? "#d2d2d2" : "#12121a")
                    radius: 12
                    width: 42
                    height: 42
                    anchors.horizontalCenter: parent.horizontalCenter

                    Column {
                        anchors.centerIn: parent

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: page.lyricsExpanded ? "#101010" : "#7d7d88"
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            text: "\u266A"
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: page.lyricsExpanded ? "#101010" : "#4e4e58"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            text: "Lyric"
                        }
                    }

                    MouseArea {
                        id: handleLyricButtonMouse

                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: page.lyricsExpanded = !page.lyricsExpanded
                    }
                }
            }
        }

        LyricsPanel {
            id: panel

            activeLine: page.activeLine
            expanded: page.lyricsExpanded
            expandedWidth: Math.max(300, layout.width * 0.44)
            height: parent.height
            song: page.song
        }
    }
}
