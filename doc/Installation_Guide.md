## Installation Guide

**OS: ubuntu 22.04**

### 1. Install OMNeT++ 6.0.3

#### 1.1 download OMNeT++

```bash
$ cd ~
$ wget https://github.com/omnetpp/omnetpp/releases/download/omnetpp-6.0.3/omnetpp-6.0.3-linux-x86_64.tgz
$ tar xvfz omnetpp-6.0.3-linux-x86_64.tgz
```

#### 1.2 installing the Prerequisite Packages

```bash
$ sudo apt-get update
$ sudo apt-get install build-essential clang lld gdb bison flex perl \
 python3 python3-pip qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
 libqt5opengl5-dev libxml2-dev zlib1g-dev doxygen graphviz \
 libwebkit2gtk-4.0-37 xdg-utils
$ python3 -m pip install --user --upgrade numpy pandas matplotlib scipy \
 seaborn posix_ipc
$ sudo apt-get install mpi-default-dev
```

#### 1.3 configure and build OMNeT++

```bash
$ cd ~/omnetpp-6.0.3
$ source setenv
```

- To set the environment variables permanently, edit `.bashrc` in your home directory and add a line something like this (note: adjust the `OMNETPP_ROOT` path based on your actual `omnetpp` path):

  ```bash
  export OMNETPP_ROOT="$HOME/omnetpp-6.0.3"
  [ -f "${OMNETPP_ROOT}/setenv" ] && source "${OMNETPP_ROOT}/setenv"
  ```

Next, set `WITH_OSG=no` in `~/omnetpp-6.0.3/configure.user` to disable 3D view in Qtenv, then,

```bash
$ ./configure
$ make -j8
```

#### 1.4 post-installation steps (setting up debugging) 

By default, Ubuntu does not allow ptracing of non-child processes by non-root users. That is, if you want to be able to debug simulation processes by attaching to them with a debugger, or similar, you want to be able to use OMNeT++ just-in-time debugging (`debugger-attach-on-startup` and `debugger-attach-on-error` configuration options), you need to explicitly enable them. 

To temporarily allow ptracing non-child processes, enter the following command: 

```bash
$ echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```

To permanently allow it, edit `/etc/sysctl.d/10-ptrace.conf` and change the line: 

```
kernel.yama.ptrace_scope = 1
```

 to read

```
kernel.yama.ptrace_scope = 0
```

**Additional setup if using wsl2 on window**
[WSL2 Setting](./WSL2 Setting.md)



### 2. Install INET4 and Simu5G

**INET version: 4.5.4**

**SIMU5G version: 1.2.3**

#### 2.1 Create a workspace

Create a workspace

```bash
$ mkdir -p simulator
$ cd simulator
```

Add the simulator workspace path into the `.bashrc` file, e.g.,

```bash
export MEC_WORKSPACE="$HOME/simulator"
```



#### 2.2 install inet4

```bash
$ wget https://github.com/inet-framework/inet/releases/download/v4.5.4/inet-4.5.4-src.tgz
$ tar xvfz inet-4.5.4-src.tgz
$ rm inet-4.5.4-src.tgz
$ cd inet4.5
$ source setenv
$ make makefiles
$ make -j8
$ make MODE=debug -j8
```

Note: 

- `make makefiles` to generate the makefiles (in `src/`).

- `make` to build the inet executable (release version). Use `make MODE=debug` to build debug version.

#### 2.3 install simu5g

Before installing `simu5g`, make sure `inet4.5` has been installed in the working directory.

```bash
$ cd ~/simulator
$ wget https://github.com/Unipisa/Simu5G/archive/refs/tags/v1.2.3.tar.gz
$ tar xvfz v1.2.3.tar.gz
$ mv Simu5G-1.2.3 simu5g
$ rm v1.2.3.tar.gz
$ cd simu5g
$ source setenv
$ make makefiles
$ make -j8
$ make MODE=debug -j8
```

### 3. Install Gurobi

```bash
$ cd ~
$ wget https://packages.gurobi.com/12.0/gurobi12.0.3_linux64.tar.gz
$ tar xvfz gurobi12.0.3_linux64.tar.gz
```

#### 3.1 License

##### 3.1.1 using WSL2 on window

> [How do I set up Gurobi in WSL2 (Windows Subsystem for Linux)? – Gurobi Help Center](https://support.gurobi.com/hc/en-us/articles/7367019222929-How-do-I-set-up-Gurobi-in-WSL2-Windows-Subsystem-for-Linux)

- using [**Web License** **Service**](https://www.gurobi.com/features/web-license-service/) 

- download the WLS license from browser, save it to a folder (e.g., Downloads)

- copy the license from windows file system into wsl2 home (also make it non-executable)

  ```bash
  $ cd ~
  $ cp /mnt/c/Users/CHUANCHAO/Downloads/gurobi.lic ~/
  $ chmod 644 gurobi.lic
  ```

##### 3.1.2 using normal Linux system

download the single machine individual Gurobi License

#### 3.2 Add into Path

Add Gurobi into path. Add the following into `~/.bashrc` file

```bash
# add gurobi into path
export GUROBI_HOME="/path/to/gurobi1203/linux64"
export PATH="${PATH}:${GUROBI_HOME}/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
export GRB_LICENSE_FILE=/path/to/gurobi.lic
```

Verify if the license is valid:

```bash
$ gurobi_cl --license
```

