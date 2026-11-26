// =============================================================================
//  PLEXIS Labs — Charlie
//  PluginProcessor.h
// =============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/SynthEngine.h"

class CharlieProcessor : public juce::AudioProcessor
{
public:
    CharlieProcessor();
    ~CharlieProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;    // clavier visuel de l'éditeur

    int getActiveVoiceCount() const { return activeVoices.load(); }

private:
    void handleMidi (const juce::MidiMessage& m);

    plexis::SynthEngine engine;
    plexis::SynthParams synthParams;
    ParamPointers pointers;
    std::vector<float> scratchR;
    int currentProgram = 0;
    std::atomic<int> activeVoices { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CharlieProcessor)
};
