
## Version 1.2 (2021-05-29)

- Updated to MSDFgen 1.9.
- Multiple fonts or font sizes can now be compiled into a single atlas.
- Added `-yorigin` option to choose if Y-coordinates increase from bottom to top or from top to bottom.
- Added `-coloringstrategy` option to select MSDF edge coloring heuristic.
- Shadron preview now properly loads floating-point image outputs in full range mode.

## Version 1.1 (2020-10-18)

- Updated to MSDFgen 1.8.
- Glyph geometry is now preprocessed by Skia to resolve irregularities which were previously unsupported and caused artifacts.
    - The scanline pass and overlapping contour mode is made obsolete by this step and has been disabled by default. The preprocess step can be disabled by the new `-nopreprocess` switch and the former enabled by `-scanline` and `-overlap` respectively.
    - The project can be built without the Skia library, forgoing the geometry preprocessing feature. This is controlled by the macro definition `MSDFGEN_USE_SKIA`.
- Glyphs can now also be loaded by glyph index rather than Unicode values. In the standalone version, a set of glyphs can be passed by `-glyphset` in place of `-charset`.
- Glyphs not present in the font should now be correctly skipped instead of producing a placeholder symbol.
- Added `-threads` argument to set the number of concurrent threads used during distance field generation.

### Version 1.0.1 (2020-03-09)

- Updated to MSDFgen 1.7.1.

## Version 1.0 (2020-03-08)

- Initial release.
