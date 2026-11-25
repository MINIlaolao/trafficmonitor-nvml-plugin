#include <windows.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <nvml.h>
#include "PluginInterface.h"

namespace {
class Reading final : public IPluginItem {
public:
    Reading(const wchar_t* name, const wchar_t* label, const wchar_t* id)
        : name_(name), label_(label), id_(id) {}

    const wchar_t* GetItemName() const override { return name_; }
    const wchar_t* GetItemId() const override { return id_; }
    const wchar_t* GetItemLableText() const override { return label_; }
    const wchar_t* GetItemValueSampleText() const override { return L"100%"; }

    const wchar_t* GetItemValueText() const override {
        const int percent = percent_.load();
        if (percent < 0)
            return L"--";
        static const auto percentages = [] {
            std::array<std::array<wchar_t, 5>, 101> labels{};
            for (int value = 0; value <= 100; ++value)
                swprintf_s(labels[value].data(), labels[value].size(), L"%d%%", value);
            return labels;
        }();
        return percentages[percent].data();
    }

    void Set(int percent) { percent_.store(percent); }

private:
    const wchar_t* name_;
    const wchar_t* label_;
    const wchar_t* id_;
    std::atomic<int> percent_{-1};
};

class NvidiaMetrics final : public ITMPlugin {
public:
    IPluginItem* GetItem(int index) override {
        if (index == 0) return &gpu_;
        if (index == 1) return &vram_;
        return nullptr;
    }

    const wchar_t* GetInfo(PluginInfoIndex index) override {
        switch (index) {
        case TMI_NAME: return L"NVIDIA Taskbar Metrics";
        case TMI_DESCRIPTION: return L"GPU utilization and dedicated video memory via NVIDIA NVML";
        case TMI_VERSION: return L"1.0";
        default: return L"";
        }
    }

    void OnInitialize(ITrafficMonitor*) override {
        // TrafficMonitor retains plugins until the host exits.
        HMODULE nvml = LoadLibraryExW(L"nvml.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!nvml)
            return;
        auto init = Resolve<decltype(&nvmlInit_v2)>(nvml, "nvmlInit_v2");
        get_device_ = Resolve<decltype(&nvmlDeviceGetHandleByIndex_v2)>(nvml, "nvmlDeviceGetHandleByIndex_v2");
        get_load_ = Resolve<decltype(&nvmlDeviceGetUtilizationRates)>(nvml, "nvmlDeviceGetUtilizationRates");
        get_memory_ = Resolve<decltype(&nvmlDeviceGetMemoryInfo)>(nvml, "nvmlDeviceGetMemoryInfo");
        if (init && get_device_ && get_load_ && get_memory_)
            initialize_ = init;
    }

    void DataRequired() override {
        if (!initialized_) {
            if (!initialize_ || initialize_() != NVML_SUCCESS)
                return;
            initialized_ = true;
        }
        if (!device_ && get_device_(0, &device_) != NVML_SUCCESS) {
            device_ = nullptr;
            return;
        }
        nvmlUtilization_t load{};
        nvmlMemory_t memory{};
        const auto load_status = get_load_(device_, &load);
        const auto memory_status = get_memory_(device_, &memory);
        gpu_.Set(load_status == NVML_SUCCESS && load.gpu <= 100 ? static_cast<int>(load.gpu) : -1);
        vram_.Set(memory_status == NVML_SUCCESS && memory.total && memory.used <= memory.total
            ? static_cast<int>(100.0 * memory.used / memory.total + 0.5) : -1);
        if (load_status != NVML_SUCCESS && memory_status != NVML_SUCCESS)
            device_ = nullptr;
    }

private:
    template <typename Function>
    static Function Resolve(HMODULE module, const char* name) {
        return reinterpret_cast<Function>(GetProcAddress(module, name));
    }

    Reading gpu_{L"NVIDIA GPU utilization", L"GPU: ", L"nvidia-taskbar-metrics.gpu"};
    Reading vram_{L"NVIDIA video memory usage", L"VRAM: ", L"nvidia-taskbar-metrics.vram"};
    decltype(&nvmlInit_v2) initialize_{};
    decltype(&nvmlDeviceGetHandleByIndex_v2) get_device_{};
    decltype(&nvmlDeviceGetUtilizationRates) get_load_{};
    decltype(&nvmlDeviceGetMemoryInfo) get_memory_{};
    nvmlDevice_t device_{};
    bool initialized_{};
};
} // namespace

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance() {
    static NvidiaMetrics plugin;
    return &plugin;
}

#ifdef MONITOR_PROBE
int wmain() {
    ITMPlugin* plugin = TMPluginGetInstance();
    plugin->OnInitialize(nullptr);
    plugin->DataRequired();
    for (int index = 0; index < 2; ++index) {
        const auto* item = plugin->GetItem(index);
        const wchar_t* value = item->GetItemValueText();
        wprintf(L"%ls%ls\n", item->GetItemLableText(), value);
        if (*value == L'-')
            return 1;
    }
    return 0;
}
#endif
