# Migration from Arduino IDE

1. Install VS Code and the **PlatformIO IDE** extension.
2. Open the repository's `cat2.code-workspace`.
3. Select environment `megaatmega2560`.
4. Use Build, then (only when authorised) Upload, then Monitor.

The current code is `cat2/src` and `cat2/include`; PlatformIO resolves the pinned dependencies from `cat2/platformio.ini`. The older `sketches/main` tabs are legacy evidence of working hardware and must not be kept as a parallel firmware branch.
