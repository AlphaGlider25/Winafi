# Contributing to Winafi

Thank you for considering a contribution. Winafi is a native-Linux bootable-USB tool, and we keep
the bar **deliberately strict** so the codebase stays reliable, modular, and reviewable. Read this
document in full before opening a pull request. PRs that do not follow it will be asked to change or
closed.

> **TL;DR:** Fork → branch named `{display name}-devbranch` → one logical change → fill the PR
> template exactly (3-sentence "what", 3-sentence "how", AI disclosure) → build & tests pass.

---

## 1. Before you start

- **Open or find an issue first.** Every PR must link an issue (`Closes #123`). For a one-line typo
  fix you may write `N/A — trivial` in the linked-issue field with a short justification.
- **One PR = one logical change.** Do not bundle a bug fix with a refactor with a new feature.
  Unrelated changes will be split.
- **Read `docs/ARCHITECTURE.md`.** It defines the module boundaries. Put code where it belongs.

## 2. Fork and branch (required naming)

1. **Fork** the repository to your own GitHub account.
2. Create a branch off `main` named **exactly**:

   ```
   {display name}-devbranch
   ```

   - `{display name}` is your name/handle, lower-case, words separated by single hyphens.
   - Examples that **pass**: `jane-doe-devbranch`, `alex-devbranch`, `kai-m-devbranch`.
   - Examples that **fail**: `feature/login`, `Jane_Doe-devbranch`, `jane-devbranch-2`, `patch-1`.
   - Regex the CI enforces: `^[a-z0-9]+(-[a-z0-9]+)*-devbranch$`.

   ```bash
   git checkout main
   git checkout -b jane-doe-devbranch
   ```

> Maintainers note: the core project is versioned internally with **Tiv**, but external
> contributions come in as standard **Git** forks and pull requests — use Git as described here.

## 3. Build and test locally (must pass before you push)

```bash
# Dependencies (Debian/Ubuntu):
sudo apt-get install -y build-essential cmake pkg-config \
  libqt5widgets5 qt5-qmake libqt5core5a qtbase5-dev \
  libudev-dev libparted-dev libarchive-dev libcdio-dev \
  libmount-dev libblkid-dev libssl-dev libcurl4-openssl-dev

cmake -S . -B build
cmake --build build -j"$(nproc)"
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

All tests must pass. If a test cannot run in your environment, say so explicitly in the PR.

## 4. Code rules (strict)

1. **Follow existing module boundaries** (`docs/ARCHITECTURE.md`). Backend logic goes in
   `src/core` / `src/platform/linux`; GUI goes in `src/gui` with one widget class per file under
   `src/gui/sections/`. Do **not** put business logic in GUI classes.
2. **Test-driven.** New behaviour ships with a test. C logic → a `tests/unit/test_*.c`; Qt widgets →
   a headless test using `QT_QPA_PLATFORM=offscreen` (see `tests/unit/test_customize_section.cpp`).
3. **One responsibility per file.** If a file you touch has grown unwieldy, prefer splitting it over
   adding to the pile — but do not do unrelated refactors in the same PR.
4. **No build artefacts, no secrets.** Never commit `build/`, binaries, keys, or IDE files.
5. **C: C11, no compiler warnings** in the files you change. **C++: C++17, Qt5.**
6. **Match the surrounding style** — naming, indentation, comment density. Read the file first.
7. **Security-sensitive areas** (the boot chain, partitioning, anything writing to a block device)
   get extra scrutiny: explain *why* a change is safe in the PR.

## 5. The pull request

When you open the PR, GitHub pre-fills `.github/PULL_REQUEST_TEMPLATE.md`. Fill it **exactly**:

- **What changed** — **1 to 3 sentences**, factual, describing only what you changed.
- **How it helps** — **1 to 3 sentences**, describing the benefit.
- **AI usage disclosure** — tick **one** box:
  - **No** — no AI tools used.
  - **Yes** — any AI tool (Copilot, Claude, ChatGPT, Cursor, etc.) wrote, generated, refactored, or
    substantially suggested code/docs. This is **honour-based**. Ticking "Yes" is **not** grounds for
    rejection; it simply adds an informational `Ai Detected-warning` label so reviewers know to look
    closely. Hiding AI use, if discovered, **is** grounds for closing the PR.
- **Contribution checklist** — every box must be `[x]`.
- **Linked issue** — required.

### What the automated check enforces (`PR Validation` workflow)

Your PR will fail CI if any of these are true:

| Rule | Failure |
|------|---------|
| Branch name | does not match `^[a-z0-9]+(-[a-z0-9]+)*-devbranch$` |
| "What changed" | empty, or more than 3 sentences |
| "How it helps" | empty, or more than 3 sentences |
| AI disclosure | neither box ticked, or both ticked |
| Checklist | any unchecked `[ ]` box remains |
| Build/tests | `cmake` build or `ctest` fails |

If the AI box is ticked **Yes**, the workflow adds the **`Ai Detected-warning`** label (creating it
if it does not exist). This is informational only.

## 6. Review and merge

- A maintainer reviews every PR. Expect questions, especially on boot/partition code.
- Address review comments by pushing to the same branch.
- Squash-merge is the default. Your branch is deleted after merge.

## 7. Reporting bugs / requesting features

Open an issue with: what you expected, what happened, your distro + version, the ISO type, and exact
steps. For crashes, include logs and the command you ran.

## Code of conduct

Be respectful and constructive. Harassment or hostility gets you removed. Disagree about code, not
about people.
