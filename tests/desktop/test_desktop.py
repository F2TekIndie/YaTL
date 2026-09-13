"""Verify app activation and the packaged DMS/niri integration contracts."""
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import time
import unittest

APP = str(Path(sys.argv.pop(1)).resolve())
CLI = str(Path(sys.argv.pop(1)).resolve())
REPO = Path(sys.argv.pop(1)).resolve()


class DesktopIntegrationTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="yatl-desktop-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.db = self.root / "tasks.sqlite3"
        self.runtime = self.root / "runtime"
        self.runtime.mkdir(mode=0o700)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.niri_log = self.root / "niri.log"
        fake_niri = self.bin / "niri"
        fake_niri.write_text(
            "#!/bin/sh\n"
            "if [ \"$*\" = \"msg -j windows\" ]; then\n"
            "  printf '%s\\n' '[{\"id\":42,\"app_id\":\"org.yatl.YaTL\",\"is_focused\":false}]'\n"
            "  exit 0\n"
            "fi\n"
            "printf '%s\\n' \"$*\" >> \"$NIRI_LOG\"\n"
        )
        fake_niri.chmod(fake_niri.stat().st_mode | stat.S_IXUSR)
        self.env = {
            **os.environ,
            "PATH": f"{self.bin}:{os.environ['PATH']}",
            "NIRI_LOG": str(self.niri_log),
            "XDG_RUNTIME_DIR": str(self.runtime),
            "QT_QPA_PLATFORM": "offscreen",
            "QT_QPA_PLATFORMTHEME": "generic",
            "QT_QUICK_CONTROLS_STYLE": "Fusion",
            "QT_QUICK_BACKEND": "software",
            "QSG_RHI_BACKEND": "software",
        }

    def cli(self, *arguments):
        result = subprocess.run(
            [CLI, "--database", str(self.db), *arguments], env=self.env,
            capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        return json.loads(result.stdout)

    def test_running_app_switches_view_and_focuses_through_niri(self):
        process = subprocess.Popen(
            [APP, "--database", str(self.db)], env=self.env,
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        def stop_app():
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=5)
            process.stderr.close()
        self.addCleanup(stop_app)
        result = None
        for _ in range(30):
            time.sleep(0.1)
            result = self.cli("open", "today")
            if result["action"] == "activated":
                break
        self.assertEqual(result["action"], "activated")
        self.assertEqual(result["view"], "today")
        self.assertTrue(result["niri_focused"])
        second = subprocess.run(
            [APP, "--database", str(self.db), "--view", "upcoming"], env=self.env,
            capture_output=True, text=True, timeout=5)
        self.assertEqual(second.returncode, 0, second.stderr)
        focused = self.cli("focus")
        self.assertEqual(focused["action"], "activated")
        self.assertEqual(focused["view"], "upcoming")
        self.assertIn("msg action focus-window --id 42", self.niri_log.read_text())

    def test_launch_and_capture_use_the_configured_app(self):
        app_log = self.root / "app.log"
        fake_app = self.bin / "fake-yatl"
        fake_app.write_text("#!/bin/sh\nprintf '%s\\n' \"$*\" >> \"$YATL_APP_LOG\"\n")
        fake_app.chmod(fake_app.stat().st_mode | stat.S_IXUSR)
        self.env["YATL_APP_EXECUTABLE"] = str(fake_app)
        self.env["YATL_APP_LOG"] = str(app_log)
        self.assertEqual(self.cli("open", "today")["action"], "launched")
        self.assertEqual(self.cli("capture")["view"], "capture")
        for _ in range(20):
            if app_log.exists() and len(app_log.read_text().splitlines()) >= 2:
                break
            time.sleep(0.05)
        calls = app_log.read_text().splitlines()
        self.assertTrue(any("--view today" in call for call in calls))
        self.assertTrue(any("--quick-capture" in call for call in calls))

    def test_dms_plugin_contract(self):
        plugin = REPO / "integrations/dms/YaTL"
        manifest = json.loads((plugin / "plugin.json").read_text())
        self.assertEqual(manifest["id"], "yatl")
        self.assertEqual(manifest["type"], "widget")
        self.assertIn("process", manifest["permissions"])
        self.assertTrue((plugin / manifest["component"][2:]).is_file())
        self.assertTrue((plugin / manifest["startupCheck"][2:]).is_file())
        source = (plugin / "YaTLWidget.qml").read_text()
        for command in ("summary", "today", "settings", "add", "complete", "open", "capture"):
            self.assertIn(f'"{command}"', source)
        self.assertIn("--use-default", source)

    def test_niri_fragment_and_desktop_ids(self):
        fragment = (REPO / "integrations/niri/yatl.kdl").read_text()
        self.assertIn("org\\.yatl\\.YaTL$", fragment)
        self.assertIn("org\\.yatl\\.YaTL\\.QuickCapture$", fragment)
        self.assertIn('spawn "yatlctl" "focus"', fragment)
        self.assertIn('spawn "yatlctl" "capture"', fragment)
        main = (REPO / "integrations/niri/org.yatl.YaTL.desktop").read_text()
        capture = (REPO / "integrations/niri/org.yatl.YaTL.QuickCapture.desktop").read_text()
        self.assertIn("Exec=yatl", main)
        self.assertIn("Exec=yatl --quick-capture", capture)


if __name__ == "__main__":
    unittest.main()
