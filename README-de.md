# Plasma Notes Restore

Kleines Qt6-Programm zur Wiederherstellung verlorener Plasma-Notes-Einträge aus `~/.local/share/plasma_notes/`.
Das Programm soll verlorene Notes, z.B. durch Änderung der Bildschirm Konfiguration usw, wiederherstellen.
Funktioniert nicht für gelöschte PlamaNotes. Diese müssen, wenn noch im Papierkorb vorhanden, 
erst wiederhergestellt werden.

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

Nach einer Änderung muss Plasma die Konfiguration neu laden. Zum Testen ist es am sichersten sich auszuloggen und wieder einzuloggen.
Ein erzwungener plasmashell-Neustart kann ebenfalls funktionieren, kann aber andere laufende Programme ohne Vorwarnung beenden - also Vorsicht.
(mit systemctl --user restart plasma-plasmashell.service)
