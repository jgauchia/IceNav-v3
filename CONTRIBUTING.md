# Contributing to IceNav-v3

Thanks for your interest in contributing to IceNav-v3. This document explains how to set up the environment, project conventions, and the process for submitting changes.

> Note: IceNav-v3 is under active development and some parts (vector maps, new board support) are marked as experimental. Keep this in mind when opening PRs.

## Before you start

- Check the [open issues](https://github.com/jgauchia/IceNav-v3/issues) and [PRs](https://github.com/jgauchia/IceNav-v3/pulls) to avoid duplicating work.
- For larger changes (new board, new map format, module refactor), open an issue first to discuss the approach before investing time in code.
- For small fixes or documentation improvements, feel free to go straight to a PR.

## Development environment

IceNav-v3 uses [PlatformIO](https://platformio.org/) as its build system.

1. Install PlatformIO (VSCode extension or standalone CLI).
2. Install [git](https://git-scm.com/).
3. Clone the repository:
   ```
   git clone https://github.com/jgauchia/IceNav-v3.git
   ```
4. Check `platformio.ini` for the available environments (one per supported board).
5. Build and upload firmware to your board:
   ```
   pio run -e <environment> --target upload
   ```
6. Upload assets (icons, fonts) to the filesystem from the `data/` directory:
   ```
   pio run --target uploadfs
   ```

Physical hardware isn't required to verify the project builds: CI (GitHub Actions / PlatformIO) validates the build on every pull request.

## How to add support for a new board

This is one of the most common types of contribution to the project. General steps:

1. Create a definition file in `boards/` with the board's specs (flash, PSRAM, pinout).
2. Add a new `[env:BOARD_NAME]` environment in `platformio.ini` with the corresponding build flags.
3. Adjust GPS, screen, and sensor pinout in `hal.hpp`.
4. Select the correct screen build flag based on the driver (see the supported drivers table in the README: `ILI9488`, `ILI9341`, with SPI/8-bit/16-bit and XPT2046/FT5x06 touch variants).
5. If the board shares an SPI bus between screen and SD, add `-DSPI_SHARED`.
6. If it has an accessible BOOT button (GPIO0), you can enable `-DPOWER_SAVE`.
7. Test on real hardware before submitting the PR: screen, touch, GPS, and at least one sensor if the board includes one.

Indicate the support status in the PR (`✔️ YES` or `🚧 TESTING`) based on the level of validation performed, following the boards table in the README.

## Code conventions

### C++ style

The project has strict naming and formatting rules. Code that doesn't follow them will be rejected regardless of functionality:

- **Braces (Allman):** opening brace on its own line. Never use braces for single-line control flow.
- **Variables:** one variable per line. Underscores in variable names (`my_var` or `var_`) are forbidden.
- **Functions and methods:** new functions/methods use camelCase with word splitting (e.g., `changeDirectoryHandler`, `notFoundHandler`). snake_case is forbidden for new functions; legacy snake_case names are migrated per the roadmap rename plan.
- **Comments:** English only, single line, and only when they add real value. Never state what the code does.
- **Documentation:** Doxygen only in `.cpp` files, never in `.hpp`. Never bump the version field in Doxygen headers. No `@details` blocks.

### Folder layout

- `include/` — headers
- `lib/` — internal project libraries
- `src/` — implementation
- `boards/` — per-board hardware definitions
- `data/` — filesystem assets (fonts, icons, web utils)

Match the naming style of the module you're touching.

### Commit messages

Use conventional commits in the form `type: description`:

```
fix(maps): correct vector map rendering at tile margins
perf(display): persistent rotation buffer to stop per-frame churn
feat(diag): memory snapshot in boot report and CLI mem command
refactor(lvgl): migrate deprecated v8 APIs to 9.x names
```

Allowed types: `feat`, `fix`, `refactor`, `perf`, `chore`, `build`, `docs`, `test`, `style`.

### User preferences

If you add a new user preference (accessible via the `klist`/`kset` CLI commands), document its key, default value, and description following the format already used (see the `klist` table in the README).

## Pull Request process

1. Fork the repository and create a descriptive branch (`feature/board-name`, `fix/vector-map-parsing`, etc.).
2. Make sure the project builds for at least the environment you modified:
   ```
   pio run -e <environment>
   ```
3. Verify you haven't broken other environments if you touched shared code (`hal.hpp`, renderer, etc.).
4. Open the PR against `devel` with:
   - Description of the change and motivation
   - Board(s) it has been tested on
   - Screenshots if the change affects the UI
5. Wait for review. PRs touching multiple boards or the core rendering path may take longer to review.

Releases are prepared separately: the version is bumped, then `devel` is merged to `master` through a dedicated release PR.

## How to report a bug

Always include:

- Board and environment used (`env:` from `platformio.ini`)
- Firmware version (see the `info` command in the CLI)
- Steps to reproduce
- Relevant logs: you can capture them via Serial or Telnet, and use `scshot` to attach a screenshot from the device if the bug is visual
- For memory-related issues, also include the output of the `mem` CLI command (boot profile is stored in `DIAG.log`)

## Areas where help is welcome

Check the "TO DO" section of the README for current pending tasks, for example:

- Support for multiple IMUs and compass modules
- Vector map improvements and optimization
- New supported boards

If you'd like to contribute but aren't sure where to start, open an issue asking and you'll be pointed toward a task that matches your experience.

## Credits

Significant contributions are reflected in the "Credits" section of the main README.

---

Thanks for helping make IceNav-v3 better.