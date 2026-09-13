---
name: ping-pong
description: Drive a feature or a bug fix as a ping-pong rally — slice the work, write the failing Qt Test first (ping), add the smallest code that turns it green (pong), refactor on green, then close with an adversarial coder↔reviewer round before the verify gate. Use when implementing a feature or fixing a bug that is more than a one-line edit, or when the user asks for "ping-pong", TDD, or a red/green loop.
---

# Ping-pong

Two alternations, run one after the other:

| Loop | Partners | When |
|---|---|---|
| **Rally** (default) | failing test ↔ implementation | the whole implementation, slice by slice |
| **Adversarial round** | you ↔ a reviewer subagent | once the rally is done, before `verify` |

Skip both only for a typo, a pure doc edit, or a format-only change.

## 0. Rack up the rally — slice before you type

Write the slice list first, in a todo. Each slice is **one observable
behaviour = one test function**, small enough that one pong turns it green.
Three to seven slices is the normal shape; more than that means the task
wants splitting.

For a **bug**, slice 1 is always *reproduce it in a test* — never a fix.

## 1. Set up a fast table

```bash
python bootstrap.py compile --skip-analyze
```

Configure the tree **once** with the gate off. `test` never reconfigures — it
inherits whatever `ENABLE_ANALYZE` the build tree was configured with — so
every rally iteration is then a plain incremental build. The gate comes back
at step 5, and `verify` is what reconfigures it on.

## 2. Ping — the failing test

Write it per the **`add-test`** skill (`QTEST_GUILESS_MAIN`, mirror the source
tree, `readary_add_test(...)` in `src/tests/CMakeLists.txt`).

```bash
python bootstrap.py test -k <Suite>          # CTest name = file name minus `Test`
```

- **It must fail on an assertion, not on a compile error.** A TU that does not
  compile is not a red test — you cannot tell it from a typo. If the API does
  not exist yet, add the smallest declaration with a trivially wrong body
  (`return {};`) so the test links and fails for the right reason.
- Read the `Totals: N passed, M failed, K skipped` line. `0 passed, 0 failed`
  means the suite never ran.
- Bug slice: the test must fail **the way the report describes**. If it passes,
  you have not reproduced it — widen the input (empty list, missing column,
  ordering, locale, network failure) before writing a single line of fix.

## 3. Pong — the smallest code that turns it green

Only this slice. No speculative branches, no groundwork for slice 4.

The architecture still binds: **`add-feature`** for anything crossing layers,
**`qml-component`** under `qml/`, **`db-change`** for schema. A pong that
breaks layering is not green, it is deferred debt.

Re-run the one suite. Green → next step. Not green → fix the code, never the
test.

## 4. Clean — refactor while green

Rename, dedupe, extract, hoist literals into `Geometry`/`Styles`/`Theme`.
Re-run the suite after each refactor — it stays green or you revert. This is
also where the comments and annotations below get written.

Then serve the next slice from step 2.

If a slice needs more than about two pongs, it was too big: split it and
re-slice the rest of the list.

## Comments and annotations

### Comment only what is genuinely hard

Default is **no comment**. The code says *what*; a comment earns its place only
when a competent reader would stop and ask *why*:

- a non-obvious Qt/framework behaviour you had to work around,
- an ordering or lifetime constraint that is invisible at the call site,
- a deliberate deviation from the obvious implementation, and what it costs,
- a test that has to mirror a private detail (say which, and why).

Never comment what the next line already states, never restate a good name,
never leave a header banner over an obvious function. If a comment is the only
thing making a block readable, rename or extract instead — then delete the
comment.

[`BookSearchProxyModel.hpp:40`](../../../src/models/books/BookSearchProxyModel.hpp)
is the house exemplar: it documents a real Qt trap (the score cache is keyed by
source row and Qt does not re-run the filter on insert/remove), the symptom to
look for, and the fix — none of which is readable from the code.

### Annotate anything unfinished or propped up

A slice you deliberately left short, a shortcut, a workaround for someone
else's bug — it gets a tagged annotation **at the exact place a reader will
hit it**, never only in the chat reply or the commit message. Untagged debt is
invisible debt.

| Tag | Means |
|---|---|
| `TODO:` | work that still has to be done — say what, and what unblocks it |
| `FIXME:` | it is wrong or fragile as written, and you know it |
| `HACK:` | it works, but by the wrong means — say what the right means is |
| `NOTE:` | a real trap or constraint, nothing to do about it |

Each one says **what / how / why**, not just a noun:

```cpp
// TODO: cache is dropped whole on any insert; per-row invalidation needs the
// source model's rowsInserted/rowsRemoved wired up (see BookSearchProxyModel).
// Fine while the library is small — revisit when lists pass a few thousand rows.
```

```cpp
// HACK: re-reading the DTO after insert to pick up the AUTOINCREMENT id.
// BookTable::insert should return it; changing that signature touches every
// call site, so it is a separate change.
```

Rules:

- Never ship a bare `// TODO` with no sentence after it.
- A `FIXME` on a code path a test covers means the test is asserting wrong
  behaviour — go back to step 2 instead.
- Anything annotated goes into your final report to the user, verbatim, as
  "left undone". The annotation is the record; the report is the handover.
- The reviewer in step 5 reads annotations as claims to check, not as
  absolution — an annotated shortcut can still be the wrong call.

## 5. Adversarial round — coder ↔ reviewer

Run it for every bug fix, and for any rally that touched more than one layer
or more than about three slices.

Either use the built-in **`code-review`** skill on the working-tree diff, or
spawn a reviewer subagent with this prompt:

```
Review the working-tree diff (`git diff`, plus the untracked files in
`git status --short`) against this intent: <one line — what it should do>.

Read .claude/skills/add-feature/SKILL.md and docs/mds/architecture.md first;
layering violations are the top defect class in this repo.

Hunt for: QML reaching past a controller; a controller including another
controller instead of a cross-domain connect in AppInitializer; a Q_PROPERTY
changed without its NOTIFY emitted; drift between db/init.sql, the DTO and
BookTable; QObject parenting/ownership mistakes; blocking work on the UI
thread; untested edges (empty input, absent column, network failure + retry,
locale, Android).

Report findings only — file:line, what breaks, a concrete failing input. Do
NOT edit any file. If you find nothing, say so.
```

The reviewer **never edits** — findings only, you decide what is real. Every
finding you accept goes back through step 2 as a new red test, not a silent
patch.

Stop after two rounds. If a third round still turns up real defects, the
design is wrong — stop and put that to the user instead of patching on.

## 6. Serve the final gate

**`verify`** (format → `compile` with analysis ON → qmllint → the full
`python bootstrap.py test`) → **`sync-docs`** if the change touched a
controller API, a model/filter, the schema, the QML tree or the online-search
pipeline → **`commit`**.

## Rules that keep the rally honest

- No pong before a red ping.
- Never delete, skip or weaken a test to get green.
- One suite green is not done — the full suite runs at least once, in `verify`.
- Report what actually ran, including slices you left unplayed and why, and
  every `TODO`/`FIXME`/`HACK` you left behind.
