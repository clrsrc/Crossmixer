#include "WasapiLoopbackCapture.h"
#include <avrt.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

// Helper to safely release COM objects
template <class T>
static void safeRelease(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

WasapiLoopbackCapture::WasapiLoopbackCapture()
{
}

WasapiLoopbackCapture::~WasapiLoopbackCapture()
{
    stop();
    safeRelease(enumerator);
}

std::vector<AudioDeviceInfo> WasapiLoopbackCapture::enumerateDevices()
{
    std::vector<AudioDeviceInfo> devices;

    // Initialize COM for this call if needed
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hr) || hr == S_FALSE;

    IMMDeviceEnumerator* tempEnum = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void**>(&tempEnum));

    if (FAILED(hr) || tempEnum == nullptr)
    {
        if (comInitialized) CoUninitialize();
        return devices;
    }

    IMMDeviceCollection* collection = nullptr;
    hr = tempEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);

    if (SUCCEEDED(hr) && collection != nullptr)
    {
        UINT count = 0;
        collection->GetCount(&count);

        for (UINT i = 0; i < count; ++i)
        {
            IMMDevice* device = nullptr;
            if (SUCCEEDED(collection->Item(i, &device)) && device != nullptr)
            {
                LPWSTR idStr = nullptr;
                if (SUCCEEDED(device->GetId(&idStr)) && idStr != nullptr)
                {
                    AudioDeviceInfo info;
                    info.id = juce::String(idStr);
                    CoTaskMemFree(idStr);

                    IPropertyStore* props = nullptr;
                    if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props)) && props != nullptr)
                    {
                        PROPVARIANT varName;
                        PropVariantInit(&varName);
                        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &varName)))
                        {
                            if (varName.vt == VT_LPWSTR)
                                info.name = juce::String(varName.pwszVal);
                            PropVariantClear(&varName);
                        }
                        props->Release();
                    }

                    if (info.name.isEmpty())
                        info.name = info.id;

                    devices.push_back(info);
                }
                device->Release();
            }
        }
        collection->Release();
    }

    tempEnum->Release();
    if (comInitialized) CoUninitialize();

    return devices;
}

bool WasapiLoopbackCapture::start(const juce::String& deviceId, RingBuffer<float>& outputBuffer)
{
    if (running.load())
        stop();

    shouldStop.store(false);
    deviceLost.store(false);

    // Launch capture in a separate thread so COM apartment is properly set up
    running.store(true);

    captureThread = std::thread([this, deviceId, &outputBuffer]()
    {
        // Initialize COM as MTA on this thread
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != S_FALSE)
        {
            running.store(false);
            return;
        }

        // Set thread to Pro Audio for MMCSS scheduling
        DWORD taskIndex = 0;
        HANDLE hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

        // Create device enumerator
        IMMDeviceEnumerator* localEnum = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                              CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                              reinterpret_cast<void**>(&localEnum));

        if (FAILED(hr) || localEnum == nullptr)
        {
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        // Get the device by ID
        IMMDevice* device = nullptr;
        hr = localEnum->GetDevice(deviceId.toWideCharPointer(), &device);

        if (FAILED(hr) || device == nullptr)
        {
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        // Activate audio client
        IAudioClient* localClient = nullptr;
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                               reinterpret_cast<void**>(&localClient));
        device->Release();

        if (FAILED(hr) || localClient == nullptr)
        {
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        // Get the mix format
        WAVEFORMATEX* mixFormat = nullptr;
        hr = localClient->GetMixFormat(&mixFormat);
        if (FAILED(hr) || mixFormat == nullptr)
        {
            safeRelease(localClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        sampleRate = static_cast<int>(mixFormat->nSamplesPerSec);
        numChannels = static_cast<int>(mixFormat->nChannels);
        bitsPerSample = static_cast<int>(mixFormat->wBitsPerSample);

        // Initialize in shared mode with loopback flag
        // Buffer duration: 50ms (in 100-nanosecond units)
        REFERENCE_TIME bufferDuration = 500000; // 50ms
        hr = localClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK,
            bufferDuration,
            0,
            mixFormat,
            nullptr
        );

        if (FAILED(hr))
        {
            CoTaskMemFree(mixFormat);
            safeRelease(localClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        // Get the capture client
        IAudioCaptureClient* localCapture = nullptr;
        hr = localClient->GetService(__uuidof(IAudioCaptureClient),
                                      reinterpret_cast<void**>(&localCapture));
        if (FAILED(hr) || localCapture == nullptr)
        {
            CoTaskMemFree(mixFormat);
            safeRelease(localClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        // Determine if the format is float or integer
        bool isFloat = false;
        if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        {
            isFloat = true;
        }
        else if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            auto* ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mixFormat);
            if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
                isFloat = true;
        }

        int captureChannels = numChannels;

        // Start the audio client
        hr = localClient->Start();
        if (FAILED(hr))
        {
            CoTaskMemFree(mixFormat);
            safeRelease(localCapture);
            safeRelease(localClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        CoTaskMemFree(mixFormat);

        // Capture loop: poll every 5ms
        std::vector<float> conversionBuffer;

        while (!shouldStop.load())
        {
            UINT32 packetLength = 0;
            hr = localCapture->GetNextPacketSize(&packetLength);

            while (SUCCEEDED(hr) && packetLength > 0 && !shouldStop.load())
            {
                BYTE* data = nullptr;
                UINT32 numFramesAvailable = 0;
                DWORD flags = 0;

                hr = localCapture->GetBuffer(&data, &numFramesAvailable, &flags, nullptr, nullptr);
                if (FAILED(hr))
                    break;

                if (numFramesAvailable > 0)
                {
                    bool isSilent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;

                    // We need to output 2-channel (stereo) to the ring buffer
                    // even if capture has more channels, we downmix to stereo
                    size_t outFrames = numFramesAvailable;
                    conversionBuffer.resize(outFrames * 2);

                    if (isSilent)
                    {
                        std::fill(conversionBuffer.begin(), conversionBuffer.end(), 0.0f);
                    }
                    else if (isFloat)
                    {
                        const float* src = reinterpret_cast<const float*>(data);
                        for (size_t i = 0; i < outFrames; ++i)
                        {
                            if (captureChannels >= 2)
                            {
                                conversionBuffer[i * 2]     = src[i * captureChannels];     // L
                                conversionBuffer[i * 2 + 1] = src[i * captureChannels + 1]; // R
                            }
                            else
                            {
                                // Mono: duplicate to both channels
                                conversionBuffer[i * 2]     = src[i * captureChannels];
                                conversionBuffer[i * 2 + 1] = src[i * captureChannels];
                            }
                        }
                    }
                    else
                    {
                        // 16-bit integer PCM
                        const int16_t* src = reinterpret_cast<const int16_t*>(data);
                        for (size_t i = 0; i < outFrames; ++i)
                        {
                            if (captureChannels >= 2)
                            {
                                conversionBuffer[i * 2]     = static_cast<float>(src[i * captureChannels])     / 32768.0f;
                                conversionBuffer[i * 2 + 1] = static_cast<float>(src[i * captureChannels + 1]) / 32768.0f;
                            }
                            else
                            {
                                float val = static_cast<float>(src[i * captureChannels]) / 32768.0f;
                                conversionBuffer[i * 2]     = val;
                                conversionBuffer[i * 2 + 1] = val;
                            }
                        }
                    }

                    outputBuffer.write(conversionBuffer.data(), static_cast<int>(outFrames));
                }

                localCapture->ReleaseBuffer(numFramesAvailable);
                hr = localCapture->GetNextPacketSize(&packetLength);
            }

            // Stream is dead (device removed, BT profile switch, format change,
            // audio service restart): stop the loop and report it so the GUI
            // can restart the engine. Otherwise we would spin silently forever.
            if (FAILED(hr))
            {
                DBG("WasapiLoopbackCapture: stream lost, hr=0x" + juce::String::toHexString((int) hr));
                deviceLost.store(true);
                break;
            }

            // Sleep ~5ms between polls
            Sleep(5);
        }

        // Cleanup
        localClient->Stop();
        safeRelease(localCapture);
        safeRelease(localClient);
        safeRelease(localEnum);

        if (hTask)
            AvRevertMmThreadCharacteristics(hTask);

        CoUninitialize();
        running.store(false);
    });

    // Wait briefly for the thread to initialize
    for (int i = 0; i < 100 && running.load() && sampleRate == 0; ++i)
        juce::Thread::sleep(10);

    return running.load();
}

void WasapiLoopbackCapture::stop()
{
    shouldStop.store(true);

    if (captureThread.joinable())
        captureThread.join();

    running.store(false);
    sampleRate = 0;
    numChannels = 0;
}
