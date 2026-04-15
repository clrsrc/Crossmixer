#pragma once
#include "JuceHeader.h"
#include "GUI/MainComponent.h"
#include <windows.h>

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name)
        : DocumentWindow(name,
                          juce::Colour(0xff1e1e2e),
                          DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(false);
        setContentOwned(new MainComponent(), true);
        setResizable(true, true);
        setResizeLimits(700, 500, 1400, 900);
        centreWithSize(getWidth(), getHeight());

        // Setup system tray icon, then start minimized to tray
        trayIcon = std::make_unique<CrossmixerTrayIcon>(*this);
        setVisible(false);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    void visibilityChanged() override
    {
        DocumentWindow::visibilityChanged();
        if (auto* mc = dynamic_cast<MainComponent*>(getContentComponent()))
            mc->setSpectrumEnabled(isVisible());
    }

    void minimizeToTray()
    {
        setVisible(false);
    }

    void restoreFromTray()
    {
        setVisible(true);
        toFront(true);
    }

private:
    class CrossmixerTrayIcon : public juce::SystemTrayIconComponent
    {
    public:
        CrossmixerTrayIcon(MainWindow& owner) : window(owner)
        {
            // Create icon image - 32x32 for good scaling
            const int sz = 32;
            juce::Image img(juce::Image::ARGB, sz, sz, false);

            // Manually set every pixel to ensure full opacity
            for (int y = 0; y < sz; ++y)
                for (int x = 0; x < sz; ++x)
                    img.setPixelAt(x, y, juce::Colour(0xff2060cc));

            // Draw fader design using setPixelAt for guaranteed visibility
            auto fillBox = [&](int x0, int y0, int w, int h, juce::Colour c)
            {
                for (int yy = y0; yy < y0 + h && yy < sz; ++yy)
                    for (int xx = x0; xx < x0 + w && xx < sz; ++xx)
                        img.setPixelAt(xx, yy, c);
            };

            // Two fader tracks (lighter blue)
            fillBox(10, 4, 3, 24, juce::Colour(0xff90c0ff));
            fillBox(19, 4, 3, 24, juce::Colour(0xff90c0ff));

            // Fader thumbs (white, crossed)
            fillBox(6,  8,  10, 4, juce::Colour(0xffffffff));
            fillBox(15, 20, 10, 4, juce::Colour(0xffffffff));

            setIconImage(img, img);
            setIconTooltip("Crossmixer");
        }

        void mouseUp(const juce::MouseEvent&) override
        {
            if (!window.isVisible())
                window.restoreFromTray();
            else
                window.toFront(true);
        }

    private:
        MainWindow& window;
    };

    std::unique_ptr<CrossmixerTrayIcon> trayIcon;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
