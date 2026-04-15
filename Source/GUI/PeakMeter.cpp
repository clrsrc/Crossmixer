#include "PeakMeter.h"
#include "CrossmixerLookAndFeel.h"

PeakMeter::PeakMeter()
{
    startTimerHz(30);
}

PeakMeter::~PeakMeter()
{
    stopTimer();
}

void PeakMeter::setLevelSource(std::function<float()> source)
{
    levelSource = std::move(source);
}

void PeakMeter::setPeakLevel(float level)
{
    currentLevel = level;
}

void PeakMeter::timerCallback()
{
    if (levelSource)
        currentLevel = levelSource();

    // Ballistic decay
    if (currentLevel > displayLevel)
        displayLevel = currentLevel; // fast attack
    else
        displayLevel *= decayRate;   // slow release

    // Peak hold
    if (currentLevel > peakHold)
    {
        peakHold = currentLevel;
        peakHoldCounter = peakHoldFrames;
    }
    else if (peakHoldCounter > 0)
    {
        --peakHoldCounter;
    }
    else
    {
        peakHold *= decayRate;
    }

    repaint();
}

void PeakMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    // Background
    g.setColour(CrossmixerLookAndFeel::faderTrackColour);
    g.fillRoundedRectangle(bounds, 2.0f);

    // Meter bar (from bottom to level)
    float meterHeight = bounds.getHeight() * displayLevel;
    if (meterHeight > 0.5f)
    {
        auto meterBounds = bounds.withTop(bounds.getBottom() - meterHeight);

        // Gradient: green at bottom, yellow at middle, red at top
        juce::ColourGradient gradient(
            CrossmixerLookAndFeel::meterGreenColour, bounds.getBottomLeft(),
            CrossmixerLookAndFeel::meterRedColour, bounds.getTopLeft(),
            false
        );
        gradient.addColour(0.6, CrossmixerLookAndFeel::meterYellowColour);

        g.setGradientFill(gradient);
        g.fillRoundedRectangle(meterBounds, 2.0f);
    }

    // Peak hold indicator
    if (peakHold > 0.01f)
    {
        float peakY = bounds.getBottom() - bounds.getHeight() * peakHold;
        g.setColour(CrossmixerLookAndFeel::meterRedColour.withAlpha(0.8f));
        g.fillRect(bounds.getX(), peakY, bounds.getWidth(), 2.0f);
    }
}
