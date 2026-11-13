# Repository Guidelines

## Project Structure

nt-tools adapts Unix-style build commands to Windows compilers and execution
environments. `init_nt` selects the transport, loads local configuration, and
sets the tool environment. `tools/` contains shell and Pike wrappers such as
`rntcl`, `ar`, and `sprsh`. Shared Pike transport and path-conversion code lives
in `tools/lib.pike`; `tools/lib` provides shell helpers. `tools/sprshd` is the
Windows remote-command daemon. Protocol regressions live beside these scripts
in `tools/sprsh.test` and `tools/sprshd.test`.

There is no standalone application build or repository-wide Makefile.

## Development and Validation

Run from the repository root:

- `sh -n init_nt tools/ar tools/lib`: check shell syntax.
- `pike tools/sprsh.test`: run client framing and stdin-forwarding tests.
- `pike tools/sprshd.test`: run daemon compatibility tests with local sockets
  and temporary files.
- `pike tools/rntas.test`: check GNU assembler arguments, output aliases,
  compiler dispatch, and error handling without Windows tools.
- `pike tools/rntcl-pdb.test`: check PDB locking across concurrent compiler
  wrappers, failure cleanup, and preprocessing with a mock compiler.
- `git diff --check` and `git diff --cached --check`: check whitespace.

Run daemon tests with both Pike 8.0 and a current Pike interpreter. For compiler
wrapper changes, also compile, link, and execute a small C program using the
relevant transport. From a Pike source or build directory, invoke
`/path/to/nt-tools/init_nt make`. Debug compiler invocations share a
per-directory PDB lock, allowing parallel builds across directories even with
older Microsoft compilers.

For GNU assembly configuration and library compatibility fixes, see
`HOWTO-PORTING.txt`. It covers GMP, Nettle/Hogweed and zlib with MSVC under Wine.

## Style and Compatibility

Preserve existing encodings, executable bits, and surrounding style. Use spaces
for indentation on changed lines, generally two per level; avoid unrelated
formatting changes. Keep shell scripts compatible with `/bin/sh` and quote paths
and arguments, especially Windows paths containing spaces or backslashes.

The daemon must start with Pike 8.0. Prefer capability checks such as
`#if constant(...)`; provide a conditional typedef when a newer type alias is
unavailable. Preserve protocol compatibility with old clients and daemons.
Test ordinary stdin, OOB EOF, fragmented replies, and connection shutdown when
changing sprsh transport code.

## Configuration and Commits

Keep machine-specific settings in `~/.init_ntrc` or the file selected by
`INIT_NTRC`. Relevant variables include `REMOTE_METHOD`, `NTHOST`, `NTPORT`,
`NTDRIVE`, and `NTMOUNT`. Do not commit local paths, credentials, or generated
logs.

Follow subsystem-prefixed commit subjects, for example
`sprsh: Preserve replies when builtins receive legacy client input`.
Describe behavior and validation. Preserve unrelated workspace changes; stage
only the requested work. When staging, save a suggested commit message at the
path from `git rev-parse --git-path MERGE_MSG`, unless instructed otherwise.
