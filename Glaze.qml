import QtQuick

// Two OKLab glaze bands around a rounded surface: an inset ring hugging the
// surface's edge from the inside, and an outset band just outside it. The
// geometry, coverage ramps, and transform are Hyprion's border shader,
// without its pulse and orbit.
//
// The inset band either paints one colour or, when `source` is set, lifts the
// pixels beneath it and transforms them in OKLab: additive lightness, hue
// rotation, a chroma multiplier and offset, and a tint that replaces chroma at
// the sample's own lightness. `strength` mixes the transform in, so values
// above one overshoot and clip at the gamut edge. The outset band is always
// painted: in Hyprland it samples the backdrop, which in a shell lives behind
// the window and is out of QML's reach.
//
// The item spans the surface grown by `outsetSize` on every side, both
// centred, and side by side the bands tile the surface's edge without a seam.
ShaderEffect {
    id: root

    // Band widths. Zero disables either band.
    property real insetSize: 0
    property real outsetSize: 0
    // The surface's own corner radius, and the compositor's rounding power.
    property real radius: 0
    property real roundingPower: 2

    // The inset band's painted colour, used when `source` is null.
    property color insetPaint: "transparent"
    // The pixels to lift for the glaze; an opaque surface's fill. Over a
    // translucent surface the backdrop is out of reach, so paint instead.
    property var source: null
    // How much of the band carries the glaze, 0..1.
    property real glazeOpacity: 1

    // OKLab transform, matching Hyprion's active/inactive profile keys.
    property real lightness: 0
    property real saturation: 1
    property real chroma: 0
    property real hue: 0
    property real strength: 1
    property color tint: "transparent"
    property real tintStrength: 0

    // The outset band, always painted.
    property color outsetPaint: "transparent"

    readonly property real surfaceWidth: Math.max(0, width - 2 * outsetSize)
    readonly property real surfaceHeight: Math.max(0, height - 2 * outsetSize)

    readonly property bool painting: insetSize > 0 || outsetSize > 0
    visible: painting

    fragmentShader: Qt.resolvedUrl("glaze.frag.qsb")
    property vector2d itemSize: Qt.vector2d(width, height)
    property vector2d surfaceSize: Qt.vector2d(surfaceWidth, surfaceHeight)
    property real flatPaint: source === null ? 1 : 0
}
