#include "DeviceChangeNotifier.h"
#include <mmdeviceapi.h>
#include <atomic>
#include <mutex>

// COM object receiving endpoint notifications on an MMDevAPI worker thread.
// It only forwards a signal; all real work happens on the message thread.
class DeviceChangeNotifier::Client : public IMMNotificationClient
{
public:
    explicit Client(juce::AsyncUpdater* t) : target(t) {}

    void detach()
    {
        std::lock_guard<std::mutex> lock(mutex);
        target = nullptr;
    }

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refCount; }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG r = --refCount;
        if (r == 0)
            delete this;
        return r;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (ppv == nullptr)
            return E_POINTER;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient))
        {
            *ppv = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }

        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // IMMNotificationClient
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { signal(); return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override               { signal(); return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override             { signal(); return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

private:
    void signal()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (target != nullptr)
            target->triggerAsyncUpdate();
    }

    std::atomic<ULONG> refCount{1};
    std::mutex mutex;
    juce::AsyncUpdater* target;
};

DeviceChangeNotifier::DeviceChangeNotifier(std::function<void()> onChange)
    : callback(std::move(onChange))
{
    IMMDeviceEnumerator* enumPtr = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumPtr));
    if (FAILED(hr) || enumPtr == nullptr)
    {
        DBG("DeviceChangeNotifier: could not create device enumerator");
        return;
    }

    client = new Client(this);
    if (FAILED(enumPtr->RegisterEndpointNotificationCallback(client)))
    {
        DBG("DeviceChangeNotifier: could not register notification callback");
        client->detach();
        client->Release();
        client = nullptr;
        enumPtr->Release();
        return;
    }

    enumerator = enumPtr;
}

DeviceChangeNotifier::~DeviceChangeNotifier()
{
    if (client != nullptr)
    {
        client->detach();

        if (enumerator != nullptr)
            static_cast<IMMDeviceEnumerator*>(enumerator)->UnregisterEndpointNotificationCallback(client);

        client->Release();
        client = nullptr;
    }

    if (enumerator != nullptr)
    {
        static_cast<IMMDeviceEnumerator*>(enumerator)->Release();
        enumerator = nullptr;
    }

    cancelPendingUpdate();
}

void DeviceChangeNotifier::handleAsyncUpdate()
{
    if (callback)
        callback();
}
