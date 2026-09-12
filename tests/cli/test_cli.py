"""Exercise real processes against isolated databases, including concurrent writers."""
import concurrent.futures
from datetime import date, timedelta
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

    def test_project_settings_lists_order_and_archive(self):
        project = self.run_cli("project-add", "Release")["project"]
        self.assertEqual(project["color"], "#376548")
        self.assertFalse(project["archived"])
        self.assertTrue(self.run_cli("project-edit", project["id"], "Release 1.0",
                                     "--color", "#A1B2C3")["changed"])
        task_list = self.run_cli("list-add", project["id"], "Review")["list"]
        self.assertEqual(self.run_cli("lists", project["id"])["lists"], [task_list])
        self.assertTrue(self.run_cli("list-rename", task_list["id"], "Ready")["changed"])
        first = self.run_cli("add", "First", "--project", project["id"],
                             "--list", task_list["id"])["task"]
        second = self.run_cli("add", "Second", "--project", project["id"],
                              "--list", task_list["id"])["task"]
        ordered = self.run_cli("list", "--project", project["id"],
                               "--list", task_list["id"])["tasks"]
        self.assertEqual([item["id"] for item in ordered], [second["id"], first["id"]])
        self.assertTrue(self.run_cli("move", second["id"], "down",
                                     "--list", task_list["id"])["changed"])
        ordered = self.run_cli("list", "--project", project["id"],
                               "--list", task_list["id"])["tasks"]
        self.assertEqual([item["id"] for item in ordered], [first["id"], second["id"]])
        self.assertTrue(self.run_cli("project-archive", project["id"])["changed"])
        self.assertEqual(self.run_cli("projects")["projects"], [])
        archived = self.run_cli("projects", "--archived")["projects"][0]
        self.assertEqual(archived["name"], "Release 1.0")
        self.assertEqual(archived["color"], "#a1b2c3")
        self.assertTrue(archived["archived"])
        self.assertEqual(self.run_cli("list")["tasks"], [])
        self.run_cli("complete", first["id"], code=1)
        self.assertTrue(self.run_cli("project-restore", project["id"])["changed"])
        self.assertEqual(len(self.run_cli("list")["tasks"]), 2)

    def test_lists_and_management_reject_invalid_input(self):
        alpha = self.run_cli("project-add", "Alpha")["project"]
        beta = self.run_cli("project-add", "Beta")["project"]
        alpha_list = self.run_cli("list-add", alpha["id"], "Queue")["list"]
        beta_list = self.run_cli("list-add", beta["id"], "Queue")["list"]
        self.run_cli("project-edit", alpha["id"], "Beta", "--color", "#112233", code=1)
        self.run_cli("project-edit", alpha["id"], "Alpha", "--color", "red", code=1)
        self.run_cli("list-add", alpha["id"], "queue", code=1)
        self.run_cli("add", "Wrong", "--project", alpha["id"],
                     "--list", beta_list["id"], code=1)
        task = self.run_cli("add", "Correct", "--project", alpha["id"],
                            "--list", alpha_list["id"])["task"]
        self.run_cli("edit", task["id"], "Wrong", "--project", beta["id"],
                     "--list", alpha_list["id"], "--note", "", code=1)
        self.run_cli("move", task["id"], "sideways", "--list", alpha_list["id"], code=1)
        self.run_cli("projects", "--color", "#000000", code=2)

    def test_planning_today_upcoming_search_and_edit_preservation(self):
        today = date.today()
        tomorrow = (today + timedelta(days=1)).isoformat()
        project = self.run_cli("project-add", "Release Search")["project"]
        task_list = self.run_cli("list-add", project["id"], "Review Queue")["list"]
        task = self.run_cli("add", "High tomorrow", "--project", project["id"],
                            "--list", task_list["id"], "--due", tomorrow,
                            "--priority", "3")["task"]
        self.assertEqual(task["due_date"], tomorrow)
        self.assertEqual(task["priority"], 3)
        self.assertEqual(task["project_name"], "Release Search")
        self.assertEqual(task["list_name"], "Review Queue")
        self.run_cli("add", "Today task", "--project", project["id"],
                     "--scheduled", today.isoformat(), "--priority", "2")
        self.run_cli("add", "100% literal_name", "--note", "bad", code=2)
        literal = self.run_cli("add", "100% literal_name")["task"]
        self.assertEqual([item["title"] for item in self.run_cli("today")["tasks"]],
                         ["Today task"])
        self.assertEqual([item["id"] for item in self.run_cli("upcoming")["tasks"]],
                         [task["id"]])
        self.assertEqual(self.run_cli("search", "release search")["tasks"][0]["id"],
                         task["id"])
        self.assertEqual(self.run_cli("search", "review queue")["tasks"][0]["id"],
                         task["id"])
        self.assertEqual(self.run_cli("search", "%")["tasks"][0]["id"], literal["id"])
        self.run_cli("edit", task["id"], "Renamed", "--project", project["id"],
                     "--list", task_list["id"], "--note", "Needle note")
        preserved = self.run_cli("search", "needle note")["tasks"][0]
        self.assertEqual(preserved["due_date"], tomorrow)
        self.assertEqual(preserved["priority"], 3)
        self.run_cli("edit", task["id"], "Renamed", "--project", project["id"],
                     "--list", task_list["id"], "--note", "Needle note",
                     "--due", "none", "--scheduled", "none", "--priority", "0")
        cleared = self.run_cli("search", "renamed")["tasks"][0]
        self.assertIsNone(cleared["due_date"])
        self.assertIsNone(cleared["scheduled_date"])
        self.assertEqual(cleared["priority"], 0)

    def test_planning_rejects_invalid_values(self):
        self.run_cli("add", "Bad", "--due", "2026-2-01", code=1)
        self.run_cli("add", "Bad", "--scheduled", "tomorrow", code=1)
        self.run_cli("add", "Bad", "--scheduled", "2026-09-12",
                     "--due", "2026-09-11", code=1)
        self.run_cli("add", "Bad", "--priority", "high", code=1)
        self.run_cli("add", "Bad", "--priority", "4", code=1)
        self.run_cli("today", "extra", code=2)
        self.run_cli("search", code=2)
        self.assertEqual(self.run_cli("list")["tasks"], [])

    def test_archive_restore_and_organization_order(self):
        first = self.run_cli("project-add", "First project")["project"]
        second = self.run_cli("project-add", "Second project")["project"]
        moved = self.run_cli("project-add", "Third project")["project"]
        self.assertTrue(self.run_cli("project-move", moved["id"], "up")["changed"])
        self.assertTrue(self.run_cli("project-move", moved["id"], "up")["changed"])
        self.assertFalse(self.run_cli("project-move", moved["id"], "up")["changed"])
        self.assertEqual([item["id"] for item in self.run_cli("projects")["projects"]],
                         [moved["id"], first["id"], second["id"]])

        first_list = self.run_cli("list-add", moved["id"], "First list")["list"]
        second_list = self.run_cli("list-add", moved["id"], "Second list")["list"]
        moved_list = self.run_cli("list-add", moved["id"], "Third list")["list"]
        self.assertTrue(self.run_cli("list-move", moved_list["id"], "up")["changed"])
        self.assertTrue(self.run_cli("list-move", moved_list["id"], "up")["changed"])
        self.assertEqual([item["id"] for item in self.run_cli("lists", moved["id"])["lists"]],
                         [moved_list["id"], first_list["id"], second_list["id"]])

        task = self.run_cli("add", "Archive target", "--project", moved["id"],
                            "--list", moved_list["id"], "--scheduled", date.today().isoformat())["task"]
        self.assertTrue(self.run_cli("archive", task["id"])["changed"])
        self.assertFalse(self.run_cli("archive", task["id"])["changed"])
        self.assertEqual(self.run_cli("list", "--project", moved["id"])["tasks"], [])
        archived = self.run_cli("list", "--project", moved["id"],
                                "--filter", "archived")["tasks"]
        self.assertEqual(archived[0]["status"], "archived")
        self.assertTrue(archived[0]["archived"])
        self.assertEqual(self.run_cli("today")["tasks"], [])
        self.assertEqual(self.run_cli("search", "archive target")["tasks"][0]["id"], task["id"])
        self.run_cli("complete", task["id"], code=1)
        self.run_cli("edit", task["id"], "Blocked", "--project", moved["id"],
                     "--list", moved_list["id"], "--note", "", code=1)
        self.assertTrue(self.run_cli("restore", task["id"])["changed"])
        self.assertFalse(self.run_cli("restore", task["id"])["changed"])
        self.assertEqual(self.run_cli("today")["tasks"][0]["id"], task["id"])
        self.run_cli("project-move", moved["id"], "sideways", code=1)
        self.run_cli("list-move", moved_list["id"], "sideways", code=1)
        with sqlite3.connect(self.db) as db:
            self.assertEqual(db.execute("SELECT type FROM task_events WHERE task_id=? ORDER BY id",
                                       (task["id"],)).fetchall(),
                             [("created",), ("archived",), ("restored",)])

    def test_tag_create_assign_filter_search_edit_and_clear(self):
        work = self.run_cli("tag-add", " Work ", "--color", "#A1B2C3")["tag"]
        home = self.run_cli("tag-add", "Home", "--color", "#445566")["tag"]
        self.assertEqual(work["name"], "Work")
        self.assertEqual(work["color"], "#a1b2c3")
        self.assertEqual([tag["id"] for tag in self.run_cli("tags")["tags"]],
                         [home["id"], work["id"]])

        tagged = self.run_cli("add", "Tagged CLI task", "--tags",
                              f'{work["id"]},{home["id"]}')["task"]
        self.assertEqual([tag["name"] for tag in tagged["tags"]], ["Home", "Work"])
        self.assertEqual(self.run_cli("list", "--tag", work["id"])["tasks"][0]["id"],
                         tagged["id"])
        self.assertEqual(self.run_cli("search", "work")["tasks"][0]["id"], tagged["id"])

        self.run_cli("edit", tagged["id"], "Renamed tagged task", "--project", "inbox",
                     "--note", "Tags remain when omitted")
        preserved = self.run_cli("list", "--tag", home["id"])["tasks"][0]
        self.assertEqual(len(preserved["tags"]), 2)
        self.assertTrue(self.run_cli("tag-edit", work["id"], "Office",
                                     "--color", "#778899")["changed"])
        self.assertEqual(self.run_cli("search", "office")["tasks"][0]["id"], tagged["id"])
        self.assertEqual(self.run_cli("tags")["tags"][1]["color"], "#778899")

        self.run_cli("edit", tagged["id"], "Renamed tagged task", "--project", "inbox",
                     "--note", "Tags cleared", "--tags", "none")
        self.assertEqual(self.run_cli("list", "--tag", home["id"])["tasks"], [])
        self.assertEqual(self.run_cli("search", "office")["tasks"], [])
        self.run_cli("tag-add", "office", "--color", "#010203", code=1)
        self.run_cli("tag-add", "Bad color", "--color", "red", code=1)
        self.run_cli("add", "Unknown tag", "--tags", "999", code=1)
        self.run_cli("add", "Duplicate tag", "--tags",
                     f'{home["id"]},{home["id"]}', code=1)
        self.run_cli("list", "--tag", "999", code=1)
        self.run_cli("tag-add", "Missing color", code=2)


if __name__ == "__main__":
    unittest.main()
