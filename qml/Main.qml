pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Controls

// Window shell. The app has two screens: the songs list (root) and the
// now-playing screen pushed on top of it. Everything lyrics-related lives in
// NowPlayingPage/LyricsPanel; this file only owns the window and navigation.
ApplicationWindow {
    id: window

    color: "#08080c"
    height: 640
    title: "Karaoke"
    visible: true
    width: 1000

    Component.onCompleted: SongsModel.loadDemoSongs()

    StackView {
        id: stack

        anchors.fill: parent
        initialItem: SongsPage {
            stackView: stack
        }
    }

    Shortcut {
        enabled: stack.depth > 1
        sequence: "Space"

        onActivated: PlaybackClock.toggle()
    }
}
