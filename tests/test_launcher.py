import importlib.util
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "launcher"))
import core

class LauncherTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.game = self.root / "game with spaces; no shell"
        (self.game / "Data/GUI").mkdir(parents=True)
        for name in ("Fonts.trb", "TexSec.trb"):
            (self.game / "Data/GUI" / name).write_bytes(b"synthetic fixture")

    def test_candidate(self):
        self.assertTrue(core.inspect_folder(self.game)["candidate"])
        self.assertEqual(core.resolve_asset(self.game, "DATA\\gui\\fonts.trb"), self.game / "Data/GUI/Fonts.trb")

    def test_installer_is_not_data(self):
        archive = self.root / "game.iso"
        archive.touch()
        self.assertFalse(core.inspect_folder(archive)["candidate"])

    def test_missing_empty_collision(self):
        (self.game / "Data/GUI/Fonts.trb").write_bytes(b"")
        self.assertFalse(core.inspect_folder(self.game)["candidate"])
        (self.game / "Data/GUI/fonts.trb").write_bytes(b"other")
        with self.assertRaises(ValueError):
            core.resolve_asset(self.game, "data/gui/fonts.trb")

    def test_escape_and_slashes(self):
        for path in ("/etc/passwd", "../outside", "C:\\Data\\file", "data/../Data", "a\0b"):
            with self.assertRaises(ValueError):
                core.resolve_asset(self.game, path)
        (self.game / "escape").symlink_to(self.root, target_is_directory=True)
        with self.assertRaises(ValueError):
            core.resolve_asset(self.game, "escape")

    def test_windows_runtime_rejected(self):
        runtime = self.root / "Barnyard.exe"
        runtime.write_bytes(b"MZ" + bytes(100))
        runtime.chmod(0o755)
        self.assertIn("ELF", core.native_error(runtime))
        with self.assertRaises(RuntimeError):
            core.launch_spec(self.game, runtime)

    def test_amd64_runtime_rejected(self):
        runtime=self.root/"wrong-architecture"
        header=bytearray(64);header[:6]=b"\x7fELF\x02\x01"
        struct.pack_into("<HH",header,16,3,62)
        runtime.write_bytes(header);runtime.chmod(0o755)
        self.assertIn("i386",core.native_error(runtime))

    def test_missing_runtime(self):
        with self.assertRaises(RuntimeError):
            core.launch_spec(self.game, self.root / "missing")

    def test_config_roundtrip_and_corruption(self):
        with patch.dict(os.environ, {"XDG_CONFIG_HOME": str(self.root / "config")}):
            core.save_folder(self.game)
            self.assertEqual(core.read_folder(), str(self.game))
            self.assertEqual(core.config_file().stat().st_mode & 0o777, 0o600)
            core.config_file().write_text("{broken")
            self.assertEqual(core.read_folder(), "")
            core.config_file().write_text("[]")
            self.assertEqual(core.read_folder(), "")

    def test_xdg_reject_relative_override(self):
        with patch.dict(os.environ, {"XDG_DATA_HOME": "relative-directory"}):
            self.assertTrue(core.xdg_dir("XDG_DATA_HOME", ".local/share").is_absolute())

    def test_launch_spec_uses_user_cwd(self):
        # Header-only fixture to exercise launcher argument construction.
        # Never executed, packaged, or treated as evidence of a game runtime.
        runtime=self.root/"header-fixture"
        header=bytearray(64);header[:6]=b"\x7fELF\x01\x01"
        struct.pack_into("<HH",header,16,3,3)
        runtime.write_bytes(header);runtime.chmod(0o755)
        with patch.dict(os.environ, {"XDG_DATA_HOME": str(self.root / "user"),
                                     "XDG_STATE_HOME": str(self.root / "state"),
                                     "XDG_CACHE_HOME": str(self.root / "cache")}):
            argv, cwd, env, log = core.launch_spec(self.game, runtime)
            self.assertEqual(argv, [str(runtime.resolve())])
            self.assertNotEqual(cwd, self.game)
            self.assertEqual(env["BARNYARD_GAME_DIR"], str(self.game))
            self.assertEqual(log.parent, self.root / "state/barnyard-linux")

if __name__ == "__main__":
    unittest.main(verbosity=2)
