#include "PresetManager.h"

PresetManager::PresetManager()
{
    presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile("Crossmixer")
                    .getChildFile("Presets");

    presetDir.createDirectory();
}

juce::File PresetManager::getPresetDirectory() const
{
    return presetDir;
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    auto files = presetDir.findChildFiles(juce::File::findFiles, false, "*.xml");

    for (const auto& f : files)
        names.add(f.getFileNameWithoutExtension());

    names.sort(false);
    return names;
}

bool PresetManager::savePreset(const juce::String& name, const juce::ValueTree& state)
{
    auto file = presetDir.getChildFile(name + ".xml");
    auto xml = state.createXml();

    if (xml != nullptr)
        return xml->writeTo(file);

    return false;
}

juce::ValueTree PresetManager::loadPreset(const juce::String& name)
{
    auto file = presetDir.getChildFile(name + ".xml");

    if (!file.existsAsFile())
        return {};

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr)
        return juce::ValueTree::fromXml(*xml);

    return {};
}

bool PresetManager::deletePreset(const juce::String& name)
{
    auto file = presetDir.getChildFile(name + ".xml");
    return file.deleteFile();
}

void PresetManager::ensureDefaultPreset()
{
    if (getPresetNames().isEmpty())
    {
        // Create a flat preset
        juce::ValueTree state("CrossmixerPreset");

        for (int ch = 0; ch < 6; ++ch)
        {
            juce::ValueTree channel("Channel");
            channel.setProperty("index", ch, nullptr);
            channel.setProperty("volume", 0.0, nullptr);
            channel.setProperty("muted", false, nullptr);
            state.appendChild(channel, nullptr);
        }

        state.setProperty("masterVolume", 0.0, nullptr);

        savePreset("Flat", state);
    }
}
