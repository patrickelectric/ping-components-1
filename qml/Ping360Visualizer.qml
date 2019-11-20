import QtGraphicalEffects 1.0
import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Controls 1.4 as QC1
import QtQuick.Layouts 1.3
import QtQuick.Shapes 1.12
import Qt.labs.settings 1.0
import WaterfallPlot 1.0
import PolarPlot 1.0

import DeviceManager 1.0
import FileManager 1.0
import SettingsManager 1.0
import StyleManager 1.0
import Util 1.0

Item {
    id: root
    property alias displaySettings: displaySettings

    anchors.fill: parent
    property var ping: DeviceManager.primarySensor

    /** Some visual elements doesn't need to be refreshed in sensor sample frequency
      * Improves UI performance
      */

    Timer {
        interval: 50
        running: true
        repeat: true
        onTriggered: {
            shapeSpinner.angle = (ping.angle + 0.25)*180/200
            if(chart.visible) {
                chart.draw(ping.data, ping.range, 0)
            }
        }
    }

    Connections {
        target: ping

        /** Ping360 does not handle auto range/scale
         *  Any change in scale is a result of user input
         */
        onRangeChanged: {
            clear()
        }

        onSectorSizeChanged : {
            clear()
        }

        onDataChanged: {
            waterfall.draw(ping.data, ping.angle, 0, ping.range, ping.angular_speed, ping.sectorSize)
        }
    }

    onWidthChanged: {
        if(chart.Layout.minimumWidth === chart.width) {
            waterfall.parent.width = width - chart.width
        }
    }

    QC1.SplitView {
        orientation: Qt.Horizontal
        anchors.fill: parent

        Item {
            id: waterfallParent
            Layout.fillHeight: true
            Layout.fillWidth: true



            ShaderEffect {
                z: 10000
            id: shader
            height: Math.min(ping.sectorSize > 180 ? parent.height : parent.height*2, parent.width*scale)
            width: height
            property variant src: waterfall
            property var angle: 1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: ping.sectorSize > 180 ? parent.verticalCenter : parent.bottom

            property var shaderAngle: 0.1
            property var shaderAngleMin: 0
            property var shaderAngleWidth: 6.4
            property var shaderRadius: 1
            property var shaderRadiusMin: 1
            property var shaderRadiusWidth: 2
            //property var shaderCenter: {0, 0}
            vertexShader:
            "
uniform highp mat4 qt_Matrix;
attribute highp vec4 qt_Vertex;
attribute highp vec2 qt_MultiTexCoord0;
varying highp vec2 coord;
void main() {
    coord = qt_MultiTexCoord0;
    gl_Position = qt_Matrix * qt_Vertex;
}
            "
            fragmentShader:
                "
//The source texture that will be transformed
uniform sampler2D src;
uniform float angle;

//The texture coordinate passed in from the vertex shader (the default luxe vertex shader is suffice)
varying vec2 coord;

//x: width of the texture divided by the radius that represents the top of the image (normally screen width / radius)
//y: height of the texture divided by the radius representing the top of the image (normally screen height / radius)
//uniform vec2 sizeOverRadius;

//This is a constant to determine how far left/right on the source image we look for additional samples
//to counteract thin lines disappearing towards the center (a somewhat adjusted texel width in uv-coordinates)
//This value is 1 / (texture width * 2 PI)
//uniform float sampleOffset;

//Linear blend factor that swaps the direction the y-axis of the source is mapped onto the radius
//if the value is 1, y = 0 is at the outer edge of the circle,
//if the value is 0, y = 0 is the center of the circle
//uniform float polarFactor;

void main() {
    vec2 sizeOverRadius = vec2(2.0, 2.0);
    float sampleOffset = 0;
    float polarFactor = 1.0;
	//Make relative to center
	vec2 relPos = coord - vec2(0.5 ,0.5);

	//Adjust for screen ratio
	relPos *= sizeOverRadius;

	//Normalised polar coordinates.
	//y: radius from center
	//x: angle
	vec2 polar;

	polar.y = sqrt(relPos.x * relPos.x + relPos.y * relPos.y);

	//Any radius over 1 would go beyond the source texture size, this simply outputs black for those fragments
	if(polar.y > 1.0){
		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	polar.x = atan(relPos.y, relPos.x);

	//Normally, the angle starts from the axis going right and is counter-clockwise
	//I want the left edge of the screen image to be the line upwards,
	//so I rotate the angle half pi clockwise
	polar.x += 3.1415/2.0;

	//Normalise from angle to 0-1 range
	polar.x /= 6.28318530718;

	//Make clockwise
	//polar.x = -polar.x;

	polar.x = mod(polar.x, 1.0);

	//The xOffset fixes lines disappearing towards the center of the coordinate system
	//This happens because there's only a few pixels trying to display the whole width of the source image
	//so they 'miss' the lines. To fix this, we sample at the transformed position
	//and a bit to the left and right of it to catch anything we might miss.
	//Using 1 / radius gives us minimal offset far out from the circle,
	//and a wide offset for pixels close to the center

	float xOffset = 0.0;
	if(polar.y != 0.0){
		xOffset = 1.0 / polar.y;
	}

	//Adjusts for texture resolution
	xOffset *= sampleOffset;

	//This inverts the radius variable depending on the polarFactor
	polar.y = polar.y * polarFactor + (1.0 - polar.y) * (1.0 - polarFactor);

	//Sample at positions with a slight offset
	vec4 one = texture2D(src, vec2(polar.x - xOffset, polar.y));

	vec4 two = texture2D(src, polar);

	vec4 three = texture2D(src, vec2(polar.x + xOffset, polar.y));

	//Take the maximum of the three samples. This is not ideal, but the quickest way to choose a coloured sample over the background colour.
    /*
    vec2 dista = abs(coord - vec2(0.5, 0.5));
    if(distance(dista, vec2(0.5, 0.5)) < 0.4) {
        gl_FragColor = vec4(1, 0, 0, 1);
        return;
    }*/
	gl_FragColor = max(max(one, two), three);
}
            "
        }






            PolarPlot {
                id: waterfall
                height: Math.min(ping.sectorSize > 180 ? parent.height : parent.height*2, parent.width*scale)
                width: height
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: ping.sectorSize > 180 ? parent.verticalCenter : parent.bottom

                property var scale: ping.sectorSize >= 180 ? 1 : 0.8/Math.sin(ping.sectorSize*Math.PI/360)
                property bool verticalFlip: false
                property bool horizontalFlip: false

                transform: Rotation {
                    origin.x: waterfall.width/2
                    origin.y: waterfall.height/2
                    axis { x: waterfall.verticalFlip; y: waterfall.horizontalFlip; z: 0 }
                    angle: 180
                }

                // Spinner that shows the head angle
                Shape {
                    id: shapeSpinner
                    opacity: 1
                    anchors.centerIn: parent
                    vendorExtensionsEnabled: false
                    property var angle: 0

                    ShapePath {
                        id: shapePathSpinner
                        strokeWidth: 2
                        strokeColor: StyleManager.secondaryColor

                        PathLine {
                            x: waterfall.width*Math.cos(shapeSpinner.angle*Math.PI/180 - Math.PI/2)/2
                            y: waterfall.height*Math.sin(shapeSpinner.angle*Math.PI/180 - Math.PI/2)/2
                        }
                    }
                }

                Shape {
                    visible: waterfall.containsMouse
                    anchors.centerIn: parent
                    opacity: 0.5
                    ShapePath {
                        strokeWidth: 3
                        strokeColor: StyleManager.secondaryColor
                        startX: 0
                        startY: 0
                        //TODO: This need to be updated in sensor integration
                        PathLine {
                            property real angle: -Math.atan2(waterfall.mousePos.x - waterfall.width/2, waterfall.mousePos.y - waterfall.height/2) + Math.PI/2
                            x: waterfall.width*Math.cos(angle)/2
                            y: waterfall.height*Math.sin(angle)/2
                        }
                    }
                }
            }

            Text {
                id: mouseReadout
                visible: waterfall.containsMouse
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 5
                font.bold: true
                font.family: "Arial"
                font.pointSize: 15
                text: (waterfall.mouseSampleDistance*SettingsManager.distanceUnits['distanceScalar']).toFixed(2) + SettingsManager.distanceUnits['distance']
                color: StyleManager.secondaryColor

                Text {
                    id: mouseConfidenceText
                    visible: typeof(waterfall.mouseSampleAngle) == "number"
                    anchors.top: parent.bottom
                    anchors.margins: 5
                    font.bold: true
                    font.family: "Arial"
                    font.pointSize: 15
                    text: calcAngleFromFlips(waterfall.mouseSampleAngle) + "º"
                    color: StyleManager.secondaryColor

                    function calcAngleFromFlips(angle) {
                        var value = waterfall.mouseSampleAngle
                        if(waterfall.verticalFlip && waterfall.horizontalFlip) {
                            value = (360 + 270 - value) % 360
                            return transformValue(value)
                        }

                        if(waterfall.verticalFlip) {
                            value = (360 + 180 - value) % 360
                            return transformValue(value)
                        }

                        if(waterfall.horizontalFlip) {
                            value = 360 - value
                            return transformValue(value)
                        }

                        return transformValue(value)
                    }
                }
            }

            PolarGrid {
                id: polarGrid
                anchors.fill: waterfall
                angle: ping.sectorSize
                maxDistance: waterfall.maxDistance
            }
        }

        Chart {
            id: chart
            Layout.fillHeight: true
            Layout.maximumWidth: 250
            Layout.preferredWidth: 100
            Layout.minimumWidth: 75
            // By default for ping360 the chart starts from the bottom
            flip: true
        }

        Settings {
            property alias chartWidth: chart.width
        }
    }

    function transformValue(value, precision) {
        return typeof(value) == "number" ? value.toFixed(precision) : value + ' '
    }

    function captureVisualizer() {
        waterfall.grabToImage(function(result) {
            print("Grab waterfall image callback.")
            print(FileManager.createFileName(FileManager.Pictures))
            result.saveToFile(FileManager.createFileName(FileManager.Pictures))
        })
    }

    function clear() {
        waterfall.clear()
    }

    function handleShortcut(key) {
        return false
    }

    Component {
        id: displaySettings
        GridLayout {
            id: settingsLayout
            anchors.fill: parent
            columns: 5
            rowSpacing: 5
            columnSpacing: 5
            property bool isFullCircle: true

            // Sensor connections should be done inside component
            // Components are local '.qml' descriptions, so it's not possible get outside connections
            Connections {
                target: ping
                onSectorSizeChanged: {
                    settingsLayout.isFullCircle = ping.sectorSize == 360
                }
            }

            CheckBox {
                id: horizontalFlipChB
                text: "Head down"
                checked: false
                Layout.columnSpan: 5
                Layout.fillWidth: true
                onCheckStateChanged: {
                    waterfall.horizontalFlip = checkState
                }
            }

            CheckBox {
                id: flipAScan
                text: "Flip A-Scan"
                checked: false
                Layout.columnSpan: 5
                Layout.fillWidth: true
                onCheckStateChanged: {
                    chart.flip = !checkState
                }
            }

            CheckBox {
                id: smoothDataChB
                text: "Smooth Data"
                checked: true
                Layout.columnSpan: 5
                Layout.fillWidth: true
                onCheckStateChanged: {
                    waterfall.smooth = checkState
                }
            }

            CheckBox {
                id: antialiasingDataChB
                text: "Antialiasing"
                property bool isMac: Util.isMac()
                checked: !isMac
                visible: !isMac || SettingsManager.debugMode
                Layout.columnSpan: 5
                Layout.fillWidth: true
                onVisibleChanged: {
                    if(!visible && isMac) {
                        checked = false
                    }
                }
                onCheckStateChanged: {
                    waterfall.antialiasing = checkState
                }
            }

            CheckBox {
                id: removeAScanChB
                text: "A-Scan"
                checked: true
                Layout.columnSpan:  5
                Layout.fillWidth: true
                onCheckStateChanged: {
                    chart.visible = checkState
                }
            }

            Label {
                text: "Plot Theme:"
            }

            PingComboBox {
                id: plotThemeCB
                Layout.columnSpan: 4
                Layout.fillWidth: true
                Layout.minimumWidth: 200
                model: waterfall.themes
                onCurrentTextChanged: waterfall.theme = currentText
            }

            Settings {
                category: "Ping360Visualizer"
                property alias flipAScanState: flipAScan.checkState
                property alias plotThemeIndex: plotThemeCB.currentIndex
                property alias removeAScanState:removeAScanChB.checkState
                property alias smoothDataState: smoothDataChB.checkState
                property alias waterfallAntialiasingData: antialiasingDataChB.checkState
            }
        }
    }
}
