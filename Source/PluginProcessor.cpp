// =============================================================================
//  PLEXIS Labs — Charlie
//  PluginProcessor.cpp
// =============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

CharlieProcessor::CharlieProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CharlieState", createParameterLayout())
{
    for (auto* id : allParameterIds()) pointers.add (apvts, id);
}

bool CharlieProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void CharlieProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    scratchR.assign ((size_t) juce::jmax (samplesPerBlock, 4096), 0.0f);
    engine.prepare ((float) sampleRate);
    setLatencySamples (plexis::SynthEngine::getLatencySamples());
    keyboardState.reset();
}

void CharlieProcessor::handleMidi (const juce::MidiMessage& m)
{
    if      (m.isNoteOn())         engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())        engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())     engine.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        if      (cc == 1)  engine.setModWheel ((float) m.getControllerValue() / 127.0f);
        else if (cc == 64) engine.setSustain (m.getControllerValue() >= 64);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff (m.isAllSoundOff());
}

void CharlieProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // clavier à l'écran -> fusionné dans le flux MIDI
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;

    fillSynthParams (pointers, synthParams, bpm);
    engine.setParams (synthParams);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R == nullptr)   // sortie mono (rare) : canal droit temporaire préalloué
    {
        if ((int) scratchR.size() < numSamples) scratchR.resize ((size_t) numSamples);
        R = scratchR.data();
    }

    // rendu précis à l'échantillon entre les événements MIDI
    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos) { engine.render (L + pos, R + pos, evPos - pos); pos = evPos; }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples) engine.render (L + pos, R + pos, numSamples - pos);

    if (buffer.getNumChannels() == 1)
        for (int i = 0; i < numSamples; ++i) L[i] = 0.5f * (L[i] + R[i]);
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, numSamples);

    activeVoices.store (engine.getNumActiveVoices());
}

// ---- Programmes (banque d'usine) ----------------------------------------------
int CharlieProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

const juce::String CharlieProcessor::getProgramName (int index)
{
    const auto& p = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? juce::String (p[(size_t) index].name) : juce::String();
}

void CharlieProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentProgram = index;

    // 1) tout aux valeurs par défaut, 2) valeurs du preset
    for (auto* id : allParameterIds())
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->getDefaultValue());

    for (const auto& [id, value] : presets[(size_t) index].values)
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

// ---- État ------------------------------------------------------------------------
void CharlieProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void CharlieProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = vt.getProperty ("program", 0);
            apvts.replaceState (vt);

            // Projets sauvegardés avec la v1.0.0 : GLIDE / LFO DELAY pouvaient contenir NaN.
            for (auto* prm : getParameters())
                if (! std::isfinite (prm->getValue()))
                    prm->setValueNotifyingHost (prm->getDefaultValue());
        }
}

juce::AudioProcessorEditor* CharlieProcessor::createEditor() { return new CharlieEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CharlieProcessor(); }
