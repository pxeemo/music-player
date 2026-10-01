pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Controls

// Window shell. The app has two screens: the songs list (root) and the
// now-playing screen pushed on top of it. A mini player sits under both while
// something is playing; tapping it returns to now playing.
ApplicationWindow {
    id: window
    flags:  Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint
    readonly property bool miniVisible: Player.currentSong !== null && stack.depth === 1
    
    color: "#08080c"
    height: screen.height * 0.8
    title: "Karaoke"
    visible: true
    width: screen.width * 0.8

    Component.onCompleted: MusicLibrary.scan()

    


    Item {
        anchors.fill: parent

        StackView {
            id: stack

            anchors.bottom: mini.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            initialItem: SongsPage {
                stackView: stack
            }
        }

        MiniPlayer {
            id: mini

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            clip: true
            height: window.miniVisible ? 72 : 0
            opacity: window.miniVisible ? 1 : 0
            stackView: stack

            Behavior on height {
                NumberAnimation {
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: 200
                }
            }
        }
    }

    Shortcut {
        enabled: stack.depth > 1
        sequence: "Space"

        onActivated: Player.toggle()
    }
}
