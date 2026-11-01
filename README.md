# irides

QML visual effects extracted from Hyprion's border shader and Hyprland's
rounded-shadow geometry. Two components, no orbit animation, no host imports —
anything that needs a compositor fact receives it as a property.

`Glaze` can either paint its local ShaderEffect or ask a running Hyprion
instance to draw the ring in the compositor. The compositor path samples the
real framebuffer, including the backdrop behind a translucent surface, and
falls back to the local shader when the `hyprion_glaze_manager_v1` global is
unavailable.

- **`Glaze`** — Hyprion's OKLab border: an inset ring and an outset band over a
  rounded surface. The inset band paints a colour or transforms the pixels
  beneath it in OKLab (lightness, hue rotation, chroma multiplier and offset,
  tint), with `strength` mixing the transform and `glazeOpacity` fading it.
- **`Shadow`** — an outside-only drop shadow with Hyprland's sharp and soft
  paths, cutting the surface out of the shadow box.
- **`Oklab`** — OKLab lightness arithmetic on QML colors: `toLab`, `fromLab`,
  `lightness`, `isLight`, and an additive `shift(color, delta)` clamped to the
  gamut. It is the colour math the other two use, exposed for token ladders.

Both shaders carry the superellipse rounding power; geometry is Qt logical
pixels.

## Use

```qml
import Irides 1.0

Item {
    // ... a rounded fill ...

    Shadow {
        anchors.centerIn: parent
        surfaceWidth: parent.width
        surfaceHeight: parent.height
        windowRadius: 6
        range: 12
        shadowPower: 3
        shadowColor: "#000000"
        shadowOpacity: 0.5
    }

    Glaze {
        anchors.fill: parent
        insetSize: 1
        radius: 6
        insetPaint: "#202020"
    }
}
```

`Glaze` spans the surface grown by `outsetSize`; with no outset it fills the
surface's box exactly. With Hyprion available, the ring is drawn by the
compositor; otherwise the inset band transforms `source` (an opaque fill) or
paints `insetPaint`. The local fallback cannot sample the backdrop outside the
surface, so the outset band stays painted there.

`Shadow` sizes itself to the surface plus `padding`; centre it on the surface,
draw it beneath the fill.

## Build

`irides` is a compiled QML module because the compositor path needs a Wayland
client binding and the window's `wl_surface`:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Add `build/qml` to `QML_IMPORT_PATH`; the module is imported as `Irides 1.0`.
The Qt Wayland client development headers are required.

The `*.frag.qsb` files are generated from the `*.frag` sources when they
change:

```sh
qsb --qt6 -o glaze.frag.qsb glaze.frag
qsb --qt6 -o shadow.frag.qsb shadow.frag
```

## Provenance

Both effects are the author's own QML ports: `Glaze` from Hyprion's
`HyprionDecoration` fragment shader (the OKLab transform and band coverage),
`Shadow` from Hyprland's rounded-shadow geometry as first ported in
5vk1-shell. No third-party source is included.

## License

BSD 3-Clause. See `LICENSE`.
