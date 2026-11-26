#include "JuceHeader.h"
#include "MainWindow.h"
#include <windows.h>

class CrossmixerApplication : public juce::JUCEApplication
{
public:
    CrossmixerApplication() = default;

    const juce::String getApplicationName() override    { return "Crossmixer"; }
    const juce::String getApplicationVersion() override { return "1.0.1"; }
    bool moreThanOneInstanceAllowed() override           { return false; }

    void initialise(const juce::String& /*commandLine*/) override
    {
        registerAutoStart();
        mainWindow = std::make_unique<MainWindow>("Crossmixer");
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& /*commandLine*/) override
    {
    }

private:
    std::unique_ptr<MainWindow> mainWindow;

    void registerAutoStart()
    {
        auto exePath = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();

        HKEY hKey = nullptr;
        LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                     0, KEY_SET_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
            auto widePath = exePath.toWideCharPointer();
            RegSetValueExW(hKey, L"Crossmixer", 0, REG_SZ,
                           reinterpret_cast<const BYTE*>(widePath),
                           static_cast<DWORD>((wcslen(widePath) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    }
};

START_JUCE_APPLICATION(CrossmixerApplication)
