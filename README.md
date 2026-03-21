# SoundLinuxAgent

Sound Agent detects and outputs audio endpoint devices under Linux using PulseAudio (C++). It handles audio notifications and device changes.

The Sound Agent registers audio device information on a backend server via REST API, see the backend Audio Device Repository Server (ASP.Net Core) [audio-device-repo-server](https://github.com/collect-sound-devices/audio-device-repo-server/) with a React / TypeScript frontend: [list-audio-react-app](https://github.com/collect-sound-devices/list-audio-react-app/)

## Executables Generated
- **SoundLinuxDaemon**: Linux Daemon collects audio device information and sends it to a remote server.
- **SoundLinuxCli**: Command-line test CLI.

## Used Technologies and Requirements
- **C++20 compatible compiler**
- **CMake 3.29 or higher**
- **Ninja build system**
- **vcpkg** with `VCPKG_ROOT` set to your local vcpkg installation path
- **rmqcpp** installed under `$VCPKG_ROOT/rmqcpp/install` or available via `rmqcpp_DIR`
- **PulseAudio server**
- **Poco and cpprestsdk packages** leverage Linux Daemon life cycle and utilize HTTP REST client code.

## Building

1. Clone the repository
2. Set `VCPKG_ROOT` to your local vcpkg installation path:

   ```bash
   export VCPKG_ROOT=/path/to/vcpkg
   ```

3. Configure the project:

   ```bash
   cmake --preset linux-debug
   ```

4. Build the project:

   ```bash
   cmake --build --preset linux-debug
   ```

## Installation

In order to install SoundLinuxDaemon using the generated DEB package:

1. Install a DEB-package from latest GitHub-Release, [here](https://github.com/eduarddanziger/SoundLinuxAgent/releases/latest).
   ```bash
   sudo dpkg -i SoundLinuxAgent-x.x.x-Linux-SoundLinuxDaemon.deb
   ```
2. If there are missing dependencies, fix them by running:

   ```bash
   sudo apt-get install -f
   sudo apt-get install -y libpoco-dev
   ```
3. Install the latwest version of the RmqToRestApiForwarder (distributed via docker-compose, rogether with RabbitMQ event brocker), following the installation instructions of [rmq-to-rest-api-forwarder](https://github.com/collect-sound-devices/rmq-to-rest-api-forwarder/) repository.


## Starting

- Start it as a daemon or put the following command into your systemd service file:
   ```bash
   /usr/bin/SoundLinuxDaemon --daemon --pid-file=/tmp/SoundLinuxDaemon.pid
   ```
- To stop the daemon, call kill -TERM < pid >
- SoundLinuxDaemon can be started as console app, too. Stop it via Ctrl-C
- The --help option brings a command line help screen with all available options.

## License

This project is licensed under the terms of the [MIT License](LICENSE).

## Contact

Eduard Danziger

Email: [edanziger@gmx.de](mailto:edanziger@gmx.de)

