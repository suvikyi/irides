import QtQuick

// A sampled OKLab colour grade over a rounded area. The inset and outset
// bands are geometry; the grade itself is direction-agnostic and can darken,
// lighten, rotate hue, change chroma, or tint.
//
// With Hyprion available the compositor samples the real framebuffer,
// including the backdrop behind a translucent surface. The ShaderEffect is
// the portable fallback and can only transform an explicit `source` texture
// or paint a flat band.
ShaderEffect {
    id: root

    // v1 supports the inset area and the fill source. The protocol defines
    // outset, fill-area, backdrop and blur modes for later primitives; the
    // compositor advertises what it implements.
    readonly property int areaInset: 1
    property int area: areaInset
    // `source` below is the local ShaderEffect input; sourceMode is what the
    // compositor samples.
    readonly property int sourceFill: 1
    property int sourceMode: sourceFill

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
    property real strength: 1
    property color tint: "transparent"
    property real tintStrength: 0

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
        sourceMode: root.sourceMode
        enabled: root.painting
        insetSize: root.insetSize
        outsetSize: root.outsetSize
        radius: root.radius
        roundingPower: root.roundingPower
        lightness: root.lightness
        saturation: root.saturation
        chroma: root.chroma
        hue: root.hue
        strength: root.strength
        opacity: root.gradeOpacity
        tint: root.tint
        tintStrength: root.tintStrength
    }

    readonly property bool compositorActive: compositor.active
    visible: painting && !compositorActive

    fragmentShader: Qt.resolvedUrl("grade.frag.qsb")
    property vector2d itemSize: Qt.vector2d(width, height)
    property vector2d surfaceSize: Qt.vector2d(surfaceWidth, surfaceHeight)
    property real flatPaint: source === null ? 1 : 0
}
