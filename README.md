# kern

`kern` is a small Unix-like interactive shell written in C. It provides a prompt, built-in commands, external command execution, pipes, redirection, background jobs, persistent history, and TAB completion powered by GNU Readline.

## What this project does

This project implements a custom command-line shell executable (`kern`) that:

- Reads commands interactively from a prompt
- Executes built-in shell commands
- Executes system commands from `PATH`
- Supports command chaining with `&&`
- Supports pipelines with `|`
- Supports output/error redirection (`>`, `>>`, `1>`, `1>>`, `2>`, `2>>`)
- Supports background jobs with `&` and job listing/notifications
- Persists command history across sessions
- Provides command and file path auto-completion on `TAB`

## Repository layout

- `main.c`  
  Main shell loop, prompt rendering, built-ins, process execution, pipelines, chaining, redirection hooks, job tracking, history initialization, signal handling.
- `utils.c`, `utils.h`  
  Utility helpers for parsing commands, resolving `PATH` values, longest-common-prefix calculation, and working directory retrieval.
- `completion.c`, `completion.h`  
  TAB completion logic for commands and filesystem entries, including multi-match display behavior.
- `kern`  
  Prebuilt ELF executable for Linux.
- `success.txt`  
  Empty text file (can be used as a sample output target when testing redirection).
- `.vscode/c_cpp_properties.json`  
  VS Code C/C++ IntelliSense/compiler configuration.
- `.vscode/launch.json`  
  VS Code debug launch configuration.
- `.vscode/settings.json`  
  VS Code C/C++ Runner defaults and warning profile.
- `.vscode/tasks.json`  
  VS Code build task template for compiling C files.

## Built-in commands

The shell supports these built-ins in the command executor:

- `exit` — exit the shell
- `echo` — print arguments
- `type <cmd>` — report whether a command is a built-in or an executable found in `PATH`
- `pwd` — print current working directory
- `cd <path>` — change directory (`~` expansion supported)
- `jobs` — list tracked background jobs
- `history [N]` — print command history (all or last `N` entries)

## Requirements

- Linux/Unix environment
- GCC (or compatible C compiler)
- GNU Readline development library and runtime

On Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential libreadline-dev
```

## Build

From the project root:

```bash
gcc -Wall -Wextra -pedantic main.c utils.c completion.c -lreadline -o kern
```

## Run

From the project root:

```bash
./kern
```

You should see a prompt like:

```text
~/current/path $ 
```

## How to use

### 1. Run built-ins

```bash
pwd
cd ~/Downloads
echo hello world
history 10
```

### 2. Run external commands

```bash
ls -la
grep "main" main.c
```

### 3. Chain commands with `&&`

```bash
ls && pwd && echo done
```

### 4. Use pipelines

```bash
ls -la | grep ".c"
```

### 5. Redirect output/error

```bash
echo "build ok" > success.txt
echo "append line" >> success.txt
ls non_existing_file 2> error.log
```

### 6. Run background jobs

```bash
sleep 5 &
jobs
```

When background jobs finish, the shell prints a completion notification.

### 7. Use TAB completion

- Press `TAB` while typing the first token to complete command names.
- Press `TAB` after a space (or on paths) to complete file/directory names.

## History behavior

- History file: `~/.kern_shell_history`
- History is loaded at startup and appended command-by-command
- Session history is limited to 500 entries in memory

## Notes

- The included `kern` binary is already built for Linux x86_64.
- Rebuild from source if you make code changes or if your environment differs.
