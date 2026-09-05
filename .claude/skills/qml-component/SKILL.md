---
name: qml-component
description: Create or change QML in Readary to house style — build on the SurfaceCard/PressableSurface/PaddedCard/TouchTarget chassis, take every number and colour from the Geometry/Styles/Theme singletons, keep property access qualified for qmllint, wrap strings in qsTr, and reach data only through controllers. Use for any work under qml/.
---

# QML work

Module URI is `Library`; every `.qml` under `qml/` is globbed into the module
automatically. Full component inventory:
[`docs/mds/qml.md`](../../../docs/mds/qml.md) — read it before adding a
component, the one you want probably exists.

## Non-negotiables

1. **No raw numbers or colours.** Spacing/radius/sizes come from `Geometry`,
   font sizes/weights/elevation/opacity/durations from `Styles`, every colour
   from `Theme`. If a token is missing, add it to the singleton — do not
   inline the literal.
2. **Compose the existing chassis**, don't restart from `Rectangle`:
   - `SurfaceCard` — card background + shadow (`shadowOffset` / `shadowBlur`).
   - `PressableSurface : SurfaceCard` — adds `MouseArea`, `signal clicked`,
     `readonly property alias pressed`. Base for every button.
   - `PaddedCard : SurfaceCard` — `contentPadding` + `default property alias
     content`; implicit size derived from the **first** child. Use it for cards
     wrapping a single Layout; buttons stay on `PressableSurface` because they
     need the MouseArea on the chassis.
   - `TouchTarget` — invisible padded hit area for text-sized links.
3. **Data comes from controllers only** — `BookController`, `NavigationController`,
   `SettingsController`, `GlobalBookSearchController`, `BookFilterController`.
   Never a model or service directly. API in
   [`docs/mds/controllers.md`](../../../docs/mds/controllers.md).
4. **Qualify property access**: give the root an `id: root` and write
   `root.iconColor`, not bare `iconColor`, inside children. Unqualified access
   is what `all_qmllint` fails on.
5. **All user-visible text through `qsTr()`** — then run
   `python bootstrap.py translate` and commit the regenerated `.ts`/`.qm`.
6. **`qmlformat` owns formatting.** Run `python bootstrap.py format`; don't
   hand-align.

## Shape of a component

```qml
import QtQuick
import Library

PressableSurface {
    id: root

    property string iconGlyph: ""
    property color iconColor: Theme.primaryContent
    property int diameter: Geometry.size.avatarSm

    implicitWidth: diameter
    implicitHeight: diameter
    radius: width / 2
    restColor: Theme.primary
    shadowOffset: Styles.elevation.subtleOffset
    opacity: root.pressed ? Styles.opacity.pressed : 1.0

    IconGlyph {
        anchors.centerIn: parent
        glyph: root.iconGlyph
        color: root.iconColor
        size: Geometry.size.iconMd
    }
}
```

Order: `id` → custom properties → geometry/appearance overrides → signals →
child items. Prefer `implicitWidth/implicitHeight` over fixed sizes so parents
can lay out.

## Things that bite

- **New singleton** (`pragma Singleton`) also needs a
  `set_source_files_properties(qml/... PROPERTIES QT_QML_SINGLETON_TYPE TRUE)`
  line in the root `CMakeLists.txt`. Plain components need nothing.
- **`qint64` in a signal parameter breaks the component** — "Invalid signal
  parameter type". QML signals take basic QML types only; use `var` or `string`
  at the boundary.
- **Emoji/icons** go through `IconGlyph` + the vendored Twemoji SVGs
  (`assets/emoji/`) — bundled colour-emoji fonts do not render on Android.
  `tinted: true` colorizes the SVG.
- **Long lists**: reuse `PagedListToggle` as `ListView.footer` for any model
  exposing `canHide` / `canLoadMore`, and the header/delegate split used by the
  characters and reading-history sections.

## Verify

```bash
python bootstrap.py format
./venv/Scripts/cmake.exe --build build/debug --target all_qmllint
python bootstrap.py run          # eyeball it; -d android to deploy
```

Then finish with the **`verify`** skill.
