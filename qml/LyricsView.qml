pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Controls

// A plain lyric sheet over a Song's structured lyrics document.
//
// Every row is a single Text - no shader, no gradient - and the only animation
// is a colour fade when a line becomes the one being played (following the
// KaraokeLine transition timing, without its progress sweep). Background vocals
// and translations are drawn as their own, smaller rows rather than hidden
// under the main line. Clicking a timed row seeks to its start.
Item {
    id: root

    property Song song
    property int transitionDuration: 1000
    property int transitionTiming: Easing.InOutQuart

    readonly property var document: root.song ? root.song.document : null
    readonly property real positionMs: Player.position * 1000
    readonly property bool timed: root.document ? root.document.timed : false
    readonly property int activeGroup: root.document ? root.document.activeGroupIndex(positionMs) : -1
    readonly property real lineSize: Math.max(20, Math.min(32, width * 0.075))

    onActiveGroupChanged: root.scrollToGroup(root.activeGroup)

    function scrollToGroup(group) {
        if (!root.document || group < 0)
            return;
        var rowIndex = root.document.firstRowOfGroup(group);
        if (rowIndex >= 0)
            scroller.centerOn(rowIndex);
    }

    // Colour of one row, by kind and whether it is the current line. With no
    // timing at all (a document a parser hasn't timed yet) every line is simply
    // normal text.
    function rowColor(item) {
        var row = item.modelData;
        if (row.isSection || row.isInstrumental)
            return "#5c5c68";
        if (!root.timed) {
            if (row.isMain)
                return "#c9c9d2";
            return "#8b8b96";
        }
        if (item.active) {
            if (row.isMain)
                return "#ffffff";
            if (row.isBackground)
                return "#dcdce6";
            return "#c9c9d2";
        }
        if (row.isMain)
            return "#9b9ba4";
        return "#6f6f7c";
    }

    Flickable {
        id: scroller

        readonly property real edgeSlack: Math.max(0, (height - column.height) / 2)

        function centerOn(index) {
            if (index < 0)
                return;
            var item = repeater.itemAt(index);
            if (!item)
                return;
            var target = column.y + item.y - height / 4;
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

            spacing: 0
            y: scroller.edgeSlack
            topPadding: scroller.height / 4
            bottomPadding: scroller.height / 4 * 3 - (lastItem ? lastItem.height : 0)

            Repeater {
                id: repeater

                model: root.document ? root.document.rows : null

                delegate: Item {
                    id: rowItem

                    required property int index
                    required property var modelData

                    readonly property bool active: root.timed && root.activeGroup >= 0 && modelData.groupIndex === root.activeGroup
                    readonly property bool secondary: modelData.isBackground || modelData.isTranslation
                    readonly property real textSize: modelData.isMain ? root.lineSize : (modelData.isSection ? root.lineSize * 0.6 : (modelData.isInstrumental ? root.lineSize * 0.62 : root.lineSize * 0.7))
                    readonly property real topGap: modelData.groupStart ? 18 : 4

                    height: topGap + label.height
                    width: column.width

                    Text {
                        id: label

                        color: root.rowColor(rowItem)
                        font.italic: rowItem.modelData.isTranslation
                        font.pixelSize: rowItem.textSize
                        font.weight: rowItem.modelData.isMain ? Font.DemiBold : Font.Normal
                        text: rowItem.modelData.text
                        width: parent.width
                        wrapMode: Text.Wrap
                        y: rowItem.topGap

                        Behavior on color {
                            ColorAnimation {
                                duration: root.transitionDuration
                                easing.type: root.transitionTiming
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: rowItem.modelData.startMs >= 0 ? Qt.PointingHandCursor : Qt.ArrowCursor

                        onClicked: if (rowItem.modelData.startMs >= 0)
                            Player.seek(rowItem.modelData.startMs / 1000.0)
                    }
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        color: "#5c5c68"
        font.pixelSize: 15
        text: "No lyrics available"
        visible: !root.document || root.document.empty
    }
}
