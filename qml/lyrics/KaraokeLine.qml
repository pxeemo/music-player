pragma ComponentBehavior: Bound

import QtQuick

// One synced lyric line.
//
//   Flow of Text words      -> ShaderEffectSource  (glyph texture)
//   Repeater of gradients   -> ShaderEffectSource  (reading-order coordinate)
//   ShaderEffect            -> recolours along the reading order
//
// A line may wrap onto several rows. The highlight follows the *reading order*,
// not the screen's x axis: a second texture gives every fragment its position
// along the reading order, and the shader compares that with `progress`. Without
// this, a wrapped line would highlight both rows at the same x at once.
//
// Nothing about the text is rebuilt or recoloured on the CPU: the only value
// that changes per frame is `progress`, a normalised 0..1 position.
//
// Input is a list of timed words (`TimedWord`), so this renderer does not depend
// on any particular lyrics source.

Item {
    id: root

    // ---- supplied by the view -------------------------------------------
    // Timed fragments for a synced line, in reading order. Items expose
    // `text`, `start`, `end` and `continues` (true for a syllable that belongs
    // to the word before it, so it is drawn flush against it).
    property var words: []
    property real lineStart: 0
    property real lineEnd: 0
    property real position: 0        // playback clock, in seconds
    property bool active: false
    property bool rtl: false
    // Align the line to the reading end (right for LTR, left for RTL), used to
    // put different singers on opposite sides.
    property bool alignEnd: false

    // ---- appearance (all adjustable from QML) ----------------------------
    property color baseColor: "#9b9ba4"      // not sung yet
    property color sungColor: "#ffffff"      // the part that has been sung
    property color glowColor: "#ffd9ec"      // bright band at the playhead
    property real edge: 0.05                 // soft boundary, in line widths
    property real glow: 0.55                 // strength of the bright band
    property real sizeReduce: 0.94           // how much smaller the unsung line gets
    property int transitionDuration: 1000    // ms for the activate/deactivate tween
    property int transitionTiming: Easing.InOutQuart
    property font textFont: Qt.font({
        pixelSize: 42,
        weight: Font.DemiBold
    })
    readonly property real pad: 0           // breathing room around the glyphs

    readonly property int wordCount: wordItems.count
    readonly property real wordSpacing: textFont.pixelSize * 0.3
    readonly property real progress: computeProgress(position)
    // Total reading-order length of the line, used to normalise positions.
    readonly property real readingWidth: computeReadingWidth()

    // ---- animated state ---------------------------------------------------
    // These flip the instant `active` changes; the Behaviors below tween them,
    // so a line fades up and grows when it starts being sung, then fades back
    // down and shrinks once the next line takes over - no colour or size snap.
    property real inactiveDim: 0.6
    property real dim: active ? 1.0 : inactiveDim

    scale: active ? 1.0 : sizeReduce
    transformOrigin: root.alignEnd !== root.rtl ? Item.Right : Item.Left

    Behavior on dim {
        NumberAnimation {
            duration: root.transitionDuration
            easing.type: root.transitionTiming
        }
    }
    Behavior on scale {
        NumberAnimation {
            duration: root.transitionDuration
            easing.type: root.transitionTiming
        }
    }

    // The view supplies the width (one lyric column); the height follows the
    // laid out content.
    height: textItem.height

    // ---- the text itself -------------------------------------------------
    Item {
        id: textItem

        height: flow.height + 2 * root.pad
        width: root.width

        WordFlow {
            id: flow

            width: root.width - 2 * root.pad
            wordSpacing: root.wordSpacing
            x: root.pad
            y: root.pad

            // WordFlow does the alignment itself, per wrapped row, so the line
            // is never offset here as well - offsetting on top of it would push
            // an end-aligned single row out of the box on the right.
            rtl: root.rtl
            horizontalAlignment: root.rtl !== root.alignEnd ? Text.AlignRight : Text.AlignLeft

            Repeater {
                id: wordItems

                model: root.words

                Text {
                    id: wordText

                    required property int index
                    required property var modelData

                    // A syllable is its own text object so the sweep can follow
                    // its timing; WordFlow glues it to the word before it, so it
                    // is drawn flush with no gap.
                    property bool continues: modelData.continues

                    // True when the next fragment is a syllable of this one, so
                    // this text ends with a joiner. Read from the model rather
                    // than from the sibling item: while the delegates are being
                    // created the next one does not exist yet.
                    property bool continuesNext: root.words.length > index + 1
                        && root.words[index + 1].continues === true

                    // A joiner only does something between letters that join
                    // (Arabic and its relatives). In any other script it is an
                    // invisible glyph the font has to borrow from a fallback,
                    // whose larger metrics grow the line box and push the
                    // syllable down - so it is left out there.
                    property bool joins: /[\u0600-\u06FF\u0700-\u08FF\u1800-\u18AF\uFB50-\uFDFF\uFE70-\uFEFF]/.test(modelData.text)

                    color: root.baseColor
                    // Rebuilt from the animated pixelSize so the glyphs are
                    // re-rasterised crisp at every size instead of being scaled.
                    font: Qt.font({
                        family: root.textFont.family ? root.textFont.family : "",
                        weight: root.textFont.weight,
                        pixelSize: root.textFont.pixelSize
                    })
                    lineHeight: 1.0
                    text: (continues && joins ? "\u200D" : "")
                        + modelData.text
                        + (continuesNext && joins ? "\u200D" : "")
                    width: Math.min(flow.width, implicitWidth)
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
    }

    // The reading-order coordinate, encoded one gradient rectangle per word.
    // It lives in its own item (never in the glyph texture) and only its red
    // channel is read: an opaque gradient from the word's start to its end
    // along the reading order, normalised by the line's total length. Glyph
    // pixels are always inside a word box, so every visible fragment gets the
    // right reading position even when words wrap onto a new row.
    Item {
        id: coordItem

        anchors.fill: textItem

        Repeater {
            model: root.words

            Rectangle {
                id: coord

                required property int index
                required property var modelData

                readonly property var wordItem: wordItems.count > index ? wordItems.itemAt(index) : null
                readonly property var span: root.wordSpan(index)

                height: wordItem ? wordItem.height : 0
                width: wordItem ? wordItem.width : 0
                x: flow.x + (wordItem ? wordItem.x : 0)
                y: flow.y + (wordItem ? wordItem.y : 0)

                gradient: Gradient {
                    orientation: Gradient.Horizontal

                    GradientStop {
                        color: Qt.rgba(coord.span[root.rtl ? 1 : 0], 0, 0, 1)
                        position: 0.0
                    }

                    GradientStop {
                        color: Qt.rgba(coord.span[root.rtl ? 0 : 1], 0, 0, 1)
                        position: 1.0
                    }
                }
            }
        }
    }

    // The line rendered once into a texture. It is kept transparent because
    // ShaderEffect below draws the visible result; this item only feeds it.
    ShaderEffectSource {
        id: textTexture

        hideSource: true
        opacity: 0
        sourceItem: textItem
    }

    // Same idea for the reading-order coordinate texture.
    ShaderEffectSource {
        id: coordTexture

        hideSource: true
        opacity: 0
        sourceItem: coordItem
    }

    ShaderEffect {
        anchors.fill: textItem

        property real progress: root.progress
        property real edge: root.edge
        property real dim: root.dim
        property real inactiveDim: root.inactiveDim
        property real glow: root.glow
        property color baseColor: root.baseColor
        property color sungColor: root.sungColor
        property color glowColor: root.glowColor
        property var source: textTexture
        property var coords: coordTexture

        fragmentShader: "qrc:/shaders/karaoke.frag.qsb"
        vertexShader: "qrc:/shaders/karaoke.vert.qsb"
    }

    function lerp(t0, x0, t1, x1, t) {
        if (t1 <= t0)
            return x1;
        var f = (t - t0) / (t1 - t0);
        if (f < 0)
            f = 0;
        else if (f > 1)
            f = 1;
        return x0 + (x1 - x0) * f;
    }

    // Normalised [start, end] reading position of fragment `index`.
    function wordSpan(index) {
        var count = root.wordCount;
        if (count === 0)
            return [0, 0];
        var total = root.readingWidth;
        var acc = 0;
        for (var i = 0; i < count; ++i) {
            var item = wordItems.itemAt(i);
            var w = item ? item.width : 0;
            if (i === index)
                return [acc / total, (acc + w) / total];
            acc += w + root.gapAfter(i);
        }
        return [0, 0];
    }

    // Word gap after fragment `index`: none when the next fragment continues
    // the same word, so syllables share the word's pixels with no space.
    function gapAfter(index) {
        if (index + 1 >= root.wordCount)
            return 0;
        var next = wordItems.itemAt(index + 1);
        return next && next.continues ? 0 : root.wordSpacing;
    }

    // Sum of every fragment's width plus the word gaps between them. Read from
    // the laid out items, so it is correct once WordFlow has wrapped.
    function computeReadingWidth() {
        var count = root.wordCount;
        if (count === 0)
            return 1;
        var sum = 0;
        for (var i = 0; i < count; ++i) {
            var item = wordItems.itemAt(i);
            sum += (item ? item.width : 0) + root.gapAfter(i);
        }
        return sum > 0 ? sum : 1;
    }

    // Maps playback time to a position along the reading order, interpolating
    // between the anchors of each word and the gaps between them. Same maths as
    // a single-row line, but on the reading-order axis instead of x.
    function computeProgress(t) {
        var count = root.wordCount;
        if (count === 0)
            return 0;
        if (t <= lineStart)
            return 0;
        if (t >= lineEnd)
            return 1;

        var total = root.readingWidth;
        if (total <= 0)
            return 0;
        var prevT = lineStart;
        var prevR = 0;
        var acc = 0;

        for (var i = 0; i < count; ++i) {
            var item = wordItems.itemAt(i);
            if (!item)
                break;
            var word = item.modelData;
            if (!word)
                break;
            var w = item.width;
            var r0 = acc;
            var r1 = acc + w;

            if (t <= word.start)          // in the gap before this fragment
                return lerp(prevT, prevR / total, word.start, r0 / total, t);
            if (t <= word.end)            // inside this fragment
                return lerp(word.start, r0 / total, word.end, r1 / total, t);

            prevT = word.end;
            prevR = r1;
            acc += w + root.gapAfter(i);
        }

        // Past the last word: sweep the remainder of the line.
        return lerp(prevT, prevR / total, lineEnd, 1, t);
    }
}
