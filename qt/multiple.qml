
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (C) 2011 Thomas Perl <m@thp.io>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

        rotation: (move.trigger-move2.trigger) / 255 * 90
        color: move.color

        PSMove {
            id: move
            index: 1
            enabled: true
            rumble: move2.trigger / 2
            color: 'red'
        }

        PSMove {
            id: move2
            index: 0
            enabled: true
            rumble: move.trigger / 2
            color: Qt.darker(bg.color, 1+(move2.trigger / 200))
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

