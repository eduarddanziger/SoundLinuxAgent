# SoundLinuxAgent

Linux Sound Agent (SoundLinuxAgent) monitors audio devices under Linux and publishes their state changes to RabbitMQ for delivery to a REST API server.

## Functions
- The SoundLinuxAgent collects audio device information on startup and subscribes to its changes.
- It forms the respective request-messages and pushes them to RabbitMQ channel.
- The separate RMQ To REST API Forwarder (.NET service module) fetches the request-messages, transforms them to the REST API format (POST and PUT) and sends them to the 
Audio Device Repository Server (ASP.Net Core) [audio-device-repo-server](https://github.com/collect-sound-devices/audio-device-repo-server/) with a React / TypeScript frontend [list-audio-react-app](https://github.com/collect-sound-devices/list-audio-react-app/), see [Primary Web Client](https://list-audio-react-app.vercel.app) application.

## Executables Generated
- **LinuxSoundScanner**: Linux executable collects audio device information and sends it to a remote server.
- **SoundLinuxCli**: Command-line test CLI.

## Used Technologies and Requirements
- **C++20 compatible compiler**
- **CMake 3.29 or higher**
- **Ninja build system**
- **vcpkg** with `VCPKG_ROOT` set to your local vcpkg installation path
- **rmqcpp** provided by the repo's vcpkg manifest
- **PulseAudio server**
- **Poco package** leverages Linux executable life cycle code.

## Developer Build

### Prerequisites

- Linux build tools installed: `gcc`, `g++`, `cmake` 3.29+, `ninja`, and `pkg-config`
- PulseAudio development files installed, for example `libpulse-dev` on Debian/Ubuntu
- `vcpkg` installed and bootstrapped:
   - Clone `vcpkg` to `/path/to/vcpkg`
   - Run `./bootstrap-vcpkg.sh`
- Ensure `/path/to/vcpkg` is included into PATH and VCPKG_ROOT configured:
   ```bash
   export VCPKG_ROOT=/path/to/vcpkg
   ```
### Instructions

1. Clone the repository
2. Configure the project:

   ```bash
   cmake --preset linux-debug
   ```

3. Build the project:

   ```bash
   cmake --build --preset linux-debug
   ```

## Installation

In order to install LinuxSoundScanner using the generated DEB package:

1. Install a DEB-package from latest GitHub-Release, [here](https://github.com/eduarddanziger/SoundLinuxAgent/releases/latest).
   ```bash
   sudo dpkg -i SoundLinuxAgent-x.x.x-Linux-LinuxSoundScanner.deb
   ```
2. If there are missing dependencies, fix them by running:

   ```bash
   sudo apt-get install -f
   sudo apt-get install -y libpoco-dev
   ```
3. Install the latwest version of the RmqToRestApiForwarder (distributed via docker-compose, together with RabbitMQ event brocker), following the installation instructions of [rmq-to-rest-api-forwarder](https://github.com/collect-sound-devices/rmq-to-rest-api-forwarder/) repository.


## Starting

- Start it as a daemon or put the following command into your systemd service file:
   ```bash
   /usr/bin/LinuxSoundScanner --daemon --pid-file=/tmp/LinuxSoundScanner.pid
   ```
- To stop the daemon, call kill -TERM < pid >
- LinuxSoundScanner can be started as console app, too. Stop it via Ctrl-C
- The --help option brings a command line help screen with all available options.

## Changelog

- 2026-03-22 Sent confirm events.
- 2026-03-22 Renamed the main executable to LinuxSoundScanner.
- 2026-03-07 Simplified CMake presets and build steps.
- 2025-04-23 Split the build into CLI and daemon targets with DEB packages.

## License

This project is licensed under the terms of the [MIT License](LICENSE).

## Contact

Eduard Danziger

Email: [edanziger@gmx.de](mailto:edanziger@gmx.de)

