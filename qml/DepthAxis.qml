import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Logger 1.0

Item {
    id: root

    property var start_mm: 0
    property var end_mm: 0
    property var length_mm: end_mm - start_mm

    property var validIncrements: [0.1, 0.25, 0.5, 1, 2, 5, 10]

    property int numTicks: 10
    property int maxTicks: 10
    property var increment: 1

    property var color: "white"

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
        console.log(start_mm, end_mm, length_mm, increment)

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
                width: 15
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
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom

            property var lastIndex: (index + 1) == numTicks

            Text {
                id: label
                //anchors.fill:parent // this text object is not anchored
                //Layout.fillHeight: true
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                Layout.bottomMargin: 5
                Layout.rightMargin: 7
                text: lastIndex ? end_mm.toFixed(1) : start_mm + (index + 1) * increment
                color: root.color
                font.bold: true
                font.pointSize: 22
                visible: parent.height >= height
            }
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
