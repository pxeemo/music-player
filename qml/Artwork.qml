pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick

// A song's cover: a gradient built from the song's two colours with the title
// initial on top. No image files are needed for the demo; a real catalogue
// would swap this for an Image without touching the pages that use it.
Rectangle {
    id: root

    property Song song
    property real corner: width * 0.08

    color: "#1a1a24"
    gradient: Gradient {
        GradientStop {
            color: root.song ? root.song.colorA : "#3a3a55"
            position: 0.0
        }
        GradientStop {
            color: root.song ? root.song.colorB : "#141420"
            position: 1.0
        }
    }
    radius: root.corner

    // A large translucent note behind the initial.
    Text {
        anchors.centerIn: parent

        color: "#33ffffff"
        font.pixelSize: root.width * 0.46
        text: "\u266A"
    }

    Text {
        anchors.centerIn: parent

        color: "white"
        font.bold: true
        font.pixelSize: root.width * 0.32
        text: root.song && root.song.title.length > 0 ? root.song.title.charAt(0) : "?"
    }
}
