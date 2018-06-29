import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Logger 1.0

Item {
    id: root

    // these are actually all in meters right now
    property var start_mm: 0
    property var end_mm: 0
    property var length_mm: end_mm - start_mm
    property var depth_mm: 0

    property var validIncrements: [0.1, 0.25, 0.5, 1, 2, 5, 10]

    property int numTicks: 10
    property int maxTicks: 10
    property var increment: 1

    property var color: Style.color
    width: textMetrics.width < maxTextMetrics.width ? textMetrics.width : maxTextMetrics.width
    Layout.maximumWidth: textMetrics.width < maxTextMetrics.width ? textMetrics.width : maxTextMetrics.width
    Layout.minimumWidth: textMetrics.width < maxTextMetrics.width ? textMetrics.width : maxTextMetrics.width

    /*
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.1; color: 'gray'}
            GradientStop { position: 0.0; color: 'black'}
        }
    }*/

    TextMetrics {
        id: maxTextMetrics
        font.bold: true
        text: "999.9"
    }

    TextMetrics {
        id: textMetrics
        font.bold: true
        text: end_mm.toFixed(1)
    }

    function getIncrement() {
        var inc;
        for (var i = 0; i < validIncrements.length; i++) {
            inc = validIncrements[i];
            if (length_mm / inc <= maxTicks) {
                break;
            }
        }
        return inc;
    }

    function recalc() {
        increment = getIncrement()
        numTicks = length_mm / increment + 1
    }

    onStart_mmChanged: {
        recalc()
    }

    onEnd_mmChanged: {
        recalc()
    }

    function nominalHeight() {
        return increment * parent.height / length_mm
    }

    function remainingHeight() {
        return parent.height - ((numTicks-1) * nominalHeight())
    }

    Component {
        id: tickMark
        RowLayout {
            anchors.right: parent.right
            height: lastIndex ? remainingHeight() : nominalHeight()
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom

            property var lastIndex: (index + 1) == numTicks

            Rectangle {
                id: tick
                width: root.width
                height: 2
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                color: root.color
                border.color: root.color
                border.width: 1
                visible: parent.height > 0
            }
        }
    }

    Component {
        id: tickLabel
        RowLayout {
            anchors.right: parent.right
            height: lastIndex ? remainingHeight() : nominalHeight()
            Layout.fillHeight: true
            width: root.width
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom

            property var lastIndex: (index + 1) == numTicks

            Text {
                id: label
                //anchors.fill:parent // this text object is not anchored
                //Layout.fillHeight: true
                Layout.alignment: Qt.AlignBottom
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                //width: parent.width
                //Layout.bottomMargin: 5
                //Layout.rightMargin: 7
                text: lastIndex ? end_mm.toFixed(1) : start_mm + (index + 1) * increment
                color: root.color
                font.bold: true
                //font.pointSize: 22
                visible: parent.height >= height

                Rectangle {
                    anchors.fill: parent
                    color: '#101010a0'
                }
            }
        }
    }

    /*
    Image {
        anchors.right: parent.right
        source: "/icons/depth_indicator.svg"
        height: 20
        width: 20
        y: (parent.height / length_mm) * depth_mm - height / 2
    }*/

    Canvas {
        id: depthPointer
        y: (parent.height / length_mm) * depth_mm - height / 2
        z: root.z + 1
        width: root.width
        height: root.width
        onWidthChanged: requestPaint()
        onPaint: {
            print(width, height)
            var ctx = getContext("2d")
            ctx.fillStyle = "red"
            ctx.beginPath()
            ctx.moveTo(width/2, height/3)
            ctx.lineTo(0, height/2)
            ctx.lineTo(width/2, height*2/3)
            ctx.fill()
        }
    }

    Column {
        anchors.fill:parent
        Repeater {
            model: numTicks
            delegate: tickMark
        }
    }

    Column {
        anchors.fill:parent
        Repeater {
            model: numTicks
            delegate: tickLabel
        }
    }
}
