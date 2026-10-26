# Plasma Notes Restore

Kleines Qt6-Programm zur Wiederherstellung verlorener Plasma-Notes-Einträge aus `~/.local/share/plasma_notes/`.

## Verhalten

- liest `~/.config/plasma-org.kde.plasma.desktop-appletsrc`
- sammelt vorhandene `noteId=`-Einträge
- ergänzt nur fehlende Notes
- wenn keine Notes-Konfiguration mehr existiert, kann ein Desktop-Containment ausgewählt werden
- erzeugt vor jeder Änderung ein Timestamp-Backup
- schreibt die Konfiguration atomar mit `QSaveFile`

## Build

```bash
cmake -B build
cmake --build build
```

Danach liegt die Binärdatei unter `build/plasma-notes-restore`.

## Hinweis

Nach einer Änderung muss Plasma die Konfiguration neu laden. Zum Testen am sichersten zunächst ausloggen/einloggen; ein erzwungener plasmashell-Neustart kann ebenfalls funktionieren, sollte aber bewusst erfolgen.
