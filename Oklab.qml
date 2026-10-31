pragma Singleton
import QtQuick

// OKLab lightness arithmetic for tokens derived from the theme surface. The
// shell's own shaders use the same space; keeping the conversion here makes
// light themes fall out of the data instead of a flag.
QtObject {
    function _linear(channel) {
        return channel <= 0.04045
            ? channel / 12.92
            : Math.pow((channel + 0.055) / 1.055, 2.4)
    }

    function _encode(channel) {
        channel = Math.max(0, Math.min(1, channel))
        return channel <= 0.0031308
            ? channel * 12.92
            : 1.055 * Math.pow(channel, 1 / 2.4) - 0.055
    }

    function toLab(color) {
        var r = _linear(color.r)
        var g = _linear(color.g)
        var b = _linear(color.b)
        var l = Math.pow(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b, 1 / 3)
        var m = Math.pow(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b, 1 / 3)
        var s = Math.pow(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b, 1 / 3)
        return {
            L: 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            a: 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            b: 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s,
        }
    }

    function fromLab(lab) {
        var l = Math.pow(lab.L + 0.3963377774 * lab.a + 0.2158037573 * lab.b, 3)
        var m = Math.pow(lab.L - 0.1055613458 * lab.a - 0.0638541728 * lab.b, 3)
        var s = Math.pow(lab.L - 0.0894841775 * lab.a - 1.2914855480 * lab.b, 3)
        return Qt.rgba(
            _encode(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
            _encode(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
            _encode(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s),
            1
        )
    }

    function lightness(color) {
        return toLab(color).L
    }

    function isLight(color) {
        return lightness(color) > 0.5
    }

    // Additive OKLab L shift, clamped to the gamut. Chroma is left alone; the
    // tokens that use this are near-neutral surfaces.
    function shift(color, delta) {
        var lab = toLab(color)
        lab.L = Math.max(0, Math.min(1, lab.L + delta))
        return fromLab(lab)
    }
}
