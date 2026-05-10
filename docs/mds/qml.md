# QML structure

QML module URI: `Library`. All `.qml` files under `qml/` are bundled by
`qt_add_qml_module`, with three QML singletons (`Geometry`, `Styles`,
`Theme`) declared via `QT_QML_SINGLETON_TYPE TRUE` in CMake.
[`BookStatus`](../../src/services/BookStatus.hpp) and
[`ReadingPhase`](../../src/services/ReadingPhase.hpp) are also visible from
QML as Q_GADGET enum-namespaces (registered via `QML_ELEMENT`).

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
    settingsPage/
      SettingsPage.qml             Language picker (and future user-preference rows) — bound to SettingsController.languageModel
    bookDetailPage/
      BookDetailPage.qml           Detail page bound to BookController.currentBookData
      BookDetailHeader.qml         Back arrow + cover + title/author/meta + inline rating
      ReadingProgressCard.qml      Progress block + action button OR embedded ReadingProgressTimer
      ReadingProgressTimer.qml     Stopwatch (Phase enum) + end-session form; persists via BookController
      RatingsCard.qml              Two RatingTile halves separated by a divider
      RatingTile.qml               Optional label + value/total + optional StarRating row
      BookDetailSummary.qml        Composes BookDetailHeader/ReadingProgressCard/RatingsCard/description/actions for the book overview block
      CharacterRow.qml             Avatar + name/role; PressableSurface base
      CharactersListDelegate.qml   Wrapper around CharacterRow used as the inner ListView delegate (anchored side margins)
      CharactersListHeader.qml     "Characters" title + "+ Add" — used as ListView.header
      CharactersToggle.qml         Show less / Show more pair — used as ListView.footer; centered, hidden when nothing to do
      BookGenreTags.qml            Flow of TagPill for genres, sits below the characters ListView
  components/
    AppSearchField.qml             Pill-shaped text field with magnifier glyph
    SurfaceCard.qml                Rectangle + radius.lg + Theme.surface + MultiEffect shadow
    PressableSurface.qml           SurfaceCard + MouseArea + signal clicked + readonly pressed alias
    PaddedCard.qml                 SurfaceCard with contentPadding and auto implicit size
    PrimaryButton.qml              PressableSurface in primary fill, pill, opacity press feedback
    SecondaryButton.qml            PressableSurface in primarySoft fill, pill, primary content
    ActionButton.qml               PressableSurface with icon + label, surface fill, primarySoft tint on press
    IconButton.qml                 Circular PressableSurface (radius=width/2); pressedColor defaults to restColor
    TextButton.qml                 TouchTarget with a single Text — used for Cancel / "+ Add" style links
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
Used for the back-arrow in `BookDetailHeader` and the "+ Add" link inline
in `BookDetailPage`'s ListView header. Replaces the older "negative-margin
MouseArea" pattern.

### `PrimaryButton : PressableSurface`

Pill-shaped action button: `restColor: Theme.primary`, pill radius, content
opacity dimmed on press (so the shadow stays full). Used inside
`ReadingProgressCard` for the Continue/Read again/Start reading button and
inside `ReadingProgressTimer` for the Save action.

### `SecondaryButton : PressableSurface`

Pill-shaped, but with `restColor: Theme.primarySoft` and primary-coloured
content. Visual middle ground between `PrimaryButton` and `ActionButton`.
Used for «End session and save progress» in `ReadingProgressTimer`.

### `ActionButton : PressableSurface`

Surface-coloured button with icon + label: `pressedColor:
Theme.primarySoft`. Used for PDF / Statistics / Favorite on the detail page.

### `IconButton : PressableSurface`

Circular icon-only button (`radius: width / 2`). `pressedColor` defaults to
`restColor` (binding) — press feedback flows through `opacity` so caller
overrides only `restColor`/`iconColor`. Used for the play/pause and reset
controls inside `ReadingProgressTimer`.

### `TextButton : TouchTarget`

A `Text` inside a `TouchTarget` with `padding: Geometry.spacing.md`. Caller
sets `label`, `labelColor`, `labelSize`, `labelWeight`. Used for muted
"Cancel" actions and primary-coloured "+ Add"-style links — anywhere a
button without chrome is needed.

## Pages

### `Main.qml`

```qml
ApplicationWindow {
    Loader {
        id: pageLoader
        anchors.fill: parent
        source: NavigationController.currentPagePath
    }

    Component {
        id: bottomNavBarComponent
        BottomNavBar { Layout.fillWidth: true }
    }
    footer: Loader {
        id: bottomNavBarLoader
        sourceComponent: bottomNavBarComponent
    }

    readonly property var _retranslatableLoaders: [pageLoader, bottomNavBarLoader]
    Connections {
        target: SettingsController.languageModel
        function onCurrentChanged() {
            for (let i = 0; i < root._retranslatableLoaders.length; ++i) {
                root._retranslatableLoaders[i].active = false;
                root._retranslatableLoaders[i].active = true;
            }
        }
    }
}
```

Two things going on here:

- `pageLoader` swaps in whichever page corresponds to
  `NavigationController.currentPage` — this is the standard router pattern.
- `bottomNavBarLoader` wraps `BottomNavBar` in its own `Loader`, even though
  the nav bar is static, so the same `active = false → true` toggle that
  refreshes the page works for the footer too.

Why the toggle: `QQmlEngine::retranslate()` is called from the C++ side
whenever `LanguageModel.currentChanged` fires (queued), and it refreshes
*direct* `text: qsTr("…")` bindings. It does **not** reliably re-evaluate
`qsTr()` calls nested inside object literals stored in `var` properties
(e.g. `BottomNavBar._navItems`), and it sometimes leaves the active page
behind a `Loader` stale until you navigate away and back. Toggling
`active` releases the loaded item and re-instantiates it from the same
`source` / `sourceComponent` — every `qsTr()` re-evaluates against the
*already-installed* new `QTranslator`, so the UI snaps to the new
language without further plumbing.

To wire a new translatable subtree, add its `Loader` to
`_retranslatableLoaders` — the handler picks it up automatically.

Full pipeline (Python `translate` command, `LanguageModel`,
`SettingsController` façade, runtime hooks): see [i18n.md](i18n.md).

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

Bound to `BookController.currentBookData` (a `bl::qmltypes::BookDTOObject`
Q_GADGET — wrapper that inherits from the plain `BookDTO` data struct
and adds Q_PROPERTY aliases plus the detail-only `genres` field). Caches
the gadget once into `_book` and child sections read its members
directly. (Characters are NOT in `_book` — they're owned by a separate
model, see below.)

The page is an outer `Flickable` (single scroll surface) holding a
`ColumnLayout` with three top-level sections: a `BookDetailSummary`
(everything about the book), a focused `ListView` for the character
rows, and a `BookGenreTags` `Flow`. The character `ListView`'s only
job is the rows — `header` is a `CharactersListHeader` ("Characters"
title + "+ Add"), `footer` is a `CharactersToggle` (Show less / Show
more), `delegate` is `CharactersListDelegate`. It runs with
`interactive: false` and `implicitHeight: contentHeight` so the outer
`Flickable` owns the scroll; the C++-side model already caps `rowCount`
at `_visibleCount` (5 by default), so only the buffered rows
instantiate.

The character model is `BookController.charactersModel`
([`BookCharactersModel`](../../src/models/books/BookCharactersModel.hpp)) —
a `QAbstractListModel` that pulls all rows for the current book in one
SQL query and exposes them in pages of 5 via internal `_visibleCount`.
The `CharactersToggle` buttons are bound to `canHide` / `canLoadMore`
and call `hide()` / `loadMore()`; each click does the matching
`beginInsertRows`/`endInsertRows` (or remove pair) on the changed page —
no SQL, no model reset, no scroll jump.

Layout (top → bottom, all inside the page-level Flickable's ColumnLayout):

**1. `BookDetailSummary`** (single component, contents in order):
1.1. `BookDetailHeader` — back arrow, cover (`SurfaceCard` with hero shadow),
     title/author/meta (`%n page(s)` plural), optional star rating row.
1.2. `ReadingProgressCard` — progress block (Progress label, % indicator,
     `ProgressBar`, "X of N pages") visible only when `pagesTotal > 0`.
     Below it, **either** the `PrimaryButton` (visible when timer is
     `Stopped`) whose label is computed by `BookDetailSummary` from
     `status` — `BookStatus.Finished` / `InProgress` / default →
     `Read again` / `Continue reading` / `Start reading` — **or** the
     `ReadingProgressTimer` (visible when timer is active). The two
     never co-exist.
1.3. `RatingsCard` — two `RatingTile`s separated by a HiDPI-aware divider
     (`Math.max(1, Math.ceil(Screen.devicePixelRatio))`). Left: user's
     rating with stars (decimals: 0). Right: global rating without stars
     (decimals: 1). Each tile shows "Not rated" when `value <= 0`.
1.4. Description — header + body text, hidden when description is empty.
1.5. Three `ActionButton`s (PDF / Statistics / Favorite).

**2. Characters `ListView`** (focused — only the rows live here):
- `header`: `CharactersListHeader` ("Characters" title + "+ Add" button).
- `delegate`: `CharactersListDelegate` over `BookController.charactersModel`
  (the model exposes the first 5 rows, the rest stay buffered in C++).
- `footer`: `CharactersToggle` — centered "Show less" (visible while
  `canHide`) and "Show more" (visible while `canLoadMore`); each click
  shrinks/expands the model's visible window by one page.
- `interactive: false`, `implicitHeight: contentHeight` so the page's
  outer `Flickable` owns the scroll.

**3. `BookGenreTags`** — `Flow` of `TagPill`, hidden when empty.

Status semantics: see [`BookStatus`](../../src/services/BookStatus.hpp).
The page references `BookStatus.Finished` / `InProgress` directly — the
component (`ReadingProgressCard`) takes `actionLabel: string` and stays
ignorant of status meaning.

### `ReadingProgressTimer.qml`

Stopwatch over the current reading session. State machine driven by the
[`ReadingPhase`](../../src/services/ReadingPhase.hpp) Q_GADGET enum:
`Stopped` → idle (timer hidden, action button shown), `Running` → ticking,
`Paused` → frozen but visible.

Key behaviours:

- **Tick loop** — internal `Timer { interval: 1000; running: phase ===
  Running }` increments `seconds`.
- **Persistence** — on `Component.onCompleted` the timer captures
  `BookController.currentBookId` into `_bookIdAtCreation` and pulls saved
  state via `BookController.takeReadingSession(_bookIdAtCreation)`. A
  second `Timer { interval: 5000 }` flushes `(seconds, phase)` to
  `ReadingSessionCache` while running, so a non-graceful shutdown loses at
  most 5s. `Component.onDestruction` does a final save (or clear when
  Stopped). See [database.md → ReadingSessionCache](database.md#readingsessioncache-qsettings).
- **Confirm form** — tapping "End session and save progress" opens an
  inline form (`Where did you stop?` + page-number `TextField` + Cancel
  / Save). Open auto-pauses if currently Running and remembers
  `_phaseBeforeConfirm`; Cancel restores it; Save emits
  `endSessionConfirmed(pageNumber, durationSeconds)` and resets to
  Stopped.
- **Wiring on save** — `ReadingProgressCard.onEndSessionConfirmed` calls
  `BookController.updateReadingProgress(pageNumber, durationSeconds)`
  which writes `books.pagesRead` and inserts a row in `reading_sessions`
  (see [controllers.md → Reading-timer flow](controllers.md#reading-timer-flow)).

The component knows nothing about specific bookIds at the API level — the
QML side captures one at creation and uses it consistently. Even if
`BookController.currentBookId` shifts mid-life (future feature: jump
between books from within the detail page), the timer keeps writing under
its original id.

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
  `pillButtonHeight: 52`, `characterRowHeight: 64`,
  `dialogPrimaryWidth: 160`.

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
| [qml/Main.qml](../../qml/Main.qml) | Window + page Loader + footer Loader + language-driven `active` toggle |
| [qml/pages/mainPage/MainPage.qml](../../qml/pages/mainPage/MainPage.qml) | Home page |
| [qml/pages/categoryListPage/CategoryListPage.qml](../../qml/pages/categoryListPage/CategoryListPage.qml) | Per-category list |
| [qml/pages/settingsPage/SettingsPage.qml](../../qml/pages/settingsPage/SettingsPage.qml) | Language picker bound to `SettingsController.languageModel` |
| [qml/pages/bookDetailPage/BookDetailPage.qml](../../qml/pages/bookDetailPage/BookDetailPage.qml) | Book detail page |
| [qml/pages/bookDetailPage/ReadingProgressTimer.qml](../../qml/pages/bookDetailPage/ReadingProgressTimer.qml) | Stopwatch + end-session form, persists via `BookController` |
| [qml/components/](../../qml/components/) | Reusable widgets — base chassis (`SurfaceCard`, `PressableSurface`, `PaddedCard`, `TouchTarget`), buttons (`PrimaryButton`, `SecondaryButton`, `ActionButton`, `IconButton`, `TextButton`), atoms (`StarRating`, `TagPill`, `IconGlyph`, `ProgressBar`, `ProgressRing`) and rows |
| [qml/theme/Theme.qml](../../qml/theme/Theme.qml) | Theme singleton |
| [qml/utils/Geometry.qml](../../qml/utils/Geometry.qml) | Spacing / radius / size tokens |
| [qml/utils/Styles.qml](../../qml/utils/Styles.qml) | Font / opacity / duration / progressBar / elevation tokens |
