# Global Rules

> Auto-generated from ~/.shared-brain/ — edit there, not here.
> Regenerate: `brain export gemini > GEMINI.md`

These rules apply to ALL coding agents operating in this environment.
Last updated: 2026-07-15

## Identity
- User: Augusto Daniele (@tasken on GitHub)
- Server: medusa (Linux)

## Privileged Commands
- NEVER run `sudo` or `docker` commands directly.
- If a command needs sudo (Docker always needs sudo here), print the exact command and ask the user to run it manually.

## Git
- NEVER `git commit`, `git add`, or `git push` unless the user explicitly asks.
- Make code changes freely; leave version control to the user.
- When making multiple commits at user's request, use realistic human timestamps — set `GIT_AUTHOR_DATE` and `GIT_COMMITTER_DATE` so commits are spaced across plausible intervals (minutes/hours apart, chronological). Never let them all land in the same second.
  - Check the actual current time first (`date`) and keep every timestamp before it — never backdate into the future relative to real time.
  - Prefer timestamps outside typical working hours (evenings/nights) unless the user says otherwise.
  - Never use :00 for the seconds field — real commits don't land on an exact minute boundary. Vary seconds realistically (e.g. `20:12:37`, not `20:12:00`).
- Commit titles state the outcome, not the issue they fix, in simple human words (e.g. "Keep users logged in across page reloads", not "Fix session token invalidation on reload").

## Python Dependencies
- NEVER `pip install` globally or with `--break-system-packages`.
- Use the project's `.venv` (e.g. `.venv/bin/pip`), or Docker where that's the project's pattern.

## Docker
- When building or using Docker containers, try to use the `alpine` or `slim` versions of the required base images (e.g. `python:3.11-slim`, `node:20-alpine`) to keep the footprint small and build times fast, unless a specific variant is explicitly required.

## Writing Style
- Never use em dashes (—) unless explicitly asked.
- Commit messages should be short and human, focused on scope and user experience. No verbosity. Write them like a real person would.

## General Conduct
- When unsure, ask the user rather than guessing.
- Preserve existing code comments and documentation unless explicitly told to change them.
- Read the project's agent-specific rules file before making changes (CLAUDE.md, .cursorrules, copilot-instructions.md, GEMINI.md, AGENTS.md).
- Check ~/.shared-brain/ for shared context and coordination notes.

## Private Data and Secrets
- NEVER hardcode secrets, credentials, API keys, tokens, IPs, ports, or any server-specific values directly in source files.
- All environment-specific data must live in a `.env` file (or equivalent) that is gitignored.
- When creating a new project or feature that needs configuration, always create a `.env.example` with placeholder values and add `.env` to `.gitignore`.
- If you find a secret already hardcoded, move it to `.env` and replace it with a reference — never leave it in the repo.
- Before suggesting any code that contains a real value (IP, password, key, path), stop and ask if it should go in `.env` instead.

## Shared Brain & Documentation
- The shared memory bank and its documentation are located at `~/.shared-brain/` (refer to `~/.shared-brain/README.md` for CLI command details, file structure, and design documentation).
- Any agent making structural or behavioral changes to the shared brain codebase, memory models, or CLI commands MUST update the corresponding documentation files (such as `~/.shared-brain/README.md` or files in `~/.shared-brain/docs/`) to reflect those changes.

## Shared Brain CLI
- The shared memory bank is at ~/.shared-brain/ (docs: `~/.shared-brain/README.md`)
- Log a discovery: `brain note "<message>"`
- Check project context: `brain project <name>`
- Start work on a task (track it yourself): `brain start <project> "<task>"` then `brain done <id> "<result>"`
- Delegate a task to another agent: `brain handoff <project> "<task>" --to <agent>`
- Launch a delegated task: `brain run <id>`
- Check pending tasks assigned to you: `brain tasks --status pending`
