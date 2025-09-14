## WSL2 Configuration

### 1. Configure all GUI apps using WSLg

Add the following to the **end of your `~/.bashrc`**:

```bash
# Force WSLg environment
export DISPLAY=:0
export WAYLAND_DISPLAY=wayland-0
export XDG_RUNTIME_DIR=/mnt/wslg/runtime-dir
export QT_QPA_PLATFORM=wayland
```

If you also want `sudo` GUI apps to work, add these variables to `sudoers`:

```
sudo visudo
```

Add:

```
Defaults env_keep += "DISPLAY WAYLAND_DISPLAY XDG_RUNTIME_DIR QT_QPA_PLATFORM"
```

### 2. Configure all GUI apps using X11

Add the following to the **end of your `~/.bashrc`**:

```bash
# --- Force all GUI apps to X11 + scaling ---
# GTK apps (Firefox, Gedit, etc.)
export GDK_BACKEND=x11
export GDK_SCALE=1
export GDK_DPI_SCALE=1
# Qt apps (OMNeT++, Sublime, etc.)
export QT_QPA_PLATFORM=xcb
# export QT_AUTO_SCREEN_SCALE_FACTOR=0
# export QT_ENABLE_HIGHDPI_SCALING=0
# export QT_SCALE_FACTOR=1
```

(optional) If you also want `sudo` GUI apps to work, add these variables to `sudoers`:

```
sudo visudo
```

Add:

```
Defaults	env_keep += "QT_QPA_PLATFORM GDK_BACKEND GDK_SCALE GDK_DPI_SCALE DISPLAY XAUTHORITY"
```

