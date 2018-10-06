import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Rectangle {
    id: testItem
    width: 10
    height: 10
    color: Style.isDark ? Qt.rgba(0,0,0,0.75) : Qt.rgba(1,1,1,0.75)
    state: "closed"

    states: [
        State {
            name: "open"
            PropertyChanges { target: testItem; height: 200; width: 200}
        },
        State {
            name: "closed"
            PropertyChanges { target: testItem; height: 10; width: 10 }
        },
        State {
            name: "shy"
            PropertyChanges { target: testItem; height: 50; width: 50 }
        }
    ]

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            if(testItem.state == "closed") {
                testItem.state = "shy"
            }
        }
        onExited: {
            if(testItem.state == "shy") {
                testItem.state = "closed"
            }
        }
        //onWheel: {}
        onClicked: {
            testItem.state = testItem.state == "open" ? "closed" : "open"
        }
    }

    transitions: Transition {
        PropertyAnimation {
            properties: "height, width";
            easing.type: Easing.OutCubic
        }
    }
}
