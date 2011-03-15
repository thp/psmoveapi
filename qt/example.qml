/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
 **/

import Qt 4.7

import com.thpinfo.psmove 1.0

Rectangle {
    color: Qt.darker(bg.color, 1.5)
    width: 500
    height: width

    Rectangle {
        id: bg
        width: ((parent.width < parent.height)?parent.width:parent.height) / 2
        height: width
        anchors.centerIn: parent

        rotation: move.trigger / 255 * 90
        color: move.color

        PSMove {
            id: move
            enabled: true
            rumble: trigger / 2
        }

        SequentialAnimation {
            loops: Animation.Infinite
            running: true

            ColorAnimation {
                target: move
                property: 'color'
                to: 'red'
                duration: 2000
            }
            ColorAnimation {
                target: move
                property: 'color'
                to: 'blue'
                duration: 2000
            }
            ColorAnimation {
                target: move
                property: 'color'
                to: 'green'
                duration: 2000
            }
        }
    }
}

