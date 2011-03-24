
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

import QtMultimediaKit 1.1

import com.thpinfo.psmove 1.0

Rectangle {
    id: root
    color: state=='a'?Qt.darker(bg.color, 2):'black'
    width: 500
    height: width
    property bool first: true

    state: 'a'
    states: [
        State {
            name: 'a'

            PropertyChanges {
                target: bg
                scale: 1
                opacity: 1
            }

            PropertyChanges {
                target: card
                scale: .001
                opacity: 0
            }
        },

        State {
            name: 'b'

            PropertyChanges {
                target: bg
                scale: .1
                opacity: 0
            }

            PropertyChanges {
                target: card
                scale: 1
                opacity: 1
            }
        }
    ]

    MouseArea {
        anchors.fill: parent
        onClicked: {
            click.stop()
            click.play()
        }
    }

    Audio {
        id: click
        source: 'qrc:/click.wav'
    }

    Audio {
        id: select
        source: 'qrc:/select.wav'
    }

    Rectangle {
        id: bg
        width: parent.width - 100 // ((parent.width < parent.height)?parent.width:parent.height) * .8
        height: parent.height - 100 // width
        anchors.centerIn: parent

        Behavior on scale { PropertyAnimation { duration: 900; easing.type: Easing.OutElastic } }
        Behavior on opacity { PropertyAnimation { duration: 900; easing.type: Easing.OutExpo } }

        //rotation: move.trigger / 255 * 90
        color: move.color

        Image {
            id: backdrop
            property real opa: 0

            source: list.model.get(list.currentIndex).cover
            anchors.centerIn: parent
            opacity: (status == Image.Ready)?opa:0
            asynchronous: true

            state: 'behind'

            states: [
                State {
                    name: 'behind'
                    PropertyChanges {
                        target: backdrop
                        opa: .5
                        z: 0
                        scale: .7
                    }
                },
                State {
                    name: 'front'
                    PropertyChanges {
                        target: backdrop
                        opa: 1
                        z: 100
                        scale: 1
                    }
                }
            ]

            Behavior on opacity { PropertyAnimation { duration: 500; easing.type: Easing.OutExpo } }
            Behavior on scale { PropertyAnimation { duration: 500; easing.type: Easing.OutExpo } }
        }

        ListView {
            id: list
            anchors.fill: parent
            model: XmlListModel {
                source: 'http://www.apple.com/trailers/home/xml/current.xml'
                query: '/records/movieinfo'
                XmlRole { name: 'name'; query: 'info/title/string()' }
                XmlRole { name: 'poster'; query: 'poster/location/string()' }
                XmlRole { name: 'cover'; query: 'poster/xlarge/string()' }
                XmlRole { name: 'video'; query: 'preview/large/string()' }
            }
            delegate: Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: 100
                opacity: (list.currentIndex == index)?1:.6

                Image {
                    id: img
                    source: poster
                    anchors.left: parent.left
                    anchors.top: parent.top
                    height: parent.height
                    width: height
                    fillMode: Image.PreserveAspectCrop
                    clip: true
                }
                Text {
                    id: txt
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: img.right
                    anchors.leftMargin: 50
                    color: '#167'
                    text: name
                    font.pixelSize: 40
                }
            }
            clip: true
            highlightMoveDuration: 100
            highlight: Rectangle {
                color: '#30000000'
                width: parent.width
            }
        }

        PropertyAnimation {
            id: shortRumble
            from: 70
            to: 0
            duration: 50
            property: 'rumble'
            target: move
        }

        PSMove {
            id: move
            enabled: true
            color: '#012'
            //rumble: trigger / 2

            Behavior on color { ColorAnimation { duration: 1000 } }

            onButtonPressed: {
                if (button == PSMove.Move) {
                    select.stop()
                    select.play()
                    root.state = (root.state == 'a')?'b':'a'
                } else if (button == PSMove.PS) {
                    Qt.quit()
                } else if (button == PSMove.Cross) {
                    backdrop.state = (backdrop.state == 'behind')?'front':'behind'
                }
            }

            onButtonReleased: {
                console.log('button released: ' + button)
            }

            property int initValue: 0

            onGyroChanged: {
                if (root.state == 'a' && move.trigger > 100) {
                    var dx = move.gx / 1000
                    var dz = move.gz / 1000

                    if (click.position > 10 || root.first) {

                        if (Math.abs(dx) > 1 && Math.abs(dx) > Math.abs(dz)) {
                            var diff = parseInt(Math.abs(dx))
                            if (diff < 3) {
                                diff = 1
                            }

                            var newValue = list.currentIndex + ((dx>0)?(-diff):(diff))
                            if (newValue >= 0 && newValue < list.model.count) {
                                root.first = false
                                list.currentIndex = newValue
                                click.stop()
                                click.play()
                                shortRumble.start()
                            }
                        }

                        /*if (Math.abs(dz) > 3 && Math.abs(dz) > Math.abs(dx)) {
                            root.first = false
                            click.stop()
                            click.play()
                            shortRumble.start()
                            move.initValue += 1 //= (dz>0)?(-1):(1)
                            var colors = ['#992', '#929', '#299']
                            move.color = colors[move.initValue % 3]
                        }*/
                    }
                }
            }
        }
    }

    Video {
        id: card
        anchors.fill: parent
        anchors.centerIn: parent
        source: (root.state=='b')?(list.model.get(list.currentIndex).video):('')
        playing: (root.state == 'b')

        Behavior on scale { PropertyAnimation { duration: 500; easing.type: Easing.OutExpo } }
        Behavior on opacity { PropertyAnimation { duration: 500; easing.type: Easing.OutExpo } }
        Behavior on rotation { PropertyAnimation { duration: 700; easing.type: Easing.InExpo } }

    }
}

