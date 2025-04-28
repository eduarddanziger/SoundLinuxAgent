# SoundLinuxAgent

Sound Agent detects and visualizes audio endpoint devices under Linux using PulseAudio (C++). It handles audio notifications and device changes.

The Sound Agent collects audio device information and sends it to a remote server.**

## Executables Generated
- **SoundLinuxDaemon**: Linux Daemon collects audio device information and sends it to a remote server.
- **SoundLinuxCli**: Command-line test CLI.

## ## Technologies Used and Requirements
- **C++20 compatible compiler**
- **CMake 3.29 or higher**
- **Ninja build system**
- **PulseAudio server**
- **Poco and cpprestsdk packages** leverage Linux Daemon life cycle and utilize HTTP REST client code.

## Building

1. Clone the repository
2. Create a build directory and navigate into it, run CMake and compile the project:

   ```bash
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   ```

## Installation

In order to install SoundLinuxDaemon using the generated DEB package:

1. Install a DEB-package from latest GitHub-Release:
   ```bash
   sudo dpkg -i SoundLinuxAgent-x.x.x-Linux-SoundLinuxDaemon.deb
   ```
2. If there are missing dependencies, fix them by running:

   ```bash
   sudo apt-get install -f
   ```
Alternatively, you can install directly from the build directory

## Starting

- Start it as a daemon or put the following command into your systemd service file:
   ```bash
   /usr/bin/SoundLinuxDaemon --daemon --pid-file=/tmp/SoundLinuxDaemon.pid
   ```
- To stop the daemon, call kill -TERM < pid >
- SoundLinuxDaemon can be started as console app, too. Stop it via Ctrl-C
- SoundLinuxDaemon accepts an optional command line parameter, that can tune the URL of the backend ASP.Net Core REST API Server, e.g.:
	```bash
	/usr/bin/SoundLinuxDaemon --url=http://localhost:5027
	```
- The --help option brings a command line help screen with all available options.

## License

This project is licensed under the terms of the [MIT License](LICENSE).

## Contact

Eduard Danziger

Email: [edanziger@gmx.de](mailto:edanziger@gmx.de)

