import QtQuick

// Lays out its children as words on rows: `wordSpacing` between words,
// `lineSpacing` between rows, aligned by `horizontalAlignment`, optionally
// reading right to left.
//
// An item that declares `property bool continues: true` is a syllable of the
// word before it rather than a word of its own: it is drawn flush against that
// item, with no `wordSpacing`, and never starts a row. Items without the
// property are treated as words. A row break never splits a word - the whole
// chain moves down together - unless the word alone is wider than the row.
//
// Items in a row sit on a shared baseline (`Item.baselineOffset`) rather than
// sharing the top of their box: a glyph that comes from a fallback font grows
// the line box, and aligning by the box would push that item's text down.
Item {
    id: root

    default property alias content: contentItem.data

    property real wordSpacing: 8
    property real lineSpacing: 0
    property bool rtl: false
    property int horizontalAlignment: Text.AlignLeft

    implicitHeight: contentHeight
    property real contentHeight: 0

    Item {
        id: contentItem

        anchors.fill: parent

        // Children come and go with the model, so re-layout when they do.
        onChildrenChanged: scheduleLayout()
    }

    function scheduleLayout() {
        Qt.callLater(relayout)
    }

    function lineStartX(lineWidth) {
        if (horizontalAlignment === Text.AlignHCenter)
            return (width - lineWidth) / 2

        if (horizontalAlignment === Text.AlignRight)
            return width - lineWidth

        return 0
    }

    // Laid-out width of an item: the width it is actually drawn at, so the
    // reading-order rects painted over it stay in step.
    function itemWidth(item) {
        return item.width > 0 ? item.width : item.implicitWidth
    }

    // Gap before an item: nothing when it continues the item before it (a
    // syllable of the same word), `wordSpacing` when it starts a new word.
    function gapBefore(item) {
        return item.continues === true ? 0 : wordSpacing
    }

    function relayout() {
        if (width <= 0)
            return

        var items = contentItem.children.filter(function(item) {
            return item.visible && item.implicitWidth > 0
        })

        var lines = []
        var line = []
        var lineWidth = 0

        function pushLine() {
            lines.push({
                items: line,
                width: lineWidth
            })

            line = []
            lineWidth = 0
        }

        // Build lines
        for (var i = 0; i < items.length; ) {
            var item = items[i]

            // Find the complete continues chain starting at this item.
            var chain = [item]
            var chainWidth = itemWidth(item)

            var j = i + 1
            while (j < items.length && items[j].continues === true) {
                chain.push(items[j])
                chainWidth += itemWidth(items[j])

                ++j
            }

            var gap = line.length > 0 ? gapBefore(item) : 0

            // The whole chain moves to the next line.
            if (line.length > 0 && lineWidth + gap + chainWidth > width) {
                pushLine()
                gap = 0
            }

            if (chainWidth <= width) {
                // Add the entire chain to this line.
                for (var k = 0; k < chain.length; ++k) {
                    if (k === 0 && line.length > 0)
                        lineWidth += gap

                    line.push(chain[k])
                    lineWidth += itemWidth(chain[k])
                }
            } else {
                // A single word wider than a row on its own: breaking it is
                // the only way to keep it inside the row.
                for (var m = 0; m < chain.length; ++m) {
                    var wideItem = chain[m]

                    if (line.length > 0 && lineWidth + itemWidth(wideItem) > width)
                        pushLine()

                    line.push(wideItem)
                    lineWidth += itemWidth(wideItem)
                }
            }

            i = j
        }

        // Last line
        if (line.length > 0) {
            lines.push({
                items: line,
                width: lineWidth
            })
        }

        // Position lines
        var y = 0

        for (var l = 0; l < lines.length; ++l) {
            var current = lines[l]
            var startX = lineStartX(current.width)

            // The highest baseline in the row: every item is placed so that its
            // own baseline lands on it, so a fragment whose box grew (a glyph
            // from a fallback font) is not pushed down.
            var baseline = 0

            for (var b = 0; b < current.items.length; ++b)
                baseline = Math.max(baseline, current.items[b].baselineOffset)

            var rowHeight = 0

            if (rtl) {
                var rtlX = startX + current.width

                for (var r = 0; r < current.items.length; ++r) {
                    var rtlItem = current.items[r]

                    rtlX -= itemWidth(rtlItem)

                    rtlItem.x = rtlX
                    rtlItem.y = y + baseline - rtlItem.baselineOffset

                    rowHeight = Math.max(rowHeight, rtlItem.y - y + rtlItem.implicitHeight)

                    if (r + 1 < current.items.length)
                        rtlX -= gapBefore(current.items[r + 1])
                }
            } else {
                var ltrX = startX

                for (var n = 0; n < current.items.length; ++n) {
                    var ltrItem = current.items[n]

                    ltrItem.x = ltrX
                    ltrItem.y = y + baseline - ltrItem.baselineOffset

                    rowHeight = Math.max(rowHeight, ltrItem.y - y + ltrItem.implicitHeight)

                    ltrX += itemWidth(ltrItem)
                    if (n + 1 < current.items.length)
                        ltrX += gapBefore(current.items[n + 1])
                }
            }

            y += rowHeight + lineSpacing
        }

        contentHeight = Math.max(0, y - lineSpacing)
    }

    onWidthChanged: scheduleLayout()
    onRtlChanged: scheduleLayout()
    onHorizontalAlignmentChanged: scheduleLayout()
    onWordSpacingChanged: scheduleLayout()
    onLineSpacingChanged: scheduleLayout()

    Component.onCompleted: scheduleLayout()
}
