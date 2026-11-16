#pragma once
#include <JuceHeader.h>
#include "Engine.h"

class PcmSynthProcessor : public juce::AudioProcessor, private juce::AsyncUpdater, private juce::AudioProcessorValueTreeState::Listener
{
public:
    PcmSynthProcessor();
    ~PcmSynthProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PCM Workstation"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    // read by the editor
    std::atomic<float> outPeak { 0.0f };
    std::atomic<int> voicesActive { 0 };

    void loadCombination(int index);   // copies a combination into the 8 part parameters

private:
    void parameterChanged(const juce::String& id, float) override;
    void handleAsyncUpdate() override;
    void applyParameters();
    float p(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }

    pcm::Engine engine;
    juce::MidiBuffer keyboardEvents;
    std::atomic<bool> restoring { false };
    std::atomic<int> pendingCombi { -1 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PcmSynthProcessor)
};
