"""Exercise real processes against isolated databases, including concurrent writers."""
import concurrent.futures
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())


class CliTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="yatl-cli-")
        self.addCleanup(self.tmp.cleanup)
        self.db = str(Path(self.tmp.name) / "tasks.sqlite3")

    def run_cli(self, *args, code=0):
        result = subprocess.run([BINARY, "--database", self.db, *args],
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, code, result.stderr)
        self.assertEqual(result.stderr if code == 0 else result.stdout, "")
        return json.loads(result.stdout if code == 0 else result.stderr)

    def test_complete_loop_and_history(self):
        self.assertEqual(self.run_cli("list"), {"tasks": []})
        task = self.run_cli("add", "  Buy milk 🥛  ")["task"]
        self.assertEqual(task["title"], "Buy milk 🥛")
        self.assertEqual(self.run_cli("list")["tasks"], [task])
        self.assertEqual(self.run_cli("summary")["open_count"], 1)
        self.assertTrue(self.run_cli("complete", task["id"])["changed"])
        self.assertFalse(self.run_cli("complete", task["id"])["changed"])
        self.assertEqual(self.run_cli("list")["tasks"], [])
        done = self.run_cli("list", "--filter", "completed")["tasks"]
        self.assertEqual(done[0]["id"], task["id"])
        self.assertEqual(done[0]["status"], "completed")
        with sqlite3.connect(self.db) as db:
            self.assertEqual(db.execute("SELECT type FROM task_events ORDER BY id").fetchall(),
                             [("created",), ("completed",)])

    def test_invalid_commands_and_input(self):
        for args, code in [(("add", " "), 1), (("add",), 2), (("complete", "99"), 1),
                           (("complete", "-1"), 2), (("wat",), 2), (("--bad",), 2),
                           (("list", "--filter", "bad"), 1), (("summary", "--filter", "all"), 2)]:
            self.assertIn("error", self.run_cli(*args, code=code))
        self.assertEqual(self.run_cli("list")["tasks"], [])
        self.assertEqual(self.run_cli("add", "--", "--literal")["task"]["title"], "--literal")

    def test_concurrent_first_launch_and_writes(self):
        def add(index):
            return self.run_cli("add", f"Parallel {index}")["task"]["id"]
        with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
            ids = list(pool.map(add, range(18)))
        self.assertEqual(len(set(ids)), 18)
        self.assertEqual(len(self.run_cli("list")["tasks"]), 18)

    def test_xdg_default_location(self):
        env = {**os.environ, "XDG_DATA_HOME": self.tmp.name}
        result = subprocess.run([BINARY, "add", "XDG task"], env=env,
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((Path(self.tmp.name) / "yatl/yatl.sqlite3").exists())

    def test_project_edit_move_and_reopen_loop(self):
        project = self.run_cli("project-add", "Release")["project"]
        self.assertEqual(self.run_cli("projects")["projects"], [project])
        task = self.run_cli("add", "Draft", "--project", project["id"])["task"]
        self.assertEqual(self.run_cli("list", "--project", "inbox")["tasks"], [])
        self.assertEqual(self.run_cli("list", "--project", project["id"])["tasks"], [task])
        self.run_cli("edit", task["id"], "Publish", "--project", project["id"], "--note", "Review\nShip")
        self.run_cli("complete", task["id"])
        self.assertTrue(self.run_cli("reopen", task["id"])["changed"])
        self.assertFalse(self.run_cli("reopen", task["id"])["changed"])
        updated = self.run_cli("list", "--project", project["id"])["tasks"][0]
        self.assertEqual(updated["title"], "Publish")
        self.assertEqual(updated["note"], "Review\nShip")
        self.assertEqual(updated["status"], "open")
        self.run_cli("edit", task["id"], "Publish", "--project", "inbox", "--note", "")
        self.assertEqual(self.run_cli("list", "--project", project["id"])["tasks"], [])
        self.assertIsNone(self.run_cli("list", "--project", "inbox")["tasks"][0]["project_id"])
        with sqlite3.connect(self.db) as db:
            events = db.execute("SELECT type FROM task_events ORDER BY id").fetchall()
            self.assertEqual(events, [("created",), ("edited",), ("completed",), ("reopened",), ("edited",)])

    def test_project_commands_reject_invalid_input(self):
        self.run_cli("project-add", "Release")
        self.run_cli("project-add", "release", code=1)
        self.run_cli("add", "Bad destination", "--project", "999", code=1)
        self.run_cli("edit", "1", "Title", code=2)
        self.run_cli("projects", "--project", "inbox", code=2)
        self.run_cli("list", "--project", "", code=2)
        self.assertEqual(self.run_cli("list")["tasks"], [])


if __name__ == "__main__":
    unittest.main()
