# TrafficMonitor NVML Plugin

An independent Windows x64 plugin for TrafficMonitor that adds NVIDIA GPU utilization and dedicated video memory usage. Use TrafficMonitor's built-in CPU and RAM items to display all four together.

The plugin reads the first NVIDIA GPU reported by the installed driver through NVML. It reads at TrafficMonitor's sampling interval and displays `--` when a reading is unavailable. It has no managed runtime or separate background process. VRAM is the percentage of dedicated memory reported used by NVML.

## Build

Open an x64 Visual Studio Developer PowerShell with the C++ build tools and Windows SDK. Install the NVIDIA CUDA Toolkit for its NVML header, or pass the directory containing that header. Get the TrafficMonitor V1.86 source and pass its `include` directory.

```powershell
./build.ps1 -TrafficMonitorInclude path/to/TrafficMonitor/include
```

The DLL is written to `build/NvidiaTaskbarMetrics.dll`. Add `-Probe` to build and run a local check of both NVIDIA readings. Each probe reading must be available for it to pass.

## Install

1. Exit TrafficMonitor x64 and copy the DLL to its `plugins` directory.
2. Launch TrafficMonitor and show its taskbar window.
3. Open the taskbar window's display settings. Select **CPU**, **Memory**, **NVIDIA GPU utilization**, and **NVIDIA video memory usage**. TrafficMonitor controls left-edge placement, Windows light/dark presets, and sign-in startup.

Built against TrafficMonitor 1.86 and checked with the included probe on Windows 11 and an RTX 4060 Laptop GPU. The CUDA Toolkit is only a build dependency. The installed NVIDIA driver must provide `nvml.dll` at runtime.

## License

The plugin source uses the MIT license. Build headers are obtained separately from [TrafficMonitor V1.86](https://github.com/zhongyang219/TrafficMonitor/tree/V1.86/include) and the [NVIDIA CUDA Toolkit](https://developer.nvidia.com/cuda-toolkit).
