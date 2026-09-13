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
    addBookPage/
      AddBookPage.qml              Form for a hand-added book; submits to BookController.addCustomBook
      CoverPicker.qml              Tap-to-pick cover slot with a dashed placeholder (QtQuick.Dialogs FileDialog)
      PdfPicker.qml                Tap-to-pick PDF row; parses through BookController and reports the numbers back
      GenrePicker.qml              Preset genre chips plus a free-text slot behind "Other"
    bookDetailPage/
      BookDetailPage.qml           Detail page bound to BookController.currentBookData
      BookDetailHeader.qml         Back arrow + "⋮" overflow button + cover + title/author/meta + inline rating
      ReadingProgressCard.qml      Progress block + action button OR embedded ReadingProgressTimer
      ReadingProgressTimer.qml     Stopwatch (Phase enum) + end-session form; persists via BookController
      RatingsCard.qml              Two RatingTile halves separated by a divider
      RatingTile.qml               Optional label + value/total + optional StarRating row
      BookDetailSummary.qml        Composes BookDetailHeader/ReadingProgressCard/RatingsCard/description/actions for the book overview block
      CharacterRow.qml             Avatar + name/role; PressableSurface base
      CharactersListDelegate.qml   Wrapper around CharacterRow used as the inner ListView delegate (anchored side margins)
      CharactersListHeader.qml     "Characters" title + "+ Add" — used as ListView.header
      ReadingHistorySection.qml    ListView of finished reading sessions as a timeline (header + rows + paging toggle); collapses when the book was never read
      ReadingHistoryListHeader.qml "Reading history" title — used as ListView.header
      ReadingHistoryDelegate.qml   Wrapper around ReadingHistoryRow used as the inner ListView delegate (anchored side margins)
      ReadingHistoryRow.qml        Timeline dot on a rail + stamp / page range + delta / duration / delete tile
      BookGenreTags.qml            Flow of TagPill for genres, sits below the characters ListView
  components/
    AppSearchField.qml             Pill-shaped text field with magnifier glyph
    ActionMenu.qml                 Dropdown of actions behind a "⋮" button; items are plain objects supplied by the caller
    FieldLabel.qml                 Caption above a form input
    FormField.qml                  FieldLabel + filled input box; single-line or multiline, optional digits-only
    DashedOutline.qml              Canvas-painted dashed rounded outline for an empty slot
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
    IconGlyph.qml                  Emoji/glyph or image renderer; collapses to 0×0 when empty; `tinted` colorizes the emoji SVG
    ProgressBar.qml                Linear bar (track + fill); height defaults to Styles.progressBar.sm
    ProgressRing.qml               Canvas-based circular progress
    SectionHeader.qml              "Title + trailing badge" header
    PagedListToggle.qml            Show less / Show more pair for any model exposing canHide/canLoadMore — used as ListView.footer; centered, hidden when nothing to do
  theme/
    Theme.qml                      Singleton: forwards palette colors + global tokens (starColor, starColorEmpty)
    palettes/
      PinkPalette.qml, BluePalette.qml, YellowPalette.qml, PurplePalette.qml
  utils/
    Geometry.qml                   Singleton: spacing/radius/size sub-specs
    Styles.qml                     Singleton: font sizes, weights, elevation, opacity, duration, progressBar
    Format.qml                     Singleton: locale-aware duration (h/m/s) and entry stamp formatting
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
   `NavigationController.currentPage = NavigationController.CategoryListPage`.

### `CategoryListPage.qml`

Vertical list page. Header (back arrow + title), subtitle "%n book(s)",
`AppSearchField` bound to the shared `searchModel`, then `ListView` whose
delegate `BookListRow` calls `BookController.openBook(model.bookId)` on
tap.

### `AddBookPage.qml`

Reached from the "+ Add your own" link on the search page, next to the result
count. Header (back arrow + title), then a `Flickable` holding a `CoverPicker`,
seven `FormField`s (title, author, year, publisher, page count, ISBN,
description), a `PdfPicker`, a `GenrePicker` and the submit `PrimaryButton`.

- Nothing is bound two-way. `_submit()` reads each field's `value` into a
  `QVariantMap` keyed by SQL column names and hands it to
  `BookController.addCustomBook`, which validates. On failure the page toasts
  `BookController.errorMessage`; on success the controller opens the new book,
  which swaps this page out — so the detail page *is* the confirmation and no
  QML runs after the call. See
  [controllers.md](controllers.md#bookcontroller) for the field keys.
- The page holds no reset path: it lives behind `Main.qml`'s page `Loader`, so
  leaving and coming back builds a fresh, empty form.
- `CoverPicker` stores a local file URL straight into `coverUrl`; `Image`
  loads it the same as a remote cover, and an empty pick falls back to the
  usual 📖 placeholder.
- `GenrePicker` chips show translated labels but contribute untranslated ids
  (`"Fantasy"`, `"Sci-Fi"`, …) — rows in the `genres` table are
  language-independent keys shared with imported books, so a translated label
  must never reach storage. "Other" reveals a `FormField` whose raw text is
  appended as-is.
- `PdfPicker` calls `BookController.stagePdf()` on pick and re-emits the result
  as `parsed(info)`; the page pushes those values into the title, author and
  page-count and ISBN fields through `FormField.setValue`, and the rendered first page
  into `CoverPicker.previewUrl`. That is **display only** — the controller
  applies the same override again when it stores the book, so the rule holds
  even if QML never showed it. The trash tile emits `cleared()` and drops the
  staged file via `clearStagedPdf()`, which the page uses to clear the preview.
- `CoverPicker` keeps the two apart: `imageUrl` is what the user picked and what
  the form submits, `previewUrl` is the PDF's first page and wins for display —
  matching the controller, which lets the PDF override a hand-picked cover on a
  custom book. The preview arrives as a `data:` url because the stored cover has
  no file until the book has an isbn to be named after.

### `BookDetailPage.qml`

Bound to `BookController.currentBookData` (a `bl::qmltypes::BookDTOObject`
Q_GADGET — wrapper that inherits from the plain `BookDTO` data struct
and adds Q_PROPERTY aliases plus the detail-only `genres` field). Caches
the gadget once into `_book` and child sections read its members
directly. (Characters are NOT in `_book` — they're owned by a separate
model, see below.)

The page is an outer `Flickable` (single scroll surface) holding a
`ColumnLayout` with four top-level sections: a `BookDetailSummary`
(everything about the book), a `ReadingHistorySection`, a focused
`ListView` for the character rows, and a `BookGenreTags` `Flow`. The
character `ListView`'s only job is the rows — `header` is a
`CharactersListHeader` ("Characters" title + "+ Add"), `footer` is a
`PagedListToggle` (Show less / Show more), `delegate` is
`CharactersListDelegate`. It runs with
`interactive: false` and `implicitHeight: contentHeight` so the outer
`Flickable` owns the scroll; the C++-side model already caps `rowCount`
at `_visibleCount` (5 by default), so only the buffered rows
instantiate.

The character model is `BookController.charactersModel`
([`BookCharactersModel`](../../src/models/books/BookCharactersModel.hpp)) —
a `QAbstractListModel` that pulls all rows for the current book in one
SQL query and exposes them in pages of 5 via internal `_visibleCount`.
The `PagedListToggle` buttons are bound to `canHide` / `canLoadMore`
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
1.5. Three `ActionButton`s: Want to read / Want to buy as a pair, then
     Statistics spanning both columns. The PDF actions and the book
     deletion are **not** here — they live in the overflow menu below,
     because each of them only applies to some books and a grid of
     half-disabled tiles reads worse than a short menu.

**The overflow menu.** `BookDetailHeader` carries a "⋮" `TouchTarget` at the
right of its back-arrow row, shown only when `hasMenu` (the page passes
`_menuActions.length > 0`, so there is never an empty menu to open). The page
owns the `ActionMenu` next to its dialogs and builds `_menuActions` from the
book's state:

| Entry | Present when |
|-------|--------------|
| Open PDF | the book has a `pdfPath` |
| Attach PDF / Replace PDF | `pdfSource !== PdfSource.Server` — a catalog-supplied file is the book's, not the user's |
| Remove PDF | `pdfSource === PdfSource.User` |
| Delete book | `isCustom`; drawn under a divider |

`_runMenuAction(id)` routes the ids: open goes straight to
`BookController.openCurrentBookPdf()` (toasting on failure), attach opens a
`FileDialog`, and both removals go through a `ConfirmDialog` first. Deleting
the book also stops the reading timer, so a running session is not flushed
back to the cache under a dead ISBN, then calls
`BookController.deleteCurrentBook()` and `NavigationController.goBack()`.

**2. `ReadingHistorySection`** — the `reading_sessions` journal for the
current book, newest first, bound to `BookController.readingHistoryModel`
([`ReadingHistoryModel`](../../src/models/books/ReadingHistoryModel.hpp)):
- `header`: `ReadingHistoryListHeader` — "Reading history" title.
- `delegate`: `ReadingHistoryDelegate` over `ReadingHistoryRow` — a timeline
  dot on a vertical rail, the `d MMM, HH:mm` stamp, `p. from → to` with a
  `· +N` pages delta (hidden when the session gained no pages), the duration,
  and a delete tile. Stamps and durations come from the `Format` singleton
  (`qml/utils/Format.qml`).
- `spacing: 0` — deliberate: each row paints its own slice of the rail, so any
  gap would break the line. The rail's ends are trimmed by
  `railAbove: index > 0` / `railBelow: index < count - 1`.
- `footer`: `PagedListToggle` — same five-at-a-time paging as the characters
  list, backed by the same `canHide` / `canLoadMore` contract.
- The whole section is `visible: totalCount > 0`, so a book that was never
  read shows nothing (a `ColumnLayout` drops invisible items entirely).
- The delete tile only asks: the row emits `deleteRequested(sessionId)`, the
  section forwards it, and `BookDetailPage` confirms through a `ConfirmDialog`
  before calling `BookController.deleteReadingSession`. The id travels as a
  `string` the whole way — a QML signal cannot declare `qint64` and an `int`
  would clip it.

**3. Characters `ListView`** (focused — only the rows live here):
- `header`: `CharactersListHeader` ("Characters" title + "+ Add" button).
- `delegate`: `CharactersListDelegate` over `BookController.charactersModel`
  (the model exposes the first 5 rows, the rest stay buffered in C++).
- `footer`: `PagedListToggle` — centered "Show less" (visible while
  `canHide`) and "Show more" (visible while `canLoadMore`); each click
  shrinks/expands the model's visible window by one page.
- `interactive: false`, `implicitHeight: contentHeight` so the page's
  outer `Flickable` owns the scroll.

**4. `BookGenreTags`** — `Flow` of `TagPill`, hidden when empty.

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
  `BookController.currentBookIsbn` into `_bookIsbnAtCreation` and pulls saved
  state via `BookController.takeReadingSession(_bookIsbnAtCreation)`. A
  second `Timer { interval: 5000 }` flushes `(seconds, phase)` to
  `ReadingSessionCache` while running, so a non-graceful shutdown loses at
  most 5s. `Component.onDestruction` does a final save (or clear when
  Stopped). See [database.md → ReadingSessionCache](database.md#readingsessioncache-qsettings).
  **`_bookIsbnAtCreation` is a `string`, not a number**: a 13-digit ISBN
  passed through a `qint64` QML invokable parameter silently arrives as `0`
  on the C++ side (reads out of a `qint64` Q_PROPERTY are fine — only the
  invokable-argument direction breaks), which previously made restore a
  no-op. Capture and pass the ISBN as a string; `BookController` parses it
  back to `qint64`.
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
| `AppSearchField` | `text` is read/write; emits `textEdited` and `accepted`. `accepted` fires on Enter, on the IME's Search key (`EnterKey.type`) and on a tap of the magnifier glyph; `submit()` drops focus first so a pending Android pre-edit is committed and the keyboard closes. `busy: bool` swaps the magnifier for a `LoadingSpinner` |
| `BookListRow` | Cover + name/author + "type · year"; uses `coverSource: model.coverUrl ?? ""` |
| `ReadingBookCard` | Cover spans the card width; rounded top corners via `MultiEffect` mask |
| `BottomNavBar` | 5 items; clicking sets `NavigationController.currentPage` |
| `IconGlyph` | Image when `source` is set, else `glyph` text in `Segoe UI Emoji`. Collapses to 0×0 implicit size when both empty. |
| `ProgressBar` | Linear; `progress` 0…1; `trackColor` / `progressColor`. Default height `Styles.progressBar.sm`; callers may override (`md` in `ReadingBookCard`, `lg` in `ReadingProgressCard`). |
| `ProgressRing` | Canvas-based circular progress; configurable `strokeWidth` |
| `LoadingSpinner` | Indeterminate spinner — a `ProgressRing` arc under a `RotationAnimator`. `running` also drives `visible`; size via `diameter` |
| `CategoryRow` | Card row with rounded icon box, title, subtitle, chevron |
| `SectionHeader` | Title with optional trailing accent text (count badge) |
| `StarRating` | `value: real` rounded to int via `Math.round`; `total: int`. Renders ★/☆ in `Theme.starColor` / `Theme.starColorEmpty`. |
| `RatingTile` | `label`, `value`, `total`, `decimals: 0`, `showStars: false`. Empty `label` hides the label row; `value <= 0` shows "Not rated". |
| `TagPill` | Pill with `label`; `Theme.primarySoft` background |
| `FieldLabel` | A `Text` preset for form captions — the shared look of every label in `AddBookPage` |
| `FormField` | `FieldLabel` + a filled, rounded input box. One-way by design: the host form reads `value` on submit rather than binding both ways, so there is no write-back loop. `setValue(text)` is the escape hatch for a field the form fills in for the user (a PDF's page count overwriting what was typed). `multiline` swaps the `TextField` for a scrollable `TextArea` and grows the box to `Geometry.size.formTextAreaHeight`; `numeric` installs an `IntValidator` plus `Qt.ImhDigitsOnly`. An empty `label` hides the caption row |
| `DashedOutline` | Dashed rounded outline on a `Canvas` — `Rectangle.border` can only draw a solid line. `strokeColor` / `strokeWidth` / `radius` / `dashLength` / `dashGap`, each repainting on change the way `ProgressRing` does |
| `ActionMenu` | Styled `Popup` holding a column of tappable rows, one per entry in `actions: [{ id, label, glyph, separatorBefore }]`; emits `triggered(actionId)` and closes itself. The caller builds the list from state, so an action that does not apply is **absent rather than disabled**; `separatorBefore` draws a divider to group the destructive ones off (the palettes have no danger colour to use instead) |
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
  `dialogPrimaryWidth: 160`, `spinner: 28`, `spinnerStroke: 3`.

Wrapping nested QtObjects in named components keeps qmlls' type info accurate
(it lets the language server resolve `Geometry.size.foo` as `int` rather
than untyped `QtObject`).

`Styles` (singleton): `fontSize`, `fontWeight`, `elevation`, `opacity`,
`duration`, `progressBar`. `duration` carries `fast/normal/slow` plus `spin`
(one `LoadingSpinner` revolution). The `elevation` spec now exposes
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
| [qml/pages/bookDetailPage/ReadingHistorySection.qml](../../qml/pages/bookDetailPage/ReadingHistorySection.qml) | Finished-session journal for the current book |
| [qml/components/](../../qml/components/) | Reusable widgets — base chassis (`SurfaceCard`, `PressableSurface`, `PaddedCard`, `TouchTarget`), buttons (`PrimaryButton`, `SecondaryButton`, `ActionButton`, `IconButton`, `TextButton`), atoms (`StarRating`, `TagPill`, `IconGlyph`, `ProgressBar`, `ProgressRing`) and rows |
| [qml/theme/Theme.qml](../../qml/theme/Theme.qml) | Theme singleton |
| [qml/utils/Geometry.qml](../../qml/utils/Geometry.qml) | Spacing / radius / size tokens |
| [qml/utils/Styles.qml](../../qml/utils/Styles.qml) | Font / opacity / duration / progressBar / elevation tokens |
| [qml/utils/Format.qml](../../qml/utils/Format.qml) | `duration()` / `stamp()` display helpers |
