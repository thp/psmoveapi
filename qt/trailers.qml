/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
 **/

import Qt 4.7

import QtMultimediaKit 1.1

import com.thpinfo.psmove 1.0

Rectangle {
    id: root
    color: state=='a'?Qt.darker(bg.color, 1.5):'black'
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

        ListView {
            id: list
            anchors.fill: parent
            model: XmlListModel {
                source: 'http://www.apple.com/trailers/home/xml/current.xml'
                query: '/records/movieinfo'
                XmlRole { name: 'name'; query: 'info/title/string()' }
                XmlRole { name: 'poster'; query: 'poster/location/string()' }
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
                    text: name
                    font.pixelSize: 40
                }
            }
            clip: true
            highlightMoveDuration: 100
            highlight: Rectangle {
                color: Qt.lighter(root.color, 2)
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
            color: '#992'
            //rumble: trigger / 2

            Behavior on color { ColorAnimation { duration: 1000 } }

            onButtonPressed: {
                if (button == PSMove.Move) {
                    select.stop()
                    select.play()
                    root.state = (root.state == 'a')?'b':'a'
                } else if (button == PSMove.PS) {
                    Qt.quit()
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

                        if (Math.abs(dz) > 3 && Math.abs(dz) > Math.abs(dx)) {
                            root.first = false
                            click.stop()
                            click.play()
                            shortRumble.start()
                            move.initValue += 1 //= (dz>0)?(-1):(1)
                            var colors = ['#992', '#929', '#299']
                            move.color = colors[move.initValue % 3]
                        }
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

