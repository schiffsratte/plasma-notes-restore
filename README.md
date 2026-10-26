# Plasma Notes Restore

A small Qt6 program for recovering lost Plasma Notes entries from `~/.local/share/plasma_notes/`.
The program is intended to recover notes that have been lost due to changes in screen configuration or similar issues.
It is not designed for—and will not work for—notes that have been deleted.

## Behavior

- Reads `~/.config/plasma-org.kde.plasma.desktop-appletsrc`
- Collects existing `noteId=` entries
- Adds only the missing notes
- If no notes configuration exists, a desktop containment can be selected
- Creates a timestamped backup before making any changes
- Writes the configuration atomically using `QSaveFile`

## Build

```bash
cmake -B build
cmake --build build
```

The binary will then be located at `build/plasma-notes-restore`.

## Note

Plasma must reload the configuration after any changes are made. 
For testing purposes, the safest approach is to log out and log back in; 
a forced restart of `plasmashell` may also work, but should be done with caution.
(with systemctl --user restart plasma-plasmashell.service)
