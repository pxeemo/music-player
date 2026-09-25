pragma ComponentBehavior: Bound
import QtQuick

// One lyric line.
//
//   Text (one static Text per word, laid out in a Row)
//     -> ShaderEffectSource  (the whole line captured as one texture)
//     -> ShaderEffect        (GPU recolours it from left to right)
//
// Nothing about the text is rebuilt or recoloured on the CPU: the only value
// that changes per frame is `progress`, a normalised 0..1 position.

Item {
    id: root

    // ---- supplied by the view -------------------------------------------
    property LyricLine line: null
    property real position: 0        // playback clock, in seconds
    property bool active: false

    // ---- appearance (all adjustable from QML) ----------------------------
    property color baseColor: "#9b9ba4"      // not sung yet
    property color sungColorA: "#ffffff"     // the part that has been sung
    property color glowColor: "#ffd9ec"      // bright band at the playhead
    property real edge: 0.03                 // soft boundary, in line widths
    property real glow: 0.55                 // strength of the bright band
    property real inactiveDim: 0.5           // dimming of the non-active lines
    property real activeDim: 1.0             // dimming of the line being sung
    property real sizeBoost: 1.06            // how much bigger the singing line gets
    property int transitionDuration: 320     // ms for the activate/deactivate tween
    property font textFont: Qt.font({
        pixelSize: 34,
        weight: Font.DemiBold
    })
    readonly property real pad: 12           // breathing room around the glyphs

    readonly property var wordList: line ? line.words : []
    readonly property real progress: computeProgress(position)

    // ---- animated state ---------------------------------------------------
    // These flip the instant `active` changes; the Behaviors below tween them,
    // so a line fades up and grows when it starts being sung, then fades back
    // down and shrinks once the next line takes over - no colour or size snap.
    property real dim: active ? activeDim : inactiveDim
    property real pixelSize: active ? textFont.pixelSize * sizeBoost : textFont.pixelSize
    // Follows pixelSize directly (it is a pure function of it), so it must not
    // get its own Behavior - that would queue a second tween behind the first.
    property real wordGap: pixelSize * 0.30

    Behavior on dim {
        NumberAnimation {
            duration: root.transitionDuration
            easing.type: Easing.InOutCubic
        }
    }
    Behavior on pixelSize {
        NumberAnimation {
            duration: root.transitionDuration
            easing.type: Easing.InOutCubic
        }
    }

    width: textItem.width
    height: textItem.height

    // ---- the text itself -------------------------------------------------
    Item {
        id: textItem
        width: row.width + 2 * root.pad
        height: row.height + 2 * root.pad

        Row {
            id: row
            x: root.pad
            y: root.pad
            spacing: root.wordGap

            Repeater {
                id: words
                model: root.wordList

                Text {
                    required property var modelData

                    text: modelData.text
                    color: root.baseColor
                    // Rebuilt from the animated pixelSize so the glyphs are
                    // re-rasterised crisp at every size instead of being scaled.
                    font: Qt.font({
                        family: root.textFont.family,
                        weight: root.textFont.weight,
                        pixelSize: root.pixelSize
                    })
                }
            }
        }
    }

    // The line rendered once into a texture. It is kept transparent because
    // ShaderEffect below draws the visible result; this item only feeds it.
    ShaderEffectSource {
        id: textTexture
        sourceItem: textItem
        hideSource: true
        opacity: 0
    }

    ShaderEffect {
        anchors.fill: textItem

        property real progress: root.progress
        property real edge: root.edge
        property real dim: root.dim
        property real glow: root.glow
        property color baseColor: root.baseColor
        property color sungColorA: root.sungColorA
        property color glowColor: root.glowColor
        property var source: textTexture

        vertexShader: "qrc:/shaders/karaoke.vert.qsb"
        fragmentShader: "qrc:/shaders/karaoke.frag.qsb"
    }

    // Maps playback time to a horizontal position inside this line's texture.
    //
    // The word boundaries come from the timing data; the x positions come from
    // where the words were actually laid out. Between two anchors the result is
    // interpolated, so the highlight moves continuously instead of jumping from
    // word to word.
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

    function computeProgress(t) {
        if (!line || wordList.length === 0)
            return 0;
        if (t <= line.start)
            return 0;
        if (t >= line.end)
            return 1;

        var texWidth = textItem.width;
        if (texWidth <= 0)
            return 0;

        var prevT = line.start;
        var prevX = 0;

        for (var i = 0; i < wordList.length; ++i) {
            var word = wordList[i];
            var item = words.itemAt(i);
            if (!item)
                break;
            var x0 = (root.pad + item.x) / texWidth;
            var x1 = (root.pad + item.x + item.width) / texWidth;

            if (t <= word.start)          // in the gap before this word
                return lerp(prevT, prevX, word.start, x0, t);
            if (t <= word.end)            // inside this word
                return lerp(word.start, x0, word.end, x1, t);

            prevT = word.end;
            prevX = x1;
        }

        // Past the last word: sweep the remainder of the line.
        return lerp(prevT, prevX, line.end, 1, t);
    }
}
