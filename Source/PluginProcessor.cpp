#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::String noteName(int n) { static const char* nm[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return juce::String(nm[n % 12]) + juce::String(n / 12 - 2); }
}

juce::AudioProcessorValueTreeState::ParameterLayout PcmSynthProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using I = juce::AudioParameterInt;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto id = [](const juce::String& s) { return juce::ParameterID { s, 1 }; };

    l.add(std::make_unique<C>(id("mode"), "Mode", juce::StringArray { "Program", "Combination", "Multi" }, 0));
    {
        juce::StringArray combis;
        for (auto& c : pcm::combinations()) combis.add(c.name);
        l.add(std::make_unique<C>(id("combi"), "Combination", combis, 0));
    }
    juce::StringArray progNames;
    for (auto& pr : pcm::programs()) progNames.add(pr.name);
    for (int i = 1; i <= 8; ++i)
    {
        const juce::String s(i);
        l.add(std::make_unique<C>(id("p" + s + "prog"), "Part " + s + " Program", progNames, 0));
        l.add(std::make_unique<F>(id("p" + s + "lvl"), "Part " + s + " Level", R(0.0f, 1.0f, 0.01f), i == 1 ? 0.8f : 0.0f));
        l.add(std::make_unique<F>(id("p" + s + "pan"), "Part " + s + " Pan", R(-1.0f, 1.0f, 0.01f), 0.0f));
        l.add(std::make_unique<I>(id("p" + s + "lo"), "Part " + s + " Low Key", 0, 127, 0, juce::AudioParameterIntAttributes().withStringFromValueFunction([](int v, int) { return noteName(v); })));
        l.add(std::make_unique<I>(id("p" + s + "hi"), "Part " + s + " High Key", 0, 127, 127, juce::AudioParameterIntAttributes().withStringFromValueFunction([](int v, int) { return noteName(v); })));
        l.add(std::make_unique<I>(id("p" + s + "tr"), "Part " + s + " Transpose", -24, 24, 0));
        l.add(std::make_unique<I>(id("p" + s + "ch"), "Part " + s + " MIDI Channel", 1, 16, i));
    }

    // tone macros
    l.add(std::make_unique<F>(id("cutoff"), "Brightness", R(-1.0f, 1.0f, 0.01f), 0.0f));
    l.add(std::make_unique<F>(id("res"), "Resonance", R(0.0f, 0.6f, 0.01f), 0.0f));
    l.add(std::make_unique<F>(id("attack"), "Attack Time", R(0.1f, 6.0f, 0.0f, 0.5f), 1.0f));
    l.add(std::make_unique<F>(id("release"), "Release Time", R(0.1f, 6.0f, 0.0f, 0.5f), 1.0f));
    l.add(std::make_unique<F>(id("lfo"), "LFO Depth", R(0.0f, 3.0f, 0.01f), 1.0f));
    l.add(std::make_unique<I>(id("bend"), "Bend Range", 1, 12, 2));

    // effects
    juce::StringArray fxNames;
    for (int i = 0; i < pcm::FxCount; ++i) fxNames.add(pcm::fxName(i));
    for (int i = 1; i <= 2; ++i)
    {
        const juce::String s(i);
        l.add(std::make_unique<C>(id("fx" + s + "type"), "FX" + s + " Type", fxNames, i == 1 ? 1 : 3));
        l.add(std::make_unique<F>(id("fx" + s + "a"), "FX" + s + " A", R(0.0f, 1.0f, 0.01f), 0.5f));
        l.add(std::make_unique<F>(id("fx" + s + "b"), "FX" + s + " B", R(0.0f, 1.0f, 0.01f), 0.5f));
        l.add(std::make_unique<F>(id("fx" + s + "mix"), "FX" + s + " Mix", R(0.0f, 1.0f, 0.01f), i == 1 ? 0.18f : 0.0f));
    }

    l.add(std::make_unique<F>(id("master"), "Master", R(-24.0f, 6.0f, 0.1f), 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    l.add(std::make_unique<F>(id("joyx"), "Joystick X (Bend)", R(-1.0f, 1.0f, 0.001f), 0.0f));
    l.add(std::make_unique<F>(id("joyy"), "Joystick Y (Mod)", R(0.0f, 1.0f, 0.001f), 0.0f));
    return l;
}

PcmSynthProcessor::PcmSynthProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    apvts.addParameterListener("combi", this);
}

PcmSynthProcessor::~PcmSynthProcessor() { apvts.removeParameterListener("combi", this); cancelPendingUpdate(); }

bool PcmSynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PcmSynthProcessor::parameterChanged(const juce::String& id, float v)
{
    if (id == "combi" && !restoring.load())
    {
        pendingCombi = juce::roundToInt(v);
        triggerAsyncUpdate();
    }
}

void PcmSynthProcessor::handleAsyncUpdate()
{
    const int c = pendingCombi.exchange(-1);
    if (c >= 0) loadCombination(c);
}

void PcmSynthProcessor::loadCombination(int index)
{
    const auto& cb = pcm::combinations()[(size_t) juce::jlimit(0, pcm::kNumCombis - 1, index)];
    auto setP = [this](const juce::String& id, float real) {
        if (auto* prm = apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1(real)); };
    for (int i = 0; i < 8; ++i)
    {
        const juce::String s(i + 1); const auto& pc = cb.part[i];
        setP("p" + s + "prog", (float) pc.prog); setP("p" + s + "lvl", pc.level); setP("p" + s + "pan", pc.pan);
        setP("p" + s + "lo", (float) pc.lo); setP("p" + s + "hi", (float) pc.hi);
        setP("p" + s + "tr", (float) pc.transpose); setP("p" + s + "ch", (float) pc.channel);
    }
}

void PcmSynthProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate);
    applyParameters();
    keyboardState.reset();
}

void PcmSynthProcessor::applyParameters()
{
    auto& g = engine.g;
    g.mode = juce::roundToInt(p("mode"));
    g.masterDb = p("master");
    g.cutoffOffset = p("cutoff"); g.resOffset = p("res");
    g.attackScale = p("attack"); g.releaseScale = p("release"); g.lfoScale = p("lfo");
    g.bendRange = (float) juce::roundToInt(p("bend"));
    engine.joyX = p("joyx"); engine.joyY = p("joyy");
    for (int i = 0; i < 8; ++i)
    {
        const juce::String s(i + 1); auto& pc = engine.parts[(size_t) i];
        pc.prog = juce::roundToInt(p("p" + s + "prog")); pc.level = p("p" + s + "lvl"); pc.pan = p("p" + s + "pan");
        pc.lo = juce::roundToInt(p("p" + s + "lo")); pc.hi = juce::roundToInt(p("p" + s + "hi"));
        pc.transpose = juce::roundToInt(p("p" + s + "tr")); pc.channel = juce::roundToInt(p("p" + s + "ch"));
    }
    for (int i = 0; i < 2; ++i)
    {
        const juce::String s(i + 1);
        engine.fx[i].set(juce::roundToInt(p("fx" + s + "type")), p("fx" + s + "a"), p("fx" + s + "b"), p("fx" + s + "mix"));
    }
}

void PcmSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2 || n == 0) return;

    keyboardState.processNextMidiBuffer(midi, 0, n, true);   // on-screen keyboard -> same stream
    applyParameters();

    float* l = buffer.getWritePointer(0);
    float* r = buffer.getWritePointer(1);
    int pos = 0;
    auto handle = [this](const juce::MidiMessage& m) {
        const int ch = m.getChannel() - 1;
        if (m.isNoteOn()) engine.noteOn(ch, m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) engine.noteOff(ch, m.getNoteNumber());
        else if (m.isPitchWheel()) engine.pitchBend(ch, (float) (m.getPitchWheelValue() - 8192) / 8192.0f);
        else if (m.isController())
        {
            const int cc = m.getControllerNumber(), v = m.getControllerValue();
            if (cc == 1) engine.modWheel(ch, (float) v / 127.0f);
            else if (cc == 64) engine.pedalChange(ch, v >= 64);
            else if (cc == 120 || cc == 123) engine.allNotesOff();
        } };
    for (const auto meta : midi)
    {
        const int at = juce::jlimit(pos, n, meta.samplePosition);
        if (at > pos) { engine.process(l + pos, r + pos, at - pos); pos = at; }
        handle(meta.getMessage());
    }
    if (pos < n) engine.process(l + pos, r + pos, n - pos);

    outPeak = juce::jmax(buffer.getMagnitude(0, 0, n), buffer.getMagnitude(1, 0, n));
    voicesActive = engine.activeVoices();
}

juce::AudioProcessorEditor* PcmSynthProcessor::createEditor() { return new PcmSynthEditor(*this); }

void PcmSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, destData);
}

void PcmSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
        {
            restoring = true;                      // do not let the restored "combi" overwrite restored parts
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            restoring = false;
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PcmSynthProcessor(); }
