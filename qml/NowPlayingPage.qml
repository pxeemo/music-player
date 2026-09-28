pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Layouts

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
    RowLayout {
        id: layout

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: back.bottom
        anchors.margins: 48
        spacing: 42

        Item {
            id: infoPanel

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: info.width

            // visible: parent.width >= 800 || !page.lyricsExpanded

            Behavior on Layout.preferredWidth {
                NumberAnimation {
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }

            Column {
                id: info

                anchors.centerIn: parent
                width: 250
                spacing: 20

                Artwork {
                    id: cover

                    song: page.song
                    width: parent.width
                    height: width
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

                                onClicked: function (mouse) {
                                    if (PlaybackClock.duration > 0)
                                        PlaybackClock.seek(mouse.x / width * PlaybackClock.duration);
                                }
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

        // The collapsible lyrics half of the now-playing screen.
        //
        // When expanded it shows the full lyric sheet with a slim handle on its right
        // edge; clicking the handle collapses the panel down to just that handle and
        // the artwork next to it fills the freed space. The width is animated here so
        // the host page can simply lay it out in a Row and let it repartition.
        Item {
            id: lyricsPanel

            property Song song: page.song
            property bool expanded: page.lyricsExpanded
            property int activeLine: page.activeLine

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: page.lyricsExpanded ? 700 : 0

            Behavior on Layout.minimumWidth {
                NumberAnimation {
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }

            opacity: page.lyricsExpanded ? 1.0 : 0.0

            Behavior on opacity {
                NumberAnimation {
                    duration: 350
                    easing.type: Easing.Linear
                }
            }

            clip: true

            // ---- lyrics ----------------------------------------------------------
            Item {
                id: content

                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.top: parent.top
                clip: true
                opacity: lyricsPanel.expanded ? 1 : 0
                width: Math.max(0, lyricsPanel.width)

                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                LyricsView {
                    anchors.fill: parent
                    activeLine: lyricsPanel.activeLine
                    song: lyricsPanel.song
                }
            }
        }
    }
}
