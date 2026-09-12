# DankMaterialShell integration

The `YaTL` directory is a DMS 1.6 widget plugin. The YaTL package installs it
under `/usr/local/share/yatl/dms/YaTL`; the user chooses when to add it to DMS:

```sh
mkdir -p ~/.config/DankMaterialShell/plugins
cp -a /usr/local/share/yatl/dms/YaTL ~/.config/DankMaterialShell/plugins/
dms ipc plugin-scan rescan yatl
```

Enable **YaTL** in DMS Settings → Plugins, then add it to DankBar. The widget
refreshes every five seconds, shows today's count and the next actionable task,
and opens a popout for Inbox capture, completion, opening Today, and quick
capture. It invokes `yatlctl` with argument arrays and expects its JSON output.
