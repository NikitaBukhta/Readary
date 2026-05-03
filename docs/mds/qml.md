# QML structure

QML module URI: `Library`. All `.qml` files under `qml/` are bundled by
`qt_add_qml_module`, with three QML singletons (`Geometry`, `Styles`,
`Theme`) declared via `QT_QML_SINGLETON_TYPE TRUE` in CMake.
[`BookStatus`](../../src/services/BookStatus.hpp) is also visible from QML
as a Q_GADGET enum-namespace (registered via `QML_ELEMENT`).

## Layout

```
qml/
  Main.qml                         Window root, page Loader
  pages/
    mainPage/
      MainPage.qml                 Home/dashboard
      WelcomeHeader.qml            Greeting block
      GoalCard.qml                 Monthly-goal card with progress ring
      CurrentlyReadingSection.qml  Horizontal list of "InProgress" books
      ReadingBookCard.qml          Card delegate used in the section above
      CategoriesSection.qml        Three category rows
    categoryListPage/
      CategoryListPage.qml         Vertical list per category, with search
    bookDetailPage/
      BookDetailPage.qml           Detail page bound to BookController.currentBookData
      BookDetailHeader.qml         Back arrow + cover + title/author/meta + inline rating
      ReadingProgressCard.qml      Progress block (hidden if totalPages=0) + PrimaryButton
      RatingsCard.qml              Two RatingTile halves separated by a divider
      RatingTile.qml               Optional label + value/total + optional StarRating row
      CharactersSection.qml        Header (title + "+ Add" link) + Repeater of CharacterRow
      CharacterRow.qml             Avatar + name/role; PressableSurface base
  components/
    AppSearchField.qml             Pill-shaped text field with magnifier glyph
    SurfaceCard.qml                Rectangle + radius.lg + Theme.surface + MultiEffect shadow
    PressableSurface.qml           SurfaceCard + MouseArea + signal clicked + readonly pressed alias
    PaddedCard.qml                 SurfaceCard with contentPadding and auto implicit size
    PrimaryButton.qml              PressableSurface in primary fill, pill shape, opacity press feedback
    ActionButton.qml               PressableSurface with icon + label, surface fill, primarySoft tint on press
    TouchTarget.qml                Item wrapping content with positive padding + click area
    StarRating.qml                 Row of ★/☆ glyphs with rounded fill against value
    TagPill.qml                    Pill-shaped genre/tag label
    BookListRow.qml                Vertical-list book row (cover + meta)
    BottomNavBar.qml               5-item nav bar driven by NavigationController
    BottomNavItem.qml              Single nav item
    CategoryRow.qml                "Want to read 4 books >" row
    IconGlyph.qml                  Emoji/glyph or image renderer; collapses to 0×0 when empty
    ProgressBar.qml                Linear bar (track + fill); height defaults to Styles.progressBar.sm
    ProgressRing.qml               Canvas-based circular progress
    SectionHeader.qml              "Title + trailing badge" header
  theme/
    Theme.qml                      Singleton: forwards palette colors + global tokens (starColor, starColorEmpty)
    palettes/
      PinkPalette.qml, BluePalette.qml, YellowPalette.qml, PurplePalette.qml
  utils/
    Geometry.qml                   Singleton: spacing/radius/size sub-specs
    Styles.qml                     Singleton: font sizes, weights, elevation, opacity, duration, progressBar
```

## Base components

These are the chassis used to build every card / button on the detail page
(and elsewhere). Once you understand them the page-specific files are short.

### `SurfaceCard`

```qml
Rectangle {
    radius: Geometry.radius.lg
    color: Theme.surface
    layer.effect: MultiEffect { /* shadow with overridable offset/blur */ }
}
```

Two override knobs: `shadowOffset` (default `Styles.elevation.cardOffset`)
and `shadowBlur` (default `Styles.elevation.cardBlur`). Use
`Styles.elevation.subtleOffset/Blur` for low cards (`ActionButton`,
`CharacterRow`), `heroOffset/Blur` for the cover.

### `PressableSurface : SurfaceCard`

Adds `MouseArea` + `signal clicked` + `readonly property alias pressed:
pressArea.pressed`. Press feedback via swapping `restColor` ↔ `pressedColor`
on the chassis. The `pressed` alias lets subclasses (like `PrimaryButton`)
build their own feedback (e.g. content opacity).

### `PaddedCard : SurfaceCard`

Adds `property real contentPadding: Geometry.spacing.xl` and uses an inner
`Item { anchors.fill: parent; anchors.margins: contentPadding }` exposed via
`default property alias content`. Auto-computes the card's `implicitWidth /
implicitHeight` from the **first** child's implicit size + `2 *
contentPadding`. Safe with `anchors.fill: parent` on the child because
`Layout.implicitHeight` is derived from Layout's children, not from
Layout's actual height.

Use this for cards that wrap a single Layout (RatingsCard,
ReadingProgressCard). For chassis used by buttons (PressableSurface descendants)
stay on `SurfaceCard` — they need MouseArea on the chassis, not inside the
content `Item`.

### `TouchTarget`

```qml
Item {
    default property alias content: container.children
    property int padding: Geometry.spacing.sm
    signal clicked
    // implicit size = first-child implicit + 2 * padding
    // MouseArea fills the whole expanded area
}
```

Wraps a small icon/text in a click area larger than the icon itself.
Used for the back-arrow in `BookDetailHeader` and the "+ Add" link in
`CharactersSection`. Replaces the older "negative-margin MouseArea"
pattern.

### `PrimaryButton : PressableSurface`

Pill-shaped action button: `restColor: Theme.primary`, pill radius, content
opacity dimmed on press (so the shadow stays full). Used inside
`ReadingProgressCard` for the Continue/Read again/Start reading button.

### `ActionButton : PressableSurface`

Surface-coloured button with icon + label: `pressedColor:
Theme.primarySoft`. Used for PDF / Statistics / Favorite on the detail page.

## Pages

### `Main.qml`

```qml
ApplicationWindow {
    Loader {
        anchors.fill: parent
        source: NavigationController.currentPagePath
    }
    footer: BottomNavBar {}
}
```

The `Loader` swaps in whichever page corresponds to
`NavigationController.currentPage`.

### `MainPage.qml`

Vertical scroll. Layout (top → bottom):

1. `WelcomeHeader` — greeting + app title.
2. `AppSearchField` — bound two-way to `BookController.searchModel.searchQuery`.
3. `GoalCard` — monthly-goal widget (a card with ring on the right).
4. `CurrentlyReadingSection` — bound to
   `BookController.getSortFilterProxyForKind(BookController.InProgress)`.
   Horizontal list of `ReadingBookCard`. Tapping a card calls
   `BookController.openBook(model.bookId)`.
5. `CategoriesSection` — three categories. On click: sets
   `BookController.activeKind = categoryId` and
   `NavigationController.currentPage = NavigationController.CATEGORY_LIST_PAGE`.

### `CategoryListPage.qml`

Vertical list page. Header (back arrow + title), subtitle "%n book(s)",
`AppSearchField` bound to the shared `searchModel`, then `ListView` whose
delegate `BookListRow` calls `BookController.openBook(model.bookId)` on
tap.

### `BookDetailPage.qml`

Bound to `BookController.currentBookData`. Caches the QVariantMap once into
`_book` and derives 13 small `_xxx` properties through `?? defaults` so
each child component reads typed values directly.

Layout (top → bottom):

1. `BookDetailHeader` — back arrow, cover (`SurfaceCard` with hero shadow),
   title/author/meta (`%n page(s)` plural), optional star rating row.
2. `ReadingProgressCard` — progress block (Progress label, % indicator,
   `ProgressBar`, "X of N pages") visible only when `pagesTotal > 0`. Below
   it the `PrimaryButton` whose label is computed at the page level by
   switching on `BookStatus.Finished` / `InProgress` / default → `Read again`
   / `Continue reading` / `Start reading`.
3. `RatingsCard` — two `RatingTile`s separated by a HiDPI-aware divider
   (`Math.max(1, Math.ceil(Screen.devicePixelRatio))`). Left: user's rating
   with stars (decimals: 0). Right: global rating without stars (decimals: 1
   so 9.0 renders as "9.0", not "9"). Each tile shows "Not rated" when
   `value <= 0`.
4. Description — header + body text, hidden when description is empty.
5. Three `ActionButton`s (PDF / Statistics / Favorite).
6. `CharactersSection` — title + "+ Add" link (via `TouchTarget`); Repeater
   of `CharacterRow`. Signal `characterClicked(int characterId)`.
7. `Flow` of `TagPill` for genres, hidden when empty.

Status semantics: see [`BookStatus`](../../src/services/BookStatus.hpp).
The page references `BookStatus.Finished` / `InProgress` directly — the
component (`ReadingProgressCard`) takes `actionLabel: string` and stays
ignorant of status meaning.

## Components

| Component | Notes |
|-----------|-------|
| `AppSearchField` | `text` is read/write; emits `textEdited` and `accepted` |
| `BookListRow` | Cover + name/author + "type · year"; uses `coverSource: model.coverUrl ?? ""` |
| `ReadingBookCard` | Cover spans the card width; rounded top corners via `MultiEffect` mask |
| `BottomNavBar` | 5 items; clicking sets `NavigationController.currentPage` |
| `IconGlyph` | Image when `source` is set, else `glyph` text in `Segoe UI Emoji`. Collapses to 0×0 implicit size when both empty. |
| `ProgressBar` | Linear; `progress` 0…1; `trackColor` / `progressColor`. Default height `Styles.progressBar.sm`; callers may override (`md` in `ReadingBookCard`, `lg` in `ReadingProgressCard`). |
| `ProgressRing` | Canvas-based circular progress; configurable `strokeWidth` |
| `CategoryRow` | Card row with rounded icon box, title, subtitle, chevron |
| `SectionHeader` | Title with optional trailing accent text (count badge) |
| `StarRating` | `value: real` rounded to int via `Math.round`; `total: int`. Renders ★/☆ in `Theme.starColor` / `Theme.starColorEmpty`. |
| `RatingTile` | `label`, `value`, `total`, `decimals: 0`, `showStars: false`. Empty `label` hides the label row; `value <= 0` shows "Not rated". |
| `TagPill` | Pill with `label`; `Theme.primarySoft` background |
| `CharacterRow` | `PressableSurface`; reserves `avatarSource: url` for future avatars (currently the IconGlyph 👥 placeholder shows when source is empty) |

## Theme and tokens

`Theme` (singleton): forwards palette colors (`background`, `surface`,
`primary`, `primarySoft`, `textPrimary`, `textSecondary`, `textMuted`,
`divider`, `shadow`, `searchBackground`, `ringTrack`, …). Two non-palette
tokens: `starColor` (#F59E0B amber) and `starColorEmpty` (35% alpha amber)
— same across all four palettes (Pink/Blue/Yellow/Purple).

`Geometry` (singleton, named-component sub-specs):
- `WindowSpec`, `SpacingSpec`, `RadiusSpec`, `SizeSpec`.
- New `SizeSpec` tokens: `iconHuge: 36`, `bookDetailCoverWidth: 100`,
  `bookDetailCoverHeight: 140`, `actionButtonHeight: 56`,
  `pillButtonHeight: 52`, `characterRowHeight: 64`.

Wrapping nested QtObjects in named components keeps qmlls' type info accurate
(it lets the language server resolve `Geometry.size.foo` as `int` rather
than untyped `QtObject`).

`Styles` (singleton): `fontSize`, `fontWeight`, `elevation`, `opacity`,
`duration`, `progressBar`. The `elevation` spec now exposes
`subtleOffset/Blur`, `cardOffset/Blur`, `heroOffset/Blur` (used by
`SurfaceCard` callers to pick a shadow level). `progressBar` exposes
`sm: 4`, `md: 5`, `lg: 6`.

## Conventions

- Every reusable component has explicit `signal`s rather than reaching into
  parent state.
- Every meaningful element gets an `id`, even when not externally referenced
  — they serve as documentation labels (so future maintainers see what each
  block IS at a glance).
- Two-way bindings to controller properties: explicit `text: ...` plus
  `onTextEdited: ...`. QML doesn't do automatic two-way binding.
- Layouts use `RowLayout`/`ColumnLayout` from `QtQuick.Layouts`. `Layout.*`
  attached properties (`fillWidth`, `alignment`, `*Margin`) propagate sizing
  through the tree.
- `pragma ComponentBehavior: Bound` on files that reference outer ids from
  inline components (Repeater delegates etc.).
- QML signal parameter types are limited to the basic QML types — use `int`
  (or `var` for 64-bit precision), **not** `qint64`. Past mistake noted in
  the project's auto-memory.

## File map

| File | Purpose |
|------|---------|
| [qml/Main.qml](../../qml/Main.qml) | Window + page Loader |
| [qml/pages/mainPage/MainPage.qml](../../qml/pages/mainPage/MainPage.qml) | Home page |
| [qml/pages/categoryListPage/CategoryListPage.qml](../../qml/pages/categoryListPage/CategoryListPage.qml) | Per-category list |
| [qml/pages/bookDetailPage/BookDetailPage.qml](../../qml/pages/bookDetailPage/BookDetailPage.qml) | Book detail page |
| [qml/components/](../../qml/components/) | Reusable widgets |
| [qml/theme/Theme.qml](../../qml/theme/Theme.qml) | Theme singleton |
| [qml/utils/Geometry.qml](../../qml/utils/Geometry.qml) | Spacing / radius / size tokens |
| [qml/utils/Styles.qml](../../qml/utils/Styles.qml) | Font / opacity / duration / progressBar / elevation tokens |
