# SoundLinuxAgent

Linux Sound Agent (SoundLinuxAgent) monitors audio devices under Linux and publishes their state changes to RabbitMQ for delivery to a REST API server.

## What It Does

- Collects audio device information at startup and monitors device changes.
- Publishes device events to RabbitMQ.
- Integrates with the Audio Device Repository stack:
   [audio-device-repo-server](https://github.com/collect-sound-devices/audio-device-repo-server/),
   [list-audio-react-app](https://github.com/collect-sound-devices/list-audio-react-app/),
   [Primary Web Client](https://list-audio-react-app.vercel.app).

## Deployment

### Prerequisites

- Docker Engine and the Docker Compose plugin installed on the target Linux host

### Instructions

1. Create a deployment folder and download `docker-compose.yml` from the latest release assets into it:
   [Release](https://github.com/eduarddanziger/SoundLinuxAgent/releases/latest)
2. Start the service:

   ```bash
   docker compose up -d
   ```

3. Install the latest version of the RMQ to REST API Forwarder by following the instructions in
   [rmq-to-rest-api-forwarder](https://github.com/collect-sound-devices/rmq-to-rest-api-forwarder/).


## Developer Build

### Requirements

- **C++20 compatible compiler**
- **CMake 3.29 or higher**
- **Ninja build system**
- **vcpkg** package manager
- **rmqcpp** for publishing to RabbitMQ
- **PulseAudio server**
- **Poco** for Linux service lifecycle support

### Prerequisites

- Linux build tools installed: `gcc`, `g++`, `cmake` 3.29+, `ninja`, and `pkg-config`
- PulseAudio development files installed, for example `libpulse-dev` on Debian/Ubuntu
- `vcpkg` installed and bootstrapped
- `VCPKG_ROOT` configured, for example:
   ```bash
   export VCPKG_ROOT=/path/to/vcpkg
   ```

### Instructions

1. Clone the repository.
2. Configure the project:

   ```bash
   cmake --preset linux-debug
   ```

3. Build the project:

   ```bash
   cmake --build --preset linux-debug
   ```

## Configuration

### Environment Variables and RabbitMQ

- `TRANSPORT_METHOD` selects the transport mode. Supported values are `RabbitMQ` and `None`, the default is `RabbitMQ`
<br><br>Set `TRANSPORT_METHOD` to `None`, if you want the scanner not top send requests to RabbitMQ but only log them:
   ```bash
   export TRANSPORT_METHOD=None
   ```

- `RMQ_HOST` sets the RabbitMQ host name used by the scanner when `TRANSPORT_METHOD=RabbitMQ`, the default is `localhost`.
<br><br>In [deploy-via-docker/docker-compose.yml](deploy-via-docker/docker-compose.yml), it is set to `rabbitmq` via the container environment.

- `RMQ_USER` sets the RabbitMQ user name used by the scanner when `TRANSPORT_METHOD=RabbitMQ`, the default is `guest`.

- `RMQ_PASSWORD` sets the RabbitMQ password used by the scanner when `TRANSPORT_METHOD=RabbitMQ`, the default is `guest`.

## Changelog

- 2026-03-29 CLI executable and DEB file removed. **LinuxSoundScanner** is distributed via Docker Compose.
- 2026-03-22 Sent confirm events.
- 2026-03-22 Renamed the main executable to LinuxSoundScanner.
- 2026-03-07 Simplified CMake presets and build steps.

## License

This project is licensed under the terms of the [MIT License](LICENSE).

## Contact

Eduard Danziger

Email: [edanziger@gmx.de](mailto:edanziger@gmx.de)

