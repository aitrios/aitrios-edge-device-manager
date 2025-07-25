# Edge Device Manager for Edge Device Core

## Overview

The **Edge Device Manager** is a core component of the [Edge Device Core](https://github.com/aitrios/aitrios-edge-device-core) project. It provides a variety of modules and functions to enable management and operation of edge devices.

### Features
- Define a highly customizable abstract layer for low-level device management in Edge Device Core.
  - **Network Manager**: Provide network configuration support including IP address, gateway, DNS settings.
  - **Firmware Manager**: Provide OTA function for system firmware, AI models, and sensor. 
  - **LED Manager**: Provide LED management function for device status indication.
  - **Memory Manager**: Provide memory management function for device memory.
  - **Log Manager**: Provide log collection and log output function.
  - **Parameter Storage Manager**: Provide data management for system parameters and configurations.
  - **Power Manager**: Provide reboot, shutdown, and factory reset functions.
  - **Clock Manager**: Provide system clock and NTP management function.
  - **Button Manager**: Provide button detection function.
  - **System Manager**: Provide functions of retrieving and setting device system information.
- Define a lightweight HAL (Hardware Abstraction Layer) for the Edge Device Core.
- Provide reference implementations of porting layer for specific hardware platforms, such as Raspberry Pi.


### Supported Environments

-   **Raspberry Pi OS** with the [Raspberry Pi Camera Module](https://www.raspberrypi.com/documentation/accessories/ai-camera.html)

## Building the Application

As a submodule of the **Edge Device Core** project, the Edge Device Manager is not intended to be built as a standalone component. The entire system, including this application, must be built from the top-level `aitrios-edge-device-core` repository.

For complete and detailed build instructions, please consult the [Edge Device Core manual](https://github.com/aitrios/aitrios-edge-device-core).

### Directory Structure

```
.
├── README.md           # This file
├── LICENSE             # Apache 2.0 License
├── CODE_OF_CONDUCT.md  # Community guidelines
├── CONTRIBUTING.md     # Contribution guidelines
├── PrivacyPolicy.md    # Privacy policy
├── SECURITY.md         # Security policy
├── .gitignore          # Git ignore file
├── docs/                # Documentation
├── meson.build         # Meson build configuration
└── src/                # Source code
```

## Contribution

We welcome and encourage contributions to this project! We appreciate bug reports, feature requests, and any other form of community engagement.

-   **Issues and Pull Requests:** Please submit issues and pull requests for any bugs or feature enhancements.
-   **Contribution Guidelines:** For details on how to contribute, please see [CONTRIBUTING.md](CONTRIBUTING.md).
-   **Code of Conduct:** To ensure a welcoming and inclusive community, please review our [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## Security

For information on reporting vulnerabilities and our security policy, please refer to the [SECURITY.md](SECURITY.md) file.

## License

This project is licensed under the Apache License 2.0. For more details, please see the [LICENSE](LICENSE) file.
