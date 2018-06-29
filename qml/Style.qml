// Style.qml
pragma Singleton
import QtQuick 2.0
import QtQuick.Controls.Material 2.1

QtObject {
    property color linen: Qt.rgba(0.98, 0.94, 0.9)
    property color black: Qt.rgba(1, 1, 1)
    property color textColor: linen
    property color iconColor: textColor
    property color color: isDark ? black : linen
    property color inverseColor: invertColor(color)
    property string dark: "dark"
    property string light: "light"
    property bool isDark: true
    property int theme: Material.Dark

    function useLightStyle() {
        theme = Material.Light
        textColor = black
        isDark = false
        inverseColor = Qt.lighter(textColor, 1.2)
    }

    function useDarkStyle() {
        theme = Material.Dark
        textColor = linen
        isDark = true
        inverseColor = Qt.darker(textColor, 1.2)
    }

    function invertColor(color) {
        var rcolor = Qt.rgba(0, 0, 0, 1)
        rcolor.r = color.r > 0.5 ? color.r - 0.5 : color.r + 0.5;
        rcolor.g = color.g > 0.5 ? color.g - 0.5 : color.g + 0.5;
        rcolor.b = color.b > 0.5 ? color.b - 0.5 : color.b + 0.5;
        print('>>>>>>>> =D', rcolor, color, typeof(color))
        return rcolor
    }
}