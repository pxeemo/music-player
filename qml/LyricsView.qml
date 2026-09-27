pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Controls

// The scrolling lyric sheet for one song.
//
// This is the old main view, now reusable: it renders a Song's lines with
// KaraokeLine, scrolls itself around whichever line is being sung, and leaves
// layout (window, side panel, collapse animation) to whoever hosts it.
Item {
    id: root

    property Song song
    property int activeLine: -1
    property real lineSize: Math.max(22, Math.min(38, width * 0.062))

    onActiveLineChanged: scroller.centerOn(activeLine)

    // More lines than fit: flick with the wheel or a drag to read ahead, and
    // the view recenters whenever a line starts being sung.
    Flickable {
        id: scroller

        // Half a screen of slack above and below the list while it still fits,
        // which keeps a short list centered. Once it overflows the slack is
        // zero and the list behaves like any other scrollable view.
        readonly property real edgeSlack: Math.max(0, (height - column.height) / 2)

        // Brings the given line to the middle of the view.
        function centerOn(index) {
            if (index < 0)
                return;
            var item = repeater.itemAt(index);
            if (!item)
                return;
            var target = column.y + item.y - height / 5;
            smoothscrolling.to = Math.max(0, Math.min(target, contentHeight - height));
            smoothscrolling.restart();
        }

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: column.height + 2 * edgeSlack
        contentWidth: width
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: ScrollBar {}

        // A hand drag takes priority over the centering animation so the two
        // never fight each other.
        onDraggingChanged: if (dragging)
            smoothscrolling.stop()
        onHeightChanged: contentY = Math.min(contentY, Math.max(0, contentHeight - height))

        NumberAnimation {
            id: smoothscrolling

            duration: 1000
            easing.type: Easing.InOutQuart
            property: "contentY"
            target: scroller
        }

        Column {
            id: column

            property Item lastItem: repeater.itemAt(repeater.count - 1)

            spacing: 24
            y: scroller.edgeSlack
            topPadding: scroller.height / 5
            bottomPadding: scroller.height / 5 * 4 - lastItem.height

            Repeater {
                id: repeater

                model: root.song ? root.song.lyrics : null

                KaraokeLine {
                    required property int index

                    active: root.activeLine === index
                    line: root.song.lineAt(index)
                    position: PlaybackClock.position
                    textFont: Qt.font({
                        pixelSize: root.lineSize,
                        weight: Font.DemiBold
                    })
                }
            }
        }
    }
}
