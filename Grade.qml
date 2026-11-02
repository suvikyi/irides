import QtQuick

// A sampled colour grade over a rounded area. The inset and outset bands are
// geometry; the grade itself is direction-agnostic and can darken, lighten,
// rotate hue, change chroma, or colorize.
//
// With Hyprion available the compositor samples the real framebuffer,
// including the backdrop behind a translucent surface, and grades in OKLab on
// SDR or Jzazbz on HDR. Request blur separately through
// ext-background-effect-v1; the grade samples the blurred result. The
// ShaderEffect is the portable SDR fallback and can only transform an explicit
// `source` texture or paint a flat band.
Item {
    id: root

    // Area bits. The compositor advertises which it implements.
    readonly property int areaInset: 1
    readonly property int areaOutset: 2
    readonly property int areaFill: 4
    property int area: areaInset

    // Grade mode. Auto keeps outset source-over and inset/fill in-place.
    readonly property int modeAuto: -1
    readonly property int modeInPlace: 1
    readonly property int modeOver: 2
    property int mode: modeAuto

    // Band widths. Zero disables either band.
    property real insetSize: 0
    property real outsetSize: 0
    // The surface's own corner radius, and the compositor's rounding power.
    property real radius: 0
    property real roundingPower: 2

    // Fallback painting inputs, used only when the compositor path is off.
    property color insetPaint: "transparent"
    property var source: null
    property color outsetPaint: "transparent"
    // How much of the grade covers what is beneath it, 0..1.
    property real gradeOpacity: 1

    // OKLab grade transform.
    property real lightness: 0
    property real saturation: 1
    property real chroma: 0
    property real hue: 0
    property var colorizeColors: []
    property real colorizeAngle: 0
    property real colorizeStrength: 0
    readonly property int colorizeStopCount: Math.min(colorizeColors.length, 10)
    readonly property color colorizeStop0: colorizeStopCount > 0 ? colorizeColors[0] : "transparent"
    readonly property color colorizeStop1: colorizeStopCount > 1 ? colorizeColors[1] : colorizeStop0
    readonly property color colorizeStop2: colorizeStopCount > 2 ? colorizeColors[2] : colorizeStop1
    readonly property color colorizeStop3: colorizeStopCount > 3 ? colorizeColors[3] : colorizeStop2
    readonly property color colorizeStop4: colorizeStopCount > 4 ? colorizeColors[4] : colorizeStop3
    readonly property color colorizeStop5: colorizeStopCount > 5 ? colorizeColors[5] : colorizeStop4
    readonly property color colorizeStop6: colorizeStopCount > 6 ? colorizeColors[6] : colorizeStop5
    readonly property color colorizeStop7: colorizeStopCount > 7 ? colorizeColors[7] : colorizeStop6
    readonly property color colorizeStop8: colorizeStopCount > 8 ? colorizeColors[8] : colorizeStop7
    readonly property color colorizeStop9: colorizeStopCount > 9 ? colorizeColors[9] : colorizeStop8

    readonly property real surfaceWidth: Math.max(0, width - 2 * outsetSize)
    readonly property real surfaceHeight: Math.max(0, height - 2 * outsetSize)

    readonly property bool painting: insetSize > 0 || outsetSize > 0

    // Ask Hyprion to grade the area in the compositor when the protocol and
    // the requested modes are available. That path samples the real
    // framebuffer, including the backdrop behind a translucent surface.
    HyprionGrade {
        id: compositor
        item: root
        area: root.area
        mode: root.mode
        enabled: root.painting && root.visible
        insetSize: root.insetSize
        outsetSize: root.outsetSize
        radius: root.radius
        roundingPower: root.roundingPower
        lightness: root.lightness
        saturation: root.saturation
        chroma: root.chroma
        hue: root.hue
        opacity: root.gradeOpacity
        colorizeColors: root.colorizeColors
        colorizeAngle: root.colorizeAngle
        colorizeStrength: root.colorizeStrength
    }

    readonly property bool compositorActive: compositor.active
    property vector2d itemSize: Qt.vector2d(width, height)
    property vector2d surfaceSize: Qt.vector2d(surfaceWidth, surfaceHeight)
    property real flatPaint: source === null ? 1 : 0

    // Consumers own this item's visibility. Keep fallback selection on a
    // private child so an explicit visible binding cannot paint both paths.
    ShaderEffect {
        anchors.fill: parent
        visible: root.painting && !root.compositorActive
        fragmentShader: Qt.resolvedUrl("grade.frag.qsb")
        property var source: root.source
        property vector2d itemSize: root.itemSize
        property vector2d surfaceSize: root.surfaceSize
        property real flatPaint: root.flatPaint
        property real insetSize: root.insetSize
        property real outsetSize: root.outsetSize
        property real radius: root.radius
        property real roundingPower: root.roundingPower
        property color insetPaint: root.insetPaint
        property color outsetPaint: root.outsetPaint
        property real gradeOpacity: root.gradeOpacity
        property real lightness: root.lightness
        property real saturation: root.saturation
        property real chroma: root.chroma
        property real hue: root.hue
        property real colorizeStrength: root.colorizeStrength
        property int colorizeStopCount: root.colorizeStopCount
        property real colorizeAngle: root.colorizeAngle
        property color colorizeStop0: root.colorizeStop0
        property color colorizeStop1: root.colorizeStop1
        property color colorizeStop2: root.colorizeStop2
        property color colorizeStop3: root.colorizeStop3
        property color colorizeStop4: root.colorizeStop4
        property color colorizeStop5: root.colorizeStop5
        property color colorizeStop6: root.colorizeStop6
        property color colorizeStop7: root.colorizeStop7
        property color colorizeStop8: root.colorizeStop8
        property color colorizeStop9: root.colorizeStop9
    }
}
