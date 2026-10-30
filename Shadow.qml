import QtQuick

// An outside-only drop shadow following Hyprland's rounded shadow geometry:
// sharp is a flat, antialiased rounded rectangle that keeps the colour's own
// alpha; soft applies the compositor's range falloff with `shadowPower`. Both
// cut the surface out, so a translucent surface under the shadow does not
// reveal it through its own fill.
//
// The item sizes itself to the surface grown by `padding` on every side and is
// meant to be centred on the surface. Draw it beneath the fill; the fill's own
// box and radius are the cutout.
Item {
    id: root

    // The surface the shadow wraps, in logical pixels.
    property real surfaceWidth: 0
    property real surfaceHeight: 0
    // The shadow's own corner radius and the surface cutout's radius.
    property real radius: 0
    property real windowRadius: 0

    // Hyprland's shadow keys.
    property real range: 0
    property real shadowPower: 3
    property real roundingPower: 2
    property color shadowColor: "black"
    property real shadowOpacity: 1
    property bool sharp: false
    property real scale: 1
    property real offsetX: 0
    property real offsetY: 0

    // Enough room for the falloff and the offset; the shadow box only grows.
    readonly property real padding: Math.ceil(range + Math.abs(offsetX) + Math.abs(offsetY)) + 2
    implicitWidth: surfaceWidth + 2 * padding
    implicitHeight: surfaceHeight + 2 * padding

    readonly property real scaleClamped: Math.max(0, Math.min(1, scale))
    readonly property real shadowHalfW: (surfaceWidth / 2 + range) * scaleClamped
    readonly property real shadowHalfH: (surfaceHeight / 2 + range) * scaleClamped
    readonly property real shadowCenterX: padding + surfaceWidth / 2 + offsetX
    readonly property real shadowCenterY: padding + surfaceHeight / 2 + offsetY

    ShaderEffect {
        anchors.fill: parent
        visible: root.range > 0 || root.shadowOpacity > 0

        fragmentShader: Qt.resolvedUrl("shadow.frag.qsb")
        property vector2d itemSize: Qt.vector2d(width, height)
        property vector2d winMin: Qt.vector2d(root.padding, root.padding)
        property vector2d winMax: Qt.vector2d(root.padding + root.surfaceWidth,
            root.padding + root.surfaceHeight)
        property vector2d shMin: Qt.vector2d(root.shadowCenterX - root.shadowHalfW,
            root.shadowCenterY - root.shadowHalfH)
        property vector2d shMax: Qt.vector2d(root.shadowCenterX + root.shadowHalfW,
            root.shadowCenterY + root.shadowHalfH)
        property color shadowPaint: root.shadowColor
        property real windowRadius: root.windowRadius
        property real radius: root.radius
        property real range: root.range
        property real shadowPower: root.shadowPower
        property real roundingPower: root.roundingPower
        property real sharp: root.sharp ? 1 : 0
        property real shadowOpacity: root.shadowOpacity
    }
}
