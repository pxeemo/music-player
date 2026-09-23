import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 1000
    height: 640
    visible: true
    title: "Karaoke · Qt 6 ShaderEffect demo"
    color: "#08080c"

    LyricsModel {
        id: lyrics
        Component.onCompleted: loadDemoLyrics()
    }

    // A single running clock drives every line.
    PlaybackClock {
        id: clock
        duration: Math.max(lyrics.duration + 2.0, 1.0)
        playing: true
    }

    Shortcut {
        sequence: "Space"
        onActivated: clock.toggle()
    }

    Item {
        id: stage
        anchors.fill: parent
        anchors.margins: 56

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            spacing: 34

            Repeater {
                model: lyrics.lines

                KaraokeLine {
                    required property int index

                    line: lyrics.lineAt(index)
                    position: clock.position
                    active: lyrics.activeLine(clock.position) === index
                }
            }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        spacing: 14

        Text {
            text: clock.playing ? "Pause" : "Play"
            color: "#7d7d88"
            font.pixelSize: 13
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: clock.toggle()
            }
        }

        Text {
            text: window.formatTime(clock.position) + "  /  " + window.formatTime(clock.duration)
            color: "#4e4e58"
            font.family: "monospace"
            font.pixelSize: 13
        }
    }

    function formatTime(seconds) {
        var s = Math.max(0, seconds)
        var m = Math.floor(s / 60)
        var r = Math.floor(s % 60)
        return m + ":" + (r < 10 ? "0" : "") + r
    }
}
