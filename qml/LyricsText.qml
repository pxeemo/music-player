pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// Plain, unsynced lyrics: whatever text we extracted from a sidecar .lrc/.ttml
// or an embedded tag. Real timing is a later phase; this just makes it readable.
Item {
    id: root

    property string text
    /// Monospaced, smaller text - used for the raw parser dump.
    property bool monospace: false
    property string emptyText: "No lyrics available"

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
