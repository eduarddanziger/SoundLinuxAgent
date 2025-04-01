# SoundLinuxAgent

#### Sound Agent Agent detects and visualizes audio endpoint devices under Linux using PulseAudio (C++). It handles audio notifications and device changes.

#### The Sound Agent Service collects audio device information and sends it to a remote server.

## Requirements

- **C++20 compatible compiler**
- **CMake 3.29 or higher**
- **Ninja build system**
- **PulseAudio running server**

## Building

1. **Clone the repository**
2. **Create a build directory and navigate into it, run CMake and compile the project:**

   ```bash
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   ```

## Installation

### To install SoundLinuxAgent using the generated DEB package:

1. **Build the project and generate the DEB package:**

   ```bash
   mkdir -p build
   cd build
   cmake ..
   cmake --build .
   ```
2. **Install the package:**

   ```bash
   sudo dpkg -i SoundLinuxAgent-x.x.x.deb
   ```
3. **If there are missing dependencies, fix them by running:**

   ```bash
   sudo apt-get install -f
   ```

### Alternatively, you can install directly from the build directory

## License

### This project is licensed under the terms of the [MIT License](LICENSE).

## Contact

### For questions or suggestions, please contact:

- **Eduard Danziger**
- **Email: [edanziger@gmx.de](mailto:edanziger@gmx.de)**

