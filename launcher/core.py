"""Launcher model; stdlib only. No downloads, shell invocation, or Wine fallback.
Data checks are heuristics from observed source paths, NOT compatibility proof.
"""
from __future__ import annotations
import json
import os
from pathlib import Path
import struct
import tempfile

ASSET_HINTS = ("data/gui/fonts.trb", "data/gui/texsec.trb")
RUNTIME = Path("/usr/lib/barnyard-linux/barnyard-native")


def xdg_dir(variable: str, fallback: str) -> Path:
    value = os.environ.get(variable, "")
    return (Path(value) if value and Path(value).is_absolute() else Path.home() / fallback) / "barnyard-linux"


def config_file() -> Path:
    return xdg_dir("XDG_CONFIG_HOME", ".config") / "launcher.json"


def read_folder() -> str:
    try:
        value = json.loads(config_file().read_text(encoding="utf8")).get("game_dir", "")
        return value if isinstance(value, str) else ""
    except (OSError, ValueError, AttributeError):
        return ""


def save_folder(folder: Path) -> None:
    destination = config_file()
    destination.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=".launcher-", dir=destination.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf8") as stream:
            json.dump({"schema": 1, "game_dir": str(folder.resolve(strict=True))}, stream)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, destination)
    finally:
        Path(temporary).unlink(missing_ok=True)


def resolve_asset(root: Path, relative: str) -> Path:
    """Read-only ASCII case resolver, shared semantics with C++ AssetPaths.
    Not a security sandbox for concurrently modified, untrusted directories.
    """
    relative = relative.replace("\\", "/")
    if not relative or relative.startswith("/") or ":" in relative or "\0" in relative:
        raise ValueError("Path aset tidak valid")
    parts = relative.split("/")
    if ".." in parts:
        raise ValueError("Parent traversal ditolak")
    root = root.resolve(strict=True)
    current = root
    table = str.maketrans("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz")
    for part in parts:
        if part in ("", "."):
            continue
        matches = [p for p in current.iterdir() if p.name.translate(table) == part.translate(table)]
        if not matches:
            raise FileNotFoundError(relative)
        if len(matches) > 1:
            raise ValueError("Nama file ambigu (beda kapitalisasi): " + part)
        current = matches[0].resolve(strict=True)
        if not current.is_relative_to(root):
            raise ValueError("Symlink aset keluar folder game")
    return current


def inspect_folder(folder: Path) -> dict:
    if not folder.is_dir():
        return {"candidate": False, "issues": ["Pilih folder instalasi PC, bukan ZIP/ISO/EXE."]}
    issues = []
    for hint in ASSET_HINTS:
        try:
            path = resolve_asset(folder, hint)
            if not path.is_file() or path.stat().st_size == 0:
                issues.append("File kosong/bukan file biasa: " + hint)
        except (OSError, ValueError) as exc:
            issues.append(f"{hint}: {exc}")
    return {"candidate": not issues, "issues": issues,
            "note": "Hanya indikator folder PC; kelengkapan dan revisi aset belum diverifikasi."}


def native_error(path: Path) -> str:
    try:
        with path.open("rb") as stream:
            header = stream.read(64)
        if len(header) < 64 or header[:6] != b"\x7fELF\x01\x01":
            return "Runtime bukan ELF Linux i386 32-bit little-endian."
        kind, machine = struct.unpack_from("<HH", header, 16)
        if kind not in (2, 3) or machine != 3:
            return "Runtime bukan executable i386 yang didukung."
        if not os.access(path, os.X_OK):
            return "Runtime tidak memiliki izin executable."
        return ""
    except OSError:
        return "Engine native belum tersedia; build ini belum dapat memainkan Barnyard."


def launch_spec(folder: Path, runtime: Path = RUNTIME):
    """Linux i386 env-v1 contract; actual gameplay remains a separate release gate.
    ELF validation only detects obvious wrong binaries; not a gameplay certification.
    """
    error = native_error(runtime)
    if error:
        raise RuntimeError(error)
    report = inspect_folder(folder)
    if not report["candidate"]:
        raise RuntimeError("\n".join(report["issues"]))
    data = xdg_dir("XDG_DATA_HOME", ".local/share")
    cache = xdg_dir("XDG_CACHE_HOME", ".cache")
    state = xdg_dir("XDG_STATE_HOME", ".local/state")
    for path in (data, cache, state):
        path.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(BARNYARD_GAME_DIR=str(folder.resolve(strict=True)),
               BARNYARD_USER_DIR=str(data), BARNYARD_CACHE_DIR=str(cache))
    # Use user directory as cwd, not the original game's installation.
    return [str(runtime.resolve(strict=True))], data, env, state / "game.log"
