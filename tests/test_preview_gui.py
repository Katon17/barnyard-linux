"""Run with xvfb-run /usr/bin/python3. Uses real installed Tk/SDL/GLES libraries."""
import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch

# Optional path validates the installed package rather than the source copy.
module_dir = Path(os.environ.get("BARNYARD_PREVIEW_TEST_DIR", Path(__file__).resolve().parents[1] / "launcher"))
sys.path.insert(0, str(module_dir))
import preview

class PreviewGUITests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.rootdir = Path(self.temp.name)
        self.env = patch.dict(os.environ, {"XDG_CONFIG_HOME": str(self.rootdir / "config"),
                                          "XDG_STATE_HOME": str(self.rootdir / "state")})
        self.env.start()
        self.addCleanup(self.env.stop)
        self.root = tk.Tk()
        self.addCleanup(self.root.destroy)
        self.window = preview.PreviewWindow(self.root)
        self.root.update()

    def test_play_is_disabled_and_preview_is_explicit(self):
        self.assertIn("belum bisa main", self.root.title())
        self.assertTrue(self.window.play.instate(["disabled"]))
        self.assertFalse(preview.status_report()["playable"])
        self.assertFalse(preview.status_report()["engine_included"])

    def test_folder_check_persists_but_never_enables_play(self):
        game = self.rootdir / "synthetic game"
        (game / "Data/GUI").mkdir(parents=True)
        for name in ("Fonts.trb", "TexSec.trb"):
            (game / "Data/GUI" / name).write_bytes(b"not a real game asset")
        self.window.folder.set(str(game))
        self.window.check()
        self.root.update()
        self.assertIn("indikator", self.window.message.get())
        self.assertTrue(self.window.play.instate(["disabled"]))
        config = json.loads((self.rootdir / "config/barnyard-linux/launcher.json").read_text())
        self.assertEqual(config["game_dir"], str(game))
        self.window.folder.set(str(self.rootdir / "not-there.iso"))
        self.window.check()
        self.assertIn("Belum cocok", self.window.message.get())

    def test_real_graphics_button_runs_probe(self):
        self.window.start_probe()
        self.assertIsNotNone(self.window.process)
        self.assertTrue(self.window.probe.instate(["disabled"]))
        deadline = time.monotonic() + 20
        while self.window.process is not None and time.monotonic() < deadline:
            self.root.update()
            time.sleep(0.02)
        if self.window.process is not None:
            self.window.process.kill()
            self.window.process.wait()
            self.fail("Graphics subprocess did not complete")
        self.assertIn("Tes GLES3 berhasil", self.window.message.get())
        self.assertTrue(self.window.probe.instate(["!disabled"]))
        report = json.loads(self.window.log.read_text())
        self.assertTrue(report["ok"])
        self.assertGreater(report["frames"], 0)
        self.assertTrue(self.window.play.instate(["disabled"]))

if __name__ == "__main__":
    unittest.main(verbosity=2)
