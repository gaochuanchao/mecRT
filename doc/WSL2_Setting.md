## WSL2 Configuration

### Setting up GUI apps in WSL2
By default, WSL2 supports GUI apps using WSLg (Wayland). However, some GUI apps may not work well with WSLg's scaling features. This section provides instructions to configure all GUI apps to use vcXsrv.

#### Setting up vcXsrv
```bash
WindowMode="MultiWindow" Display="-1" ClientMode="NoClient" Clipboard="True" ClipboardPrimary="True" Wgl="True" DisableAC="True" 
```

#### Configuring GUI apps setting in .bashrc

```bash
# For GTK apps (Firefox, Gedit, etc.)
export GDK_BACKEND=x11
export GDK_SCALE=2  # for high-DPI displays, set to 1 for normal DPI
export GDK_DPI_SCALE=1
# For Qt apps (OMNeT++, Sublime, etc.)
export QT_QPA_PLATFORM=xcb

# --- using VcXsrv ---
export DISPLAY=$(grep nameserver /etc/resolv.conf | awk '{print $2}'):0.0
export LIBGL_ALWAYS_INDIRECT=1
export LIBGL_ALWAYS_SOFTWARE=true
export QT_WA_DontShowOnScreen=1
```

(optional) If you also want `sudo` GUI apps to work, add these variables to `sudoers`:

```
sudo visudo
```

Add:

```
Defaults env_keep += "DISPLAY LIBGL_ALWAYS_INDIRECT LIBGL_ALWAYS_SOFTWARE QT_WA_DontShowOnScreen GDK_BACKEND GDK_SCALE GDK_DPI_SCALE QT_QPA_PLATFORM XAUTHORITY"
```


### matplotlib configuration in WSL2
To use matplotlib in WSL2 with vcXsrv, you need to set the backend to 'Qt5Agg'. 

Add the following lines to your matplotlib configuration file (`~/.config/matplotlib/matplotlibrc`):

```
backend : Qt5Agg
```

or in the beginning of your Python script before importing `matplotlib.pyplot`:

```python
import matplotlib
matplotlib.use('Qt5Agg')
```

Make sure you have the necessary Qt5 packages installed in your WSL2 environment:

```bash
sudo apt-get install python3-pyqt5
```
This configuration ensures that matplotlib uses the Qt5 backend, which is compatible with vcXsrv for rendering plots.
