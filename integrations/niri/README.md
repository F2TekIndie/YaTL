# niri integration

YaTL exposes stable app IDs for the main and quick-capture windows. To apply the
recommended tiled main window, floating capture window, and keybinds, add this
line to `~/.config/niri/config.kdl`:

```kdl
include "/usr/local/share/yatl/niri/yatl.kdl"
```

The packaged fragment binds `Mod+Y` to focus or launch YaTL, `Mod+Shift+Y` to
open Today, and `Mod+Ctrl+Y` to show quick capture. Change the keys if they are
already assigned. YaTL never edits or reloads the user's niri configuration.
Validate the combined configuration before reloading it:

```sh
niri validate
```
