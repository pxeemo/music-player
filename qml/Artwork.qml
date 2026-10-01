pragma ComponentBehavior: Bound

import Karaoke 1.0
import QtQuick
import QtQuick.Effects

// A track's cover: the embedded artwork when there is one, otherwise a gradient
// built from the track's colours with the title initial on top. The rounded
// corners are applied with a mask so they clip the image, not just the fallback.
Item {
    id: root

    property Song song
    property real corner: width * 0.08

    readonly property bool hasArtwork: root.song && root.song.artworkSource.toString().length > 0

    // The gradient fallback, visible until (and behind) the image.
    Rectangle {
        anchors.fill: parent
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
        visible: !artworkImage.visible

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

    Image {
        id: artworkImage

        anchors.fill: parent
        asynchronous: true
        cache: true
        fillMode: Image.PreserveAspectCrop
        source: root.hasArtwork ? root.song.artworkSource : ""
        sourceSize.height: Math.max(1, Math.round(root.height))
        sourceSize.width: Math.max(1, Math.round(root.width))
        visible: false
    }

    // Renders the image through a rounded mask.
    MultiEffect {
        anchors.fill: parent
        autoPaddingEnabled: false
        maskEnabled: true
        maskSource: roundedMask
        source: artworkImage
        visible: artworkImage.status === Image.Ready
    }

    Item {
        id: roundedMask

        anchors.fill: parent
        layer.enabled: true
        visible: false

        Rectangle {
            anchors.fill: parent
            color: "white"
            radius: root.corner
        }
    }
}
