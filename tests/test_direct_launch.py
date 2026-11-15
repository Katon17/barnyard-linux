"""Launcher persistence/unit checks; no game simulation."""
from pathlib import Path
import os
import sys
import tempfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'launcher'))
import direct_launch as app
with tempfile.TemporaryDirectory() as work:
    base=Path(work)
    os.environ.update(XDG_CONFIG_HOME=str(base/'config'),XDG_DATA_HOME=str(base/'data'))
    game=base/'game';(game/'Data').mkdir(parents=True)
    assert app.remembered_game() is None
    app.remember_game(game)
    assert app.remembered_game()==game
    config=app.config_file()
    assert config.stat().st_mode & 0o077==0
    assert app.user_root()==base/'data/barnyard-linux-test'
    # No accidental rename/new save namespace during upgrade.
    for value in ['',str(base),str(game/'missing')]:
        try:app.validate_game(value)
        except (OSError,ValueError):pass
        else:raise AssertionError(value)
    (app.user_root()/'Data').mkdir(parents=True)
    try:app.validate_game(str(app.user_root()))
    except ValueError:pass
    else:raise AssertionError('game/save overlap')
    config.write_text('{broken')
    assert app.remembered_game() is None
    config.write_text('x'*20000)
    assert app.remembered_game() is None
    config.unlink();config.symlink_to(base/'outside')
    assert app.remembered_game() is None
    config.unlink();config.write_text('{"game_root": 1}')
    assert app.remembered_game() is None
print('PASS: direct-launch folder memory, private config, invalid/stale/oversized/symlink config, unchanged save namespace, overlap rejection')
