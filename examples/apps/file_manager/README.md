# Yazi-Style Terminal File Manager

A modern, fast, 3-column Miller column terminal file manager written in C++20 and pure Qtui/QML. Inspired by [Yazi](https://github.com/sxyazi/yazi), it features reactive directory navigation, real-time syntax-previewed text inspection, directory subtree exploration, metadata cards, and Vim keybindings.

## Architecture

- **Three-Column Miller Navigation**:
  - **Parent Column**: Shows parent directory context, with active directory highlighted.
  - **Current Column**: Interactive file explorer with cursor indicator, selection checkboxes `[✓]`, file type icons (📁, ⚡, 🦀, 🐍, 📄, ⚙️, 📦, 🖼️), and sizes.
  - **Preview Column**: Dynamic live preview pane:
    - Text/Code files: Line-numbered content preview.
    - Directories: Subtree items and folder stats.
    - Binary/Images/Archives: MIME type, file size, permissions, owner, and metadata cards.
- **Modern Qt 6.8+ Core Backend**:
  - Uses `QDirListing` for low-overhead C++20 range-based directory entry streaming.
  - Uses `QStorageInfo` for real-time filesystem capacity and mount point metrics.
- **Top Header Bar**: User@host badge, path breadcrumbs (`~ ❯ projects ❯ Qtui`), drive usage, and mode badges (`NORMAL`, `SELECT`, `FILTER`).
- **Bottom Status Bar**: File permissions/owner, toast notifications, selection count, sorting mode indicator, cursor position, and quick shortcuts cheatsheet.
- **Interactive Modals**:
  - **Live Filter**: Instant substring filtering with `/` or `f`.
  - **Help Modal**: Comprehensive keybindings cheat sheet (`?` or `F1`).
  - **Create / Rename Dialog**: Input modal (`a` or `r`).
  - **Confirm Delete Dialog**: Confirmation modal (`d` or `Delete`).

## Keybindings

| Key | Action |
| --- | --- |
| `h` / `Left` | Navigate to parent directory (`cd ..`) |
| `l` / `Right` / `Enter` | Enter directory / open file |
| `j` / `Down` | Move cursor down |
| `k` / `Up` | Move cursor up |
| `g` / `Home` | Jump to first item |
| `G` / `End` | Jump to last item |
| `Space` / `v` | Toggle selection mark on current item |
| `V` / `Ctrl+A` | Select all / invert selection |
| `y` / `x` / `p` | Yank (copy) / Cut / Paste |
| `a` / `n` | Create new file or directory |
| `r` / `F2` | Rename item |
| `d` / `Delete` | Delete selected or current item |
| `/` / `f` | Open live filter bar |
| `.` | Toggle hidden dotfiles |
| `s` | Cycle sort mode (Name, Size, Modified Time, Type) |
| `?` / `F1` | Open keybindings help modal |
| `q` / `Esc` | Quit application (contextual escape) |

## Building & Running

```bash
# Build
rtk cmake -S . -B build -DQTUI_BUILD_EXAMPLES=ON -DQTUI_BUILD_TESTS=ON
rtk cmake --build build --target qtui_file_manager

# Run live
./build/examples/apps/file_manager/qtui_file_manager

# Run automated scenarios
rtk ctest --test-dir build -R '^file_manager_' --output-on-failure
```
