# shell

A Unix shell written in C. Supports command execution, piping, I/O redirection, and basic builtins.

## Features

- **Command execution** — runs any program on your `$PATH` via `fork` + `execvp`
- **Piping** — connects two commands with `|`, wiring stdout of the left to stdin of the right using `pipe` + `dup2`
- **I/O redirection** — `>` redirects stdout to a file, `<` reads stdin from a file
- **Builtins** — `cd`, `pwd`, `echo`, `history`, `exit`
- **History** — tracks commands in-session and prints them with `history`

## Build & Run

```bash
gcc shell.c -o shell
./shell
```

## What I'd Add Next

- Persist history to `~/.shell_history` across sessions
- `&&` and `||` chaining
- Ctrl+C signal handling
