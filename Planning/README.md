# Planning

## User story

A user story is a few sentences about a player doing something in the game, or a developer doing something in the source or the build. It names any game concept the story introduces. The Acceptance section says what is true when the story is done. Implementation notes hold details that do not belong in the story or the acceptance criteria, and that section is left out when there are none.

One file is one user story.

- An open story is a file in this folder.
- A story in `Done/` has met the Definition of Done. Move the file then. Do not leave a copy here.
- The file is the story.
- A commit that changes a story starts with `Plan: `. A product commit does not. That prefix is how the two are told apart. The final commit updates the story file and moves it into `Done/`. That commit message starts with `Plan: `.
- A story update that stands on its own is pushed to `main`. It does not ask for a build. A story update on top of product commits the builder has not landed stays on `build/<name>`, and that tip is buildable. Product work that needs Unreal uses `build/<name>`, including a story that is already In Progress. `AGENTS.md` is that rule.

## Status

The line under the title is the status.

- `New` — identified, and not yet Ready for development.
- `Ready` — meets the Definition of Ready.
- `In Progress` — a programmer or an agent has started work on a Ready story, and it is not yet Done.
- `Done` — meets the Definition of Done.

## Definition of Ready (DoR)

The agent and the human developer must agree that the story is ready. Set `Ready` after that agreement. A Ready story has not shipped.

The story must have Acceptance Criteria. Those are the `Acceptance` section: what is true when the story is done.

The story should call out any game concept it introduces, so the implementing agent knows to write a class for each one, following [`CodingGuidelines.md`](../CodingGuidelines.md). A game concept is a C++ class.

A Ready story may depend on another Ready story. The story names that dependency. The named story ships first.

## Definition of Done (DoD)

Set `Done` after the story has been properly tested and the user and the agent have agreed it is done, then move the file to `Done/`. The agreement is a discussion. The user says whether they are happy with the implementation. The agent confirms that every acceptance criterion is met. `AGENTS.md` says what properly tested means. The commit that updates the story file and moves it into `Done/` has a message that starts with `Plan: `.

The story stays `In Progress` after the code lands on `main`. Files already in `Done/` stay as they are. A file in `Done/` records why that work was done. It does not instruct later work. A rule that later work still follows lives in [`AGENTS.md`](../AGENTS.md), [`CodingGuidelines.md`](../CodingGuidelines.md), [`AgentBuild.md`](../AgentBuild.md), this file, or [`README.md`](../README.md).

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
