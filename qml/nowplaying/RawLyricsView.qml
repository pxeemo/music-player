pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// A scrollable block of plain text. Used to show the raw parsed-lyrics dump.
Item {
    id: root

    property string text
    property string emptyText: "(no lyrics document)"
    property bool monospace: false

    readonly property bool empty: !text || text.length === 0

    Flickable {
        id: scroller

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: label.height
        contentWidth: width
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: ScrollBar {}

        Text {
            id: label

            color: root.empty ? "#5c5c68" : "#d6d6de"
            font.family: root.monospace ? "monospace" : ""
            font.pixelSize: root.monospace ? 12 : 16
            lineHeight: 1.35
            lineHeightMode: Text.ProportionalHeight
            text: root.empty ? root.emptyText : root.text
            width: scroller.width
            wrapMode: Text.Wrap
        }
    }
}
