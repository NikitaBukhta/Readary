---
name: commit
description: Stage and commit Readary work in the repo's own style — verify first, review the diff, split unrelated changes, write the one-line "Implemented/Fixed/Refactored X" subject this history uses, and keep generated .ts/.qm and build artefacts straight. Use when the user asks to commit, and before opening a PR.
---

# Committing

## 1. Never commit a red gate

Run the **`verify`** skill first: format → `compile` (analysis ON) → qmllint →
tests. If the user asks to commit something known-broken, say so in the commit
message rather than pretending otherwise.

## 2. Look at what you are about to commit

```bash
git status --short
git diff --stat
git diff                    # actually read it
```

Watch for:

- **Debug leftovers** — stray `qDebug()`, commented-out code, `--skip-analyze`
  habits baked into a script.
- **`secrets/api_keys.env`** — never commit it. Check `.gitignore` covers any
  new secret file you introduced.
- **`build/`, `venv/`, `*.log`, `aqtinstall.log`** — build output stays out.
- **Translations** — if you touched `tr()`/`qsTr()`, the regenerated
  `translations/library_{en,ru,uk}.{ts,qm}` belong **in the same commit** as
  the strings. Committing strings without them ships untranslated UI.
- **Renames** — stage with `git mv` / `git add -A` so Git records `R`, not
  delete+add.

## 3. Split unrelated work

This history mixes concerns in single commits; prefer not to. One commit per
coherent change — feature, fix, refactor, docs — staged with `git add <paths>`
rather than `git add -A` when the tree has more than one thing going on.

## 4. Message style (match the history)

Single-line, capitalised, past-tense-or-imperative subject naming the outcome;
no type prefix, no scope, no body unless it earns one:

```
Implemented reading-history timeline on the book detail page
Fixed the bug with restoring the book reading timer
Refactored code: fixed clang warning and enhanced with google best practices
Added --all flag to bootstrap and compile all projects with one command
```

Add a body only for *why* something non-obvious was done, wrapped at 72.
Do not list files — the diff does that.

Append the trailer:

```
Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>
```

## 5. Branch discipline

`master` is the main branch. If you are on `master` and this is not a trivial
fix the user asked for directly, branch first:

```bash
git switch -c feature/<ShortName>      # the repo's existing convention
```

Commit and push only when the user asks. Never `--no-verify`, never
force-push a shared branch.

## 6. PRs

```bash
gh pr create --base master --title "<same style as the subject>" --body "..."
```

Body: what changed, why, how it was verified (name the gate steps that ran),
and anything the reviewer should check by hand (UI, Android). End with:

```
🤖 Generated with [Claude Code](https://claude.com/claude-code)
```
