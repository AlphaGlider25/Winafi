<!--
  ┌─────────────────────────────────────────────────────────────────────┐
  │  WINAFI PULL REQUEST TEMPLATE — THIS TEMPLATE IS MANDATORY            │
  │  Do NOT delete the headings or the marker comments. The automated     │
  │  PR check parses them. A PR that removes/renames a required section,  │
  │  exceeds the sentence limits, or leaves a section blank will FAIL.    │
  │  Read CONTRIBUTING.md before opening this PR.                         │
  └─────────────────────────────────────────────────────────────────────┘
-->

## What changed
<!-- BEGIN:WHAT_CHANGED -->
<!-- Exactly 1–3 sentences. Describe ONLY what you changed, factually. No marketing. -->

<!-- END:WHAT_CHANGED -->

## How it helps
<!-- BEGIN:HOW_IT_HELPS -->
<!-- Exactly 1–3 sentences. Explain the benefit to users or the project. -->

<!-- END:HOW_IT_HELPS -->

## AI usage disclosure
<!-- Tick ONE box with an "x" (e.g. "- [x]"). This is required and honour-based.
     If any AI tool (Copilot, Claude, ChatGPT, Cursor, etc.) wrote, generated,
     refactored, or substantially suggested code/docs in this PR, tick "Yes".
     Ticking "Yes" is NOT a rejection — it only adds an informational label. -->
- [ ] **No** — no AI tools were used for any part of this PR.
- [ ] **Yes** — AI tools were used (the PR will be labelled `Ai Detected-warning`).

<!-- If Yes, briefly name the tool(s) and what they touched (optional but appreciated): -->
<!-- AI tools used: -->

## Contribution checklist
<!-- Every box MUST be ticked "[x]". Unchecked boxes fail the automated check. -->
- [ ] My branch is named `{display name}-devbranch` (e.g. `jane-doe-devbranch`).
- [ ] This PR changes **one** logical thing (split unrelated changes into separate PRs).
- [ ] `cmake -S . -B build && cmake --build build` succeeds locally.
- [ ] `QT_QPA_PLATFORM=offscreen ctest --test-dir build` passes locally (or I explain why below).
- [ ] I added/updated tests for behaviour I changed (or it is non-code/docs-only).
- [ ] I did **not** commit build artefacts, secrets, or unrelated formatting churn.
- [ ] I followed the module boundaries in `docs/ARCHITECTURE.md`.
- [ ] I read and followed `CONTRIBUTING.md`.

## Linked issue
<!-- Required: reference the issue this addresses, e.g. "Closes #123".
     If there is no issue, write "N/A — trivial" and one line of justification. -->
Closes #
