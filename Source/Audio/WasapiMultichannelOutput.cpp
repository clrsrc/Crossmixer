#include "WasapiMultichannelOutput.h"
#include "WasapiLoopbackCapture.h" // for AudioDeviceInfo
#include <avrt.h>
#include <ks.h>
#include <ksmedia.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

template <class T>
static void safeRelease(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

WasapiMultichannelOutput::WasapiMultichannelOutput()
{
}

WasapiMultichannelOutput::~WasapiMultichannelOutput()
{
    stop();
}

std::vector<AudioDeviceInfo> WasapiMultichannelOutput::enumerateDevices()
{
    std::vector<AudioDeviceInfo> devices;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hr) || hr == S_FALSE;

    IMMDeviceEnumerator* localEnum = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void**>(&localEnum));

    if (FAILED(hr) || localEnum == nullptr)
    {
        if (comInitialized) CoUninitialize();
        return devices;
    }

    IMMDeviceCollection* collection = nullptr;
    hr = localEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);

    if (SUCCEEDED(hr) && collection != nullptr)
    {
        UINT count = 0;
        collection->GetCount(&count);

        for (UINT i = 0; i < count; ++i)
        {
            IMMDevice* device = nullptr;
            if (FAILED(collection->Item(i, &device)) || device == nullptr)
                continue;

            // List all render devices — format negotiation happens at start()
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
        collection->Release();
    }

    localEnum->Release();
    if (comInitialized) CoUninitialize();

    return devices;
}

bool WasapiMultichannelOutput::start(const juce::String& deviceId, RingBuffer<float>& inputBuffer)
{
    if (running.load())
        stop();

    shouldStop.store(false);
    running.store(true);

    outputThread = std::thread([this, deviceId, &inputBuffer]()
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != S_FALSE)
        {
            running.store(false);
            return;
        }

        DWORD taskIndex = 0;
        HANDLE hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

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

        IAudioClient* audioClient = nullptr;
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                               reinterpret_cast<void**>(&audioClient));
        device->Release();

        if (FAILED(hr) || audioClient == nullptr)
        {
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        // Get the device's mix format
        WAVEFORMATEX* mixFormat = nullptr;
        hr = audioClient->GetMixFormat(&mixFormat);
        if (FAILED(hr) || mixFormat == nullptr)
        {
            safeRelease(audioClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        DWORD deviceSampleRate = mixFormat->nSamplesPerSec;
        sampleRate = static_cast<int>(deviceSampleRate);

        // Strategy: try 8ch, 6ch, then fall back to device mix format
        WAVEFORMATEXTENSIBLE wfx6 = {};
        wfx6.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx6.Format.nChannels = 6;
        wfx6.Format.nSamplesPerSec = deviceSampleRate;
        wfx6.Format.wBitsPerSample = 32;
        wfx6.Format.nBlockAlign = 6 * 4;
        wfx6.Format.nAvgBytesPerSec = deviceSampleRate * 6 * 4;
        wfx6.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        wfx6.Samples.wValidBitsPerSample = 32;
        wfx6.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
        wfx6.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

        // Also try 8-channel (7.1) since some USB cards report 8ch
        WAVEFORMATEXTENSIBLE wfx8 = wfx6;
        wfx8.Format.nChannels = 8;
        wfx8.Format.nBlockAlign = 8 * 4;
        wfx8.Format.nAvgBytesPerSec = deviceSampleRate * 8 * 4;
        wfx8.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;

        WAVEFORMATEX* formatToUse = nullptr;
        WAVEFORMATEX* closestMatch = nullptr;
        int outputChannels = 0;
        bool needFreeFormat = false;

        // Try 1: exact 6-channel
        hr = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                             reinterpret_cast<WAVEFORMATEX*>(&wfx6),
                                             &closestMatch);
        if (hr == S_OK)
        {
            formatToUse = reinterpret_cast<WAVEFORMATEX*>(&wfx6);
            outputChannels = 6;
        }
        else
        {
            if (closestMatch) { CoTaskMemFree(closestMatch); closestMatch = nullptr; }

            // Try 2: 8-channel
            hr = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                                 reinterpret_cast<WAVEFORMATEX*>(&wfx8),
                                                 &closestMatch);
            if (hr == S_OK)
            {
                formatToUse = reinterpret_cast<WAVEFORMATEX*>(&wfx8);
                outputChannels = 8;
            }
            else
            {
                if (closestMatch) { CoTaskMemFree(closestMatch); closestMatch = nullptr; }

                // Try 3: use the device's mix format as-is
                formatToUse = mixFormat;
                outputChannels = mixFormat->nChannels;
                needFreeFormat = false; // mixFormat will be freed later
            }
        }

        // Create event for buffer notifications
        HANDLE hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        // Initialize audio client — try event-driven first, fall back to polling
        REFERENCE_TIME bufferDuration = 500000; // 50ms
        hr = audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            bufferDuration,
            0,
            formatToUse,
            nullptr
        );

        // If event-driven fails, try without event callback
        bool useEventDriven = SUCCEEDED(hr);
        if (!useEventDriven)
        {
            // Need a new audio client since Initialize can only be called once
            safeRelease(audioClient);
            IMMDevice* device2 = nullptr;
            localEnum->GetDevice(deviceId.toWideCharPointer(), &device2);
            if (device2)
            {
                device2->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                   reinterpret_cast<void**>(&audioClient));
                device2->Release();
            }

            if (audioClient != nullptr)
            {
                hr = audioClient->Initialize(
                    AUDCLNT_SHAREMODE_SHARED,
                    0,
                    bufferDuration,
                    0,
                    formatToUse,
                    nullptr
                );
            }
        }

        CoTaskMemFree(mixFormat);

        if (FAILED(hr) || audioClient == nullptr)
        {
            CloseHandle(hEvent);
            safeRelease(audioClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        if (useEventDriven)
            audioClient->SetEventHandle(hEvent);

        numOutputChannels.store(outputChannels);

        // Get buffer size
        UINT32 bufferFrameCount = 0;
        audioClient->GetBufferSize(&bufferFrameCount);

        // Get render client
        IAudioRenderClient* renderClient = nullptr;
        hr = audioClient->GetService(__uuidof(IAudioRenderClient),
                                      reinterpret_cast<void**>(&renderClient));

        if (FAILED(hr) || renderClient == nullptr)
        {
            CloseHandle(hEvent);
            safeRelease(audioClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        // Pre-fill with silence
        {
            BYTE* pData = nullptr;
            hr = renderClient->GetBuffer(bufferFrameCount, &pData);
            if (SUCCEEDED(hr))
                renderClient->ReleaseBuffer(bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
        }

        // Start playback
        hr = audioClient->Start();
        if (FAILED(hr))
        {
            safeRelease(renderClient);
            CloseHandle(hEvent);
            safeRelease(audioClient);
            safeRelease(localEnum);
            if (hTask) AvRevertMmThreadCharacteristics(hTask);
            CoUninitialize();
            running.store(false);
            return;
        }

        std::vector<float> readBuffer;

        // Output loop — event-driven or polling
        while (!shouldStop.load())
        {
            if (useEventDriven)
                WaitForSingleObject(hEvent, 100);
            else
                Sleep(5);

            UINT32 numFramesPadding = 0;
            audioClient->GetCurrentPadding(&numFramesPadding);

            UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
            if (numFramesAvailable == 0)
                continue;

            BYTE* pData = nullptr;
            hr = renderClient->GetBuffer(numFramesAvailable, &pData);
            if (FAILED(hr))
                continue;

            // Read from ring buffer (6ch from engine)
            // If device has more or fewer channels, we remap
            readBuffer.resize(numFramesAvailable * 6);
            int framesRead = inputBuffer.read(readBuffer.data(),
                                               static_cast<int>(numFramesAvailable));

            if (framesRead > 0)
            {
                float* outFloat = reinterpret_cast<float*>(pData);

                for (int i = 0; i < framesRead; ++i)
                {
                    // Map 6-channel engine data to device output channels
                    for (int ch = 0; ch < outputChannels; ++ch)
                    {
                        if (ch < 6)
                            outFloat[i * outputChannels + ch] = readBuffer[i * 6 + ch];
                        else
                            outFloat[i * outputChannels + ch] = 0.0f; // extra channels silent
                    }
                }

                // Fill remainder with silence
                for (UINT32 i = static_cast<UINT32>(framesRead); i < numFramesAvailable; ++i)
                {
                    for (int ch = 0; ch < outputChannels; ++ch)
                        outFloat[i * outputChannels + ch] = 0.0f;
                }

                renderClient->ReleaseBuffer(numFramesAvailable, 0);
            }
            else
            {
                renderClient->ReleaseBuffer(numFramesAvailable, AUDCLNT_BUFFERFLAGS_SILENT);
            }
        }

        // Cleanup
        audioClient->Stop();
        safeRelease(renderClient);
        CloseHandle(hEvent);
        safeRelease(audioClient);
        safeRelease(localEnum);

        if (hTask)
            AvRevertMmThreadCharacteristics(hTask);

        CoUninitialize();
        running.store(false);
    });

    // Wait briefly for initialization
    for (int i = 0; i < 100 && running.load() && sampleRate == 0; ++i)
        juce::Thread::sleep(10);

    return running.load();
}

void WasapiMultichannelOutput::stop()
{
    shouldStop.store(true);

    if (outputThread.joinable())
        outputThread.join();

    running.store(false);
    sampleRate = 0;
}
