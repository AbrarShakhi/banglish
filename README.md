# banglish

*PROJECT IS UNDER DEVELOPMENT*. A linux shell written in C. It might not ru right now.

---

### Built-in Commands

| Command | Description |
|---------|-------------|
| `cd [DIR]` | Change directory (`-` for previous) |
| `echo [-n] ...` | Print arguments |
| `exit [CODE]` | Exit the shell |
| `pwd` | Print working directory |
| `jobs` | List background jobs |
| `fg [%N]` | Bring job to foreground |
| `bg [%N]` | Resume stopped job in background |
| `source FILE` | Execute file in current shell |
| `export NAME` | Export variable to environment |
| `unset NAME` | Remove shell variable |
| `alias [name=val]` | Define or list aliases |
| `unalias NAME` | Remove alias |
| `type NAME` | Show type (builtin / function / external) |
| `which CMD` | Show full path of command |
| `history [N]` | Show last N history entries |
| `help` | List all builtins |

---

## Building

### Dependencies

| Package | Purpose |
|---------|---------|
| `gcc` | C17 compiler |
| `cmake` | Build system (>= 3.16) |
| `make` | Build runner |
| `libreadline-dev` | Line editing and history |
| `libncurses-dev` | Terminal support (readline dependency) |

Install on Ubuntu/Debian:

```sh
sudo apt install gcc cmake make libreadline-dev libncurses-dev
```

### Build

```sh
git clone https://github.com/AbrarShakhi/bang-lish.git
cd bang-lish
cmake -B build
make -C build -j$(nproc)
```

The binary is at `./build/banglish`.

### Install system-wide

```sh
sudo make -C build install   # installs to /usr/local/bin/banglish
```

---

## Usage

```sh
# Interactive shell
./build/banglish

# Run a script file
./build/banglish script.bl

# Run a command string
./build/banglish -c "echo hello | tr a-z A-Z"
```

---

## License

See [LICENSE](LICENSE).
