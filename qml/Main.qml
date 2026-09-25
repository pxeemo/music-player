pragma ComponentBehavior: Bound
import Karaoke 1.0
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    // The line currently being sung: drives both the highlight and the view.
    readonly property int activeLine: LyricsModel.activeLine(PlaybackClock.position)

    function formatTime(seconds) {
        var s = Math.max(0, seconds);
        var m = Math.floor(s / 60);
        var r = Math.floor(s % 60);
        return m + ":" + (r < 10 ? "0" : "") + r;
    }

    color: "#08080c"
    height: 640
    title: "Karaoke · Qt 6 ShaderEffect demo"
    visible: true
    width: 1000

    Component.onCompleted: {
        LyricsModel.loadDemoLyrics();
        PlaybackClock.playing = true;
    }
    onActiveLineChanged: scroller.centerOn(activeLine)

    Binding {
        property: "duration"
        target: PlaybackClock
        value: Math.max(LyricsModel.duration + 2.0, 1.0)
    }

    Shortcut {
        sequence: "Space"

        onActivated: PlaybackClock.toggle()
    }

    Item {
        id: stage

        anchors.fill: parent
        anchors.margins: 56

        // More lines than fit in the window: flick with the wheel or a drag to
        // read ahead, and the view recenters whenever a line starts being sung.
        Flickable {
            id: scroller

            // Half a screen of slack above and below the list while it still
            // fits, which keeps a short list centered exactly as it was before
            // scrolling existed. Once it overflows the slack is zero and the
            // list behaves like any other scrollable view.
            readonly property real edgeSlack: Math.max(0, (height - column.height) / 2)

            // Brings the given line to the middle of the window.
            function centerOn(index) {
                if (index < 0)
                    return;
                var item = repeater.itemAt(index);
                if (!item)
                    return;
                var target = column.y + item.y + item.height / 2 - height / 2;
                centering.to = Math.max(0, Math.min(target, contentHeight - height));
                centering.restart();
            }

            anchors.fill: parent
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            contentHeight: column.height + 2 * edgeSlack
            contentWidth: width
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: ScrollBar {}

            // A hand drag takes priority over the centering animation so the
            // two never fight each other.
            onDraggingChanged: if (dragging)
                centering.stop()
            onHeightChanged: contentY = Math.min(contentY, Math.max(0, contentHeight - height))

            NumberAnimation {
                id: centering

                duration: 420
                easing.type: Easing.InOutCubic
                property: "contentY"
                target: scroller
            }

            Column {
                id: column

                spacing: 34
                y: scroller.edgeSlack

                Repeater {
                    id: repeater

                    model: LyricsModel.lines

                    KaraokeLine {
                        required property int index

                        active: window.activeLine === index
                        line: LyricsModel.lineAt(index)
                        position: PlaybackClock.position
                    }
                }
            }
        }
    }

    Row {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 14

        Text {
            color: "#7d7d88"
            font.pixelSize: 13
            text: PlaybackClock.playing ? "Pause" : "Play"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor

                onClicked: PlaybackClock.toggle()
            }
        }

        Text {
            color: "#4e4e58"
            font.family: "monospace"
            font.pixelSize: 13
            text: window.formatTime(PlaybackClock.position) + " / " + window.formatTime(PlaybackClock.duration)
        }
    }
}
