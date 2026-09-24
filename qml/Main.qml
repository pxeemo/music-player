import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 1000
    height: 640
    visible: true
    title: "Karaoke · Qt 6 ShaderEffect demo"
    color: "#08080c"

    // The line currently being sung: drives both the highlight and the view.
    readonly property int activeLine: lyrics.activeLine(clock.position)

    onActiveLineChanged: {
        console.log("activeLine -> " + activeLine)
        scroller.centerOn(activeLine)
    }

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

    function formatTime(seconds) {
        var s = Math.max(0, seconds)
        var m = Math.floor(s / 60)
        var r = Math.floor(s % 60)
        return m + ":" + (r < 10 ? "0" : "") + r
    }

    Item {
        id: stage
        anchors.fill: parent
        anchors.margins: 56

        // More lines than fit in the window: flick with the wheel or a drag to
        // read ahead, and the view recenters whenever a line starts being sung.
        Flickable {
            id: scroller

            anchors.fill: parent
            contentWidth: width
            contentHeight: column.height + 2 * edgeSlack
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            // Half a screen of slack above and below the list while it still
            // fits, which keeps a short list centered exactly as it was before
            // scrolling existed. Once it overflows the slack is zero and the
            // list behaves like any other scrollable view.
            readonly property real edgeSlack: Math.max(0, (height - column.height) / 2)

            ScrollBar.vertical: ScrollBar { }

            // TEMPORARY instrumentation
            onDraggingChanged: {
                console.log("flickable dragging=" + dragging)
                if (dragging)
                    centering.stop()
            }
            onFlickingChanged: console.log("flickable flicking=" + flicking)
            onHeightChanged: contentY = Math.min(contentY, Math.max(0, contentHeight - height))

            // Brings the given line to the middle of the window.
            function centerOn(index) {
                if (index < 0)
                    return
                var item = repeater.itemAt(index)
                if (!item)
                    return
                var target = column.y + item.y + item.height / 2 - height / 2
                var to = Math.max(0, Math.min(target, contentHeight - height))
                console.log("centerOn index=" + index + " target=" + target + " to=" + to
                            + " contentY=" + contentY + " viewH=" + height
                            + " colY=" + column.y + " itemY=" + item.y)
                centering.to = to
                centering.restart()
            }

            NumberAnimation {
                id: centering
                target: scroller
                property: "contentY"
                duration: 420
                easing.type: Easing.InOutCubic
                onRunningChanged: console.log("centering running=" + running
                                               + " contentY=" + scroller.contentY
                                               + " to=" + to)
                onStopped: console.log("centering stopped at contentY=" + scroller.contentY)
            }

            Column {
                id: column
                y: scroller.edgeSlack
                spacing: 34

                Repeater {
                    id: repeater
                    model: lyrics.lines

                    KaraokeLine {
                        required property int index

                        line: lyrics.lineAt(index)
                        position: clock.position
                        active: window.activeLine === index
                    }
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
}
