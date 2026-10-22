# Planning

One file is one story.

- An open story is a file in this folder.
- A story in `Done/` has met the Definition of Done. Move the file then. Do not leave a copy here.
- `ideas.md` at the repo root is the inbox. Copy a new idea into a file here. Do not edit `ideas.md`.
- The file is the story.
- A commit that changes a story starts with `Plan: `. A product commit does not. That prefix is how the two are told apart. The final commit updates the story file and moves it into `Done/`. That commit message starts with `Plan: `.

## Status

The line under the title is the status.

- `New` — not refined yet.
- `Ready` — meets the Definition of Ready, and not shipped. A Ready story may depend on another Ready story. The story names that dependency. The named story ships first.
- `In Progress` — an agent has started the story. Set this when work starts on a `Ready` story. The story stays `In Progress` after the code lands on `main`.
- `Done` — meets the Definition of Done. Files already in `Done/` stay as they are.

## Definition of Ready (DoR)

The agent and the human developer must agree that the story is ready. Set `Ready` after that agreement.

The story must have Acceptance Criteria. Those are the `Acceptance` section: what is true when the story is done.

The story should call out any game concept it introduces, so the implementing agent knows to write a class for each one, following [`CodingGuidelines.md`](../CodingGuidelines.md). A game concept is a C++ class.

## Definition of Done (DoD)

Set `Done` after the story has been properly tested and the user and the agent have agreed it is done, then move the file to `Done/`. The agreement is a discussion. The user says whether they are happy with the implementation. The agent confirms that every acceptance criterion is met. `AGENTS.md` says what properly tested means. The commit that updates the story file and moves it into `Done/` has a message that starts with `Plan: `.

## Open story

```md
# Title

Status: New

## Story

A player does something in the game, or a developer does something in the source or the build. A few sentences. Name any game concept this story introduces.

## Acceptance

What is true when the story is done. These are the Acceptance Criteria.

## Implementation notes

Details that do not belong in the story or the acceptance criteria. Leave this out when there are none.
```
