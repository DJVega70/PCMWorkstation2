// Smoke test of the real plugin processor with MIDI. Not shipped in the plugin.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
static float runBlocks(PcmSynthProcessor& pr, juce::MidiBuffer midi, int blocks, int& sampleCount)
{
    juce::AudioBuffer<float> b(2, 512); float pk = 0;
    for (int k = 0; k < blocks; ++k)
    {
        juce::MidiBuffer m; if (k == 0) m = midi;
        pr.processBlock(b, m); if (k > 2) pk = std::max(pk, b.getMagnitude(0, 512)); sampleCount += 512;
    }
    return pk;
}
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    PcmSynthProcessor pr; pr.setPlayConfigDetails(0, 2, 48000, 512); pr.prepareToPlay(48000, 512);
    auto set = [&](const char* id, float v) { auto* p = pr.apvts.getParameter(id); p->setValueNotifyingHost(p->convertTo0to1(v)); };
    int n = 0;
    juce::MidiBuffer on; on.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
    std::printf("program mode, note on -> peak %.3f (expect > 0.02)\n", runBlocks(pr, on, 60, n));
    juce::MidiBuffer off; off.addEvent(juce::MidiMessage::allNotesOff(1), 0); runBlocks(pr, off, 400, n);
    set("mode", 1.0f); set("combi", 1.0f);   // combination 1: bass/piano split
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    std::printf("combi 1 loaded: part1 prog = %d (expect 17), part2 prog = %d (expect 0), part1 hi key = %d (expect 47)\n",
        juce::roundToInt(pr.apvts.getRawParameterValue("p1prog")->load()), juce::roundToInt(pr.apvts.getRawParameterValue("p2prog")->load()),
        juce::roundToInt(pr.apvts.getRawParameterValue("p1hi")->load()));
    juce::MidiBuffer lo; lo.addEvent(juce::MidiMessage::noteOn(1, 40, (juce::uint8) 100), 0);
    { const float pk = runBlocks(pr, lo, 30, n); std::printf("split, low note -> peak %.3f, voices %d (expect 1)\n", pk, pr.voicesActive.load()); }
    runBlocks(pr, off, 400, n);
    juce::MidiBuffer hi; hi.addEvent(juce::MidiMessage::noteOn(1, 72, (juce::uint8) 100), 0);
    runBlocks(pr, hi, 10, n); std::printf("split, high note -> voices %d (expect 1)\n", pr.voicesActive.load());
    runBlocks(pr, off, 400, n);
    juce::MemoryBlock mb; pr.getStateInformation(mb);
    PcmSynthProcessor p2; p2.setStateInformation(mb.getData(), (int) mb.getSize());
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    std::printf("state restore keeps edited parts: p1prog = %d (expect 17), state %zu bytes\n", juce::roundToInt(p2.apvts.getRawParameterValue("p1prog")->load()), mb.getSize());
    return 0;
}
