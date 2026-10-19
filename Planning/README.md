# Planning

One file is one story.

- An open story is a file in this folder.
- A shipped story is a file in `Done/`. Move the file when it ships. Do not leave a copy here.
- `ideas.md` at the repo root is the inbox. Copy a new idea into a file here. Do not edit `ideas.md`.
- `README.md` lists the files. The file is the story.
- A commit that changes a story starts with `Plan: `. A product commit does not. That prefix is how the two are told apart.

## Status

The line under the title is the status.

- `New` — not refined yet.
- `Ready` — refined, and not shipped. A Ready story may depend on another Ready story. The story names that dependency. The named story ships first.
- `In Progress` — an agent has started the story. Set this when work starts on a `Ready` story.
- `Done` — set this when the story ships, then move the file to `Done/`. Files already in `Done/` stay as they are.

## Open story

```md
# Title

Status: New

## Story

A player does something in the game, or a developer does something in the source or the build. A few sentences.

## Acceptance

What is true when the story is done.

## Implementation notes

Details that do not belong in the story or the acceptance criteria. Leave this out when there are none.
```
