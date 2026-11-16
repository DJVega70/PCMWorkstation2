#include "PluginEditor.h"

namespace
{
    const juce::Colour panelTop { 0xff2b2d33 }, panelBottom { 0xff17181c }, ink { 0xffe9e4d8 }, dim { 0xff9a968c };
    const juce::Colour amber { 0xffffb23e }, lcdBg { 0xff120d05 }, teal { 0xff3fa7a0 }, red { 0xffc8412f },
                       blue { 0xff3f78b8 }, green { 0xff4f9a64 }, slate { 0xff6a707a }, cream { 0xffcfc7b0 };
    constexpr float kPi = juce::MathConstants<float>::pi;

    void screw(juce::Graphics& g, float x, float y)
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillEllipse(x - 5, y - 4, 10, 10);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8b8e94), x - 4, y - 5, juce::Colour(0xff3c3e43), x + 4, y + 5, false));
        g.fillEllipse(x - 5, y - 5, 10, 10);
        g.setColour(juce::Colour(0xff1b1c1f)); g.drawLine(x - 3, y + 1, x + 3, y - 1, 1.4f);
    }
}

//==============================================================================
class SynthLook : public juce::LookAndFeel_V4
{
public:
    SynthLook()
    {
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0d0e10));
        setColour(juce::ComboBox::textColourId, ink);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.15f));
        setColour(juce::ComboBox::arrowColourId, amber);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1b1c20));
        setColour(juce::PopupMenu::textColourId, ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, amber.darker(0.4f));
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff0d0e10));
        setColour(juce::Slider::trackColourId, amber.darker(0.5f));
        setColour(juce::Slider::textBoxTextColourId, ink);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::FontOptions(12.0f); }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float, float startAngle, float endAngle, juce::Slider& s) override
    {
        const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
        const float d = juce::jmin(b.getWidth(), b.getHeight()) - 6.0f;
        const auto c = b.getCentre();
        const float ang = startAngle + (float) s.valueToProportionOfLength(s.getValue()) * (endAngle - startAngle);
        const juce::Colour cap((juce::uint32) (int) s.getProperties()["cap"]);
        g.setColour(dim.withAlpha(0.8f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle), r0 = d * 0.5f + 2.5f, r1 = r0 + (i % 5 == 0 ? 4.5f : 2.5f);
            g.drawLine(c.x + std::sin(a) * r0, c.y - std::cos(a) * r0, c.x + std::sin(a) * r1, c.y - std::cos(a) * r1, 1.0f);
        }
        g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillEllipse(c.x - d * 0.5f + 1, c.y - d * 0.5f + 3, d, d);
        g.setColour(juce::Colour(0xff0c0d0f)); g.fillEllipse(c.x - d * 0.5f, c.y - d * 0.5f, d, d);
        const float cd = d - 6.0f;
        g.setGradientFill(juce::ColourGradient(cap.brighter(0.25f), c.x - cd * 0.3f, c.y - cd * 0.5f, cap.darker(0.55f), c.x + cd * 0.3f, c.y + cd * 0.5f, false));
        g.fillEllipse(c.x - cd * 0.5f, c.y - cd * 0.5f, cd, cd);
        g.setColour(juce::Colours::white.withAlpha(0.18f)); g.drawEllipse(c.x - cd * 0.5f + 0.5f, c.y - cd * 0.5f + 0.5f, cd - 1, cd - 1, 1.0f);
        const float td = cd * 0.62f;
        g.setGradientFill(juce::ColourGradient(cap.brighter(0.05f), c.x, c.y - td * 0.5f, cap.darker(0.3f), c.x, c.y + td * 0.5f, false));
        g.fillEllipse(c.x - td * 0.5f, c.y - td * 0.5f, td, td);
        juce::Path p; p.addRoundedRectangle(-1.4f, -cd * 0.5f + 1.0f, 2.8f, cd * 0.42f, 1.2f);
        g.setColour(juce::Colours::white.withAlpha(0.95f)); g.fillPath(p, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
    }
};

class Knob : public juce::Component
{
public:
    Knob(juce::AudioProcessorValueTreeState& a, const juce::String& id, const juce::String& t, juce::Colour cap,
         std::function<juce::String(double)> f, SynthLook& laf) : title(t.toUpperCase()), fmt(std::move(f))
    {
        slider.setLookAndFeel(&laf); slider.getProperties().set("cap", (int) cap.getARGB());
        slider.setRotaryParameters(kPi * 1.2f, kPi * 2.8f, true); slider.setMouseDragSensitivity(170);
        addAndMakeVisible(slider);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(a, id, slider);
        if (auto* p = a.getParameter(id)) slider.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
        value.setJustificationType(juce::Justification::centred);
        value.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));
        value.setColour(juce::Label::textColourId, ink.withAlpha(0.85f)); value.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(value);
        slider.onValueChange = [this] { value.setText(fmt(slider.getValue()), juce::dontSendNotification); };
        slider.onValueChange();
    }
    void setTitle(const juce::String& t) { title = t.toUpperCase(); repaint(); }
    void paint(juce::Graphics& g) override { g.setColour(dim); g.setFont(juce::FontOptions(10.0f, juce::Font::bold)); g.drawText(title, 0, 0, getWidth(), 13, juce::Justification::centred); }
    void resized() override
    {
        const int d = juce::jmin(getWidth() - 4, getHeight() - 30);
        slider.setBounds(juce::Rectangle<int>(d, d).withCentre({ getWidth() / 2, 14 + d / 2 }));
        value.setBounds(0, getHeight() - 14, getWidth(), 14);
    }
private:
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::String title; juce::Label value; std::function<juce::String(double)> fmt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class ModeButton : public juce::Button
{
public:
    ModeButton(const juce::String& t) : juce::Button(t) {}
    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced(1.0f); const bool on = getToggleState();
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(b.translated(0, 2), 4.0f);
        auto face = down ? b.translated(0, 1) : b;
        g.setColour(juce::Colour(0xff0b0b0d)); g.fillRoundedRectangle(face, 4.0f);
        auto cap = face.reduced(2.5f);
        juce::Colour t(0xff6c7077), bt(0xff383a3f); if (over) { t = t.brighter(0.1f); bt = bt.brighter(0.1f); }
        g.setGradientFill(juce::ColourGradient(t, 0, cap.getY(), bt, 0, cap.getBottom(), false)); g.fillRoundedRectangle(cap, 3.0f);
        auto lamp = juce::Rectangle<float>(cap.getWidth() - 16.0f, 5.0f).withCentre({ cap.getCentreX(), cap.getY() + 7.0f });
        if (on) { g.setColour(amber.withAlpha(0.28f)); g.fillRoundedRectangle(lamp.expanded(4, 3), 5.0f); }
        g.setColour(on ? amber : amber.withAlpha(0.18f)); g.fillRoundedRectangle(lamp, 2.0f);
        g.setColour(ink); g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(getButtonText(), cap.withTrimmedTop(12.0f), juce::Justification::centred);
    }
};

class JoystickPad : public juce::Component, private juce::Timer
{
public:
    JoystickPad(juce::AudioProcessorValueTreeState& a) : px(*a.getParameter("joyx")), py(*a.getParameter("joyy")) { startTimerHz(30); }
    ~JoystickPad() override { stopTimer(); }
    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(b.translated(0, 2), 10.0f);
        g.setColour(juce::Colour(0xff0b0c0e)); g.fillRoundedRectangle(b, 10.0f);
        g.setColour(juce::Colours::white.withAlpha(0.12f)); g.drawRoundedRectangle(b, 10.0f, 1.0f);
        g.setColour(dim.withAlpha(0.5f)); g.drawLine(b.getCentreX(), b.getY() + 8, b.getCentreX(), b.getBottom() - 8); g.drawLine(b.getX() + 8, b.getBottom() - 12, b.getRight() - 8, b.getBottom() - 12);
        g.setColour(dim); g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText("BEND", getLocalBounds().removeFromBottom(14), juce::Justification::centred); g.drawText("MOD", getLocalBounds().withTrimmedTop(4).removeFromTop(12), juce::Justification::centred);
        const float cx = 2.0f + (px.getValue() * 0.5f * 0 + (float) (px.convertFrom0to1(px.getValue()) * 0.5 + 0.5)) * (getWidth() - 4.0f);
        const float cy = 2.0f + (1.0f - (float) py.getValue()) * (getHeight() - 16.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillEllipse(cx - 12 + 1, cy - 12 + 3, 24, 24);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff9b988d), cx - 6, cy - 8, juce::Colour(0xff3a3b3f), cx + 6, cy + 8, false)); g.fillEllipse(cx - 12, cy - 12, 24, 24);
        g.setColour(amber); g.fillEllipse(cx - 4, cy - 4, 8, 8);
    }
    void mouseDown(const juce::MouseEvent& e) override { px.beginChangeGesture(); py.beginChangeGesture(); set(e); }
    void mouseDrag(const juce::MouseEvent& e) override { set(e); }
    void mouseUp(const juce::MouseEvent&) override
    {
        px.setValueNotifyingHost(px.convertTo0to1(0.0f)); py.setValueNotifyingHost(0.0f);   // springs back
        px.endChangeGesture(); py.endChangeGesture();
    }
private:
    void set(const juce::MouseEvent& e)
    {
        const float x = juce::jlimit(0.0f, 1.0f, (float) e.x / (float) getWidth()), y = juce::jlimit(0.0f, 1.0f, 1.0f - (float) e.y / (float) (getHeight() - 12));
        const float sx = std::fabs(x - 0.5f) < 0.04f ? 0.5f : x;   // small dead zone at the centre
        px.setValueNotifyingHost(sx); py.setValueNotifyingHost(y);
    }
    void timerCallback() override { repaint(); }
    juce::RangedAudioParameter &px, &py;
};

//==============================================================================
class PcmSynthEditor::Content : public juce::Component, private juce::Timer
{
public:
    Content(PcmSynthProcessor& pr) : proc(pr), apvts(pr.apvts), joystick(pr.apvts),
        keyboard(pr.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        setLookAndFeel(&laf);
        auto knob = [&](const char* id, const char* t, juce::Colour c, std::function<juce::String(double)> f) {
            knobs.push_back(std::make_unique<Knob>(apvts, id, t, c, std::move(f), laf)); addAndMakeVisible(*knobs.back()); return knobs.back().get(); };
        auto pct = [](double v) { return juce::String(juce::roundToInt(v * 100)) + "%"; };

        // mode buttons
        for (int i = 0; i < 3; ++i)
        {
            static const char* nm[3] = { "PROGRAM", "COMBI", "MULTI" };
            modeBtn[i] = std::make_unique<ModeButton>(nm[i]); addAndMakeVisible(*modeBtn[i]);
            modeBtn[i]->onClick = [this, i] { setChoice("mode", i); };
        }
        // data entry
        data.setSliderStyle(juce::Slider::LinearHorizontal); data.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        data.setRange(0, 99, 1); data.setLookAndFeel(&laf); addAndMakeVisible(data);
        data.onValueChange = [this] { if (!updating) setChoice(currentParamId(), juce::roundToInt(data.getValue())); };
        for (int i = 0; i < 2; ++i)
        {
            step[i].setButtonText(i == 0 ? "-" : "+"); addAndMakeVisible(step[i]);
            step[i].onClick = [this, i] { data.setValue(juce::jlimit(0.0, 99.0, data.getValue() + (i == 0 ? -1 : 1)), juce::sendNotificationSync); };
        }
        knob("master", "Master", cream, [](double v) { return juce::String(v, 1) + " dB"; });

        // parts table
        for (int i = 0; i < 8; ++i)
        {
            const juce::String s(i + 1);
            auto& r = row[(size_t) i];
            r.num.setButtonText(s); r.num.onClick = [this, i] { selPart = i + 1; };
            addAndMakeVisible(r.num);
            for (int k = 0; k < pcm::kNumPrograms; ++k) r.prog.addItem(pcm::programs()[(size_t) k].name, k + 1);
            addAndMakeVisible(r.prog);
            r.progAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "p" + s + "prog", r.prog);
            auto bar = [&](juce::Slider& sl, const juce::String& id, std::function<juce::String(double)> txt, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att) {
                sl.setSliderStyle(juce::Slider::LinearBar); sl.setTextBoxIsEditable(false); sl.setLookAndFeel(&laf);
                addAndMakeVisible(sl);
                att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, sl);
                sl.textFromValueFunction = txt; sl.updateText();   // after the attachment, which installs its own formatter
                if (auto* p = apvts.getParameter(id)) sl.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue())); };
            bar(r.lvl, "p" + s + "lvl", pct, r.lvlAtt);
            bar(r.pan, "p" + s + "pan", [](double v) { const int p = juce::roundToInt(v * 100); return p == 0 ? juce::String("C") : (p < 0 ? "L" : "R") + juce::String(std::abs(p)); }, r.panAtt);
            auto nn = [](double v) { static const char* a[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; const int n = (int) v; return juce::String(a[n % 12]) + juce::String(n / 12 - 2); };
            bar(r.lo, "p" + s + "lo", nn, r.loAtt); bar(r.hi, "p" + s + "hi", nn, r.hiAtt);
            bar(r.tr, "p" + s + "tr", [](double v) { return (v > 0 ? "+" : "") + juce::String((int) v); }, r.trAtt);
            bar(r.ch, "p" + s + "ch", [](double v) { return "ch " + juce::String((int) v); }, r.chAtt);
        }

        // tone macros
        knob("cutoff", "Brightness", amber, [](double v) { return (v > 0 ? "+" : "") + juce::String(v, 2); });
        knob("res", "Resonance", amber, [](double v) { return juce::String(juce::roundToInt(v / 0.6 * 100)) + "%"; });
        knob("attack", "Attack", teal, [](double v) { return "x" + juce::String(v, 2); });
        knob("release", "Release", teal, [](double v) { return "x" + juce::String(v, 2); });
        knob("lfo", "LFO Depth", green, [](double v) { return "x" + juce::String(v, 2); });
        knob("bend", "Bend Range", slate, [](double v) { return juce::String((int) v) + " st"; });

        // effects
        for (int i = 0; i < 2; ++i)
        {
            const juce::String s(i + 1);
            for (int t = 0; t < pcm::FxCount; ++t) fxType[i].addItem(pcm::fxName(t), t + 1);
            addAndMakeVisible(fxType[i]);
            fxAtt[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "fx" + s + "type", fxType[i]);
            fxKnob[i][0] = knob(("fx" + s + "a").toRawUTF8(), "A", blue, [](double v) { return juce::String(juce::roundToInt(v * 100)) + "%"; });
            fxKnob[i][1] = knob(("fx" + s + "b").toRawUTF8(), "B", blue, [](double v) { return juce::String(juce::roundToInt(v * 100)) + "%"; });
            fxKnob[i][2] = knob(("fx" + s + "mix").toRawUTF8(), "Mix", red, [](double v) { return juce::String(juce::roundToInt(v * 100)) + "%"; });
        }

        addAndMakeVisible(joystick);
        keyboard.setAvailableRange(36, 96); keyboard.setKeyWidth(21.8f); keyboard.setScrollButtonsVisible(false);
        keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, amber.withAlpha(0.7f));
        keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, amber.withAlpha(0.25f));
        keyboard.setMidiChannel(1);
        addAndMakeVisible(keyboard);
        startTimerHz(30);
        timerCallback();
    }
    ~Content() override
    {
        stopTimer(); setLookAndFeel(nullptr); data.setLookAndFeel(nullptr);
        for (auto& r : row) for (auto* s : { &r.lvl, &r.pan, &r.lo, &r.hi, &r.tr, &r.ch }) s->setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        g.setGradientFill(juce::ColourGradient(panelTop, 0, 0, panelBottom, 0, (float) getHeight(), false)); g.fillAll();
        juce::Random rng(9); g.setColour(juce::Colours::white.withAlpha(0.02f));
        for (int y = 0; y < getHeight(); y += 2) if (rng.nextInt(3) == 0) g.drawHorizontalLine(y, 0.0f, (float) getWidth());
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRect(getLocalBounds(), 2);
        screw(g, 12, 12); screw(g, getWidth() - 12.0f, 12); screw(g, 12, getHeight() - 12.0f); screw(g, getWidth() - 12.0f, getHeight() - 12.0f);

        // brand
        g.setColour(ink); g.setFont(juce::FontOptions(36.0f, juce::Font::bold)); g.drawText("PCM", 30, 22, 220, 40, juce::Justification::centredLeft);
        g.setColour(amber); g.setFont(juce::FontOptions(14.0f, juce::Font::bold)); g.drawText("WORKSTATION", 32, 60, 220, 18, juce::Justification::centredLeft);
        g.setColour(dim); g.setFont(juce::FontOptions(9.5f)); g.drawText("16 voices  /  8 parts  /  100 programs  /  100 combinations", 32, 80, 240, 14, juce::Justification::centredLeft);

        // LCD
        auto lcd = juce::Rectangle<float>(270, 16, 372, 88);
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillRoundedRectangle(lcd.translated(0, 2), 6.0f);
        g.setGradientFill(juce::ColourGradient(lcdBg.brighter(0.08f), 0, lcd.getY(), lcdBg, 0, lcd.getBottom(), false)); g.fillRoundedRectangle(lcd, 6.0f);
        g.setColour(juce::Colours::white.withAlpha(0.14f)); g.drawRoundedRectangle(lcd, 6.0f, 1.0f);
        g.setColour(amber); g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 17.0f, juce::Font::bold));
        g.drawText(lcd1, lcd.reduced(12, 8).removeFromTop(24), juce::Justification::centredLeft);
        g.setColour(amber.withAlpha(0.8f)); g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.5f, juce::Font::plain));
        g.drawText(lcd2, lcd.reduced(12, 8).withTrimmedTop(26).removeFromTop(18), juce::Justification::centredLeft);
        g.drawText(lcd3, lcd.reduced(12, 8).withTrimmedTop(46).removeFromTop(18), juce::Justification::centredLeft);
        // output meter in the LCD
        const int segs = 24; const float lvl = juce::Decibels::gainToDecibels(meter, -60.0f);
        for (int i = 0; i < segs; ++i)
        {
            const float db = -60.0f + (float) (i + 1) / segs * 60.0f; const bool lit = lvl >= db - 1.2f;
            g.setColour(lit ? (db > -3 ? red : amber) : amber.withAlpha(0.12f));
            g.fillRect(lcd.getX() + 12 + i * 10.5f, lcd.getBottom() - 15, 8.0f, 6.0f);
        }

        auto head = [&](const juce::String& t, int x, int y, int w, juce::Colour c) {
            g.setColour(c); g.fillRoundedRectangle((float) x, (float) y + 2, 4.0f, 12.0f, 1.5f);
            g.setColour(ink.withAlpha(0.9f)); g.setFont(juce::FontOptions(11.0f, juce::Font::bold)); g.drawText(t, x + 10, y, w, 16, juce::Justification::centredLeft);
            g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawHorizontalLine(y + 19, (float) x, (float) (x + w)); };
        head("PARTS", 24, 122, 640, amber);
        head("TONE", 690, 122, 266, teal);
        head("EFFECT 1", 24, 372, 452, blue);
        head("EFFECT 2", 504, 372, 452, blue);

        g.setColour(dim); g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        const int hy = 144; const char* cols[] = { "PROGRAM", "LEVEL", "PAN", "LOW", "HIGH", "TRANS", "CHAN" };
        const int cx[] = { 58, 236, 332, 402, 456, 510, 568 }, cw[] = { 170, 92, 66, 50, 50, 54, 50 };
        for (int i = 0; i < 7; ++i) g.drawText(cols[i], cx[i], hy, cw[i], 12, juce::Justification::centred);

        // selected-part highlight
        const bool multi = proc.apvts.getRawParameterValue("mode")->load() > 1.5f;
        if (multi) { g.setColour(amber.withAlpha(0.10f)); g.fillRoundedRectangle(20.0f, (float) (158 + (selPart - 1) * 26), 648.0f, 26.0f, 4.0f); }
    }

    void resized() override
    {
        for (int i = 0; i < 3; ++i) modeBtn[i]->setBounds(660 + i * 70, 22, 66, 44);
        step[0].setBounds(660, 76, 26, 24); data.setBounds(690, 74, 160, 28); step[1].setBounds(854, 76, 26, 24);
        knobs[0]->setBounds(886, 16, 80, 92);

        for (int i = 0; i < 8; ++i)
        {
            auto& r = row[(size_t) i]; const int y = 160 + i * 26;
            r.num.setBounds(28, y, 24, 22); r.prog.setBounds(58, y, 170, 22);
            r.lvl.setBounds(236, y, 92, 22); r.pan.setBounds(332, y, 66, 22); r.lo.setBounds(402, y, 50, 22);
            r.hi.setBounds(456, y, 50, 22); r.tr.setBounds(510, y, 54, 22); r.ch.setBounds(568, y, 50, 22);
        }
        // tone knobs 3 x 2
        for (int i = 0; i < 6; ++i) knobs[(size_t) (1 + i)]->setBounds(690 + (i % 3) * 90, 148 + (i / 3) * 110, 88, 106);
        // effects
        for (int i = 0; i < 2; ++i)
        {
            const int x0 = i == 0 ? 24 : 504;
            fxType[i].setBounds(x0, 398, 110, 24);
            for (int k = 0; k < 3; ++k) fxKnob[i][k]->setBounds(x0 + 130 + k * 108, 394, 100, 112);
        }
        joystick.setBounds(24, 520, 128, 160);
        keyboard.setBounds(170, 524, 790, 156);
    }

private:
    struct Row
    {
        juce::TextButton num; juce::ComboBox prog; juce::Slider lvl, pan, lo, hi, tr, ch;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> progAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lvlAtt, panAtt, loAtt, hiAtt, trAtt, chAtt;
    };

    int mode() const { return juce::roundToInt(proc.apvts.getRawParameterValue("mode")->load()); }
    juce::String currentParamId() const { const int m = mode(); return m == 0 ? "p1prog" : m == 1 ? "combi" : "p" + juce::String(selPart) + "prog"; }
    void setChoice(const juce::String& id, int idx)
    {
        if (auto* p = apvts.getParameter(id))
        {
            p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1((float) idx)); p->endChangeGesture();
        }
    }

    void timerCallback() override
    {
        const int m = mode();
        for (int i = 0; i < 3; ++i) modeBtn[i]->setToggleState(i == m, juce::dontSendNotification);
        const auto id = currentParamId();
        const int cur = juce::roundToInt(apvts.getRawParameterValue(id)->load());
        updating = true; if (juce::roundToInt(data.getValue()) != cur) data.setValue(cur, juce::dontSendNotification); updating = false;

        const char* ml[3] = { "PROGRAM", "COMBINATION", "MULTI" };
        const auto& progs = pcm::programs();
        if (m == 0) lcd1 = "PROG  P" + juce::String(cur).paddedLeft('0', 2) + " " + juce::String(progs[(size_t) cur].name);
        else if (m == 1) lcd1 = "COMBI C" + juce::String(cur).paddedLeft('0', 2) + " " + juce::String(pcm::combinations()[(size_t) cur].name);
        else lcd1 = "MULTI part " + juce::String(selPart) + " P" + juce::String(cur).paddedLeft('0', 2) + " " + juce::String(progs[(size_t) cur].name);
        lcd1 = lcd1.substring(0, 28);
        int parts = 0; for (int i = 1; i <= 8; ++i) if (apvts.getRawParameterValue("p" + juce::String(i) + "lvl")->load() > 0.001f) ++parts;
        lcd2 = juce::String(ml[m]) + "  PARTS " + juce::String(m == 0 ? 1 : parts) + "  VOICES " + juce::String(proc.voicesActive.load()).paddedLeft('0', 2) + "/16";
        lcd3 = "FX1 " + juce::String(pcm::fxName(juce::roundToInt(apvts.getRawParameterValue("fx1type")->load()))) + "  FX2 "
             + juce::String(pcm::fxName(juce::roundToInt(apvts.getRawParameterValue("fx2type")->load())));
        const float pk = proc.outPeak.load(); meter = pk > meter ? pk : meter * 0.9f;

        static const char* lab[pcm::FxCount][2] = { { "A", "B" }, { "Size", "Damp" }, { "Time", "Feedback" }, { "Rate", "Depth" }, { "Rate", "Feedback" }, { "Drive", "Tone" }, { "Speed", "Depth" } };
        for (int i = 0; i < 2; ++i)
        {
            const int t = juce::jlimit(0, pcm::FxCount - 1, juce::roundToInt(apvts.getRawParameterValue("fx" + juce::String(i + 1) + "type")->load()));
            fxKnob[i][0]->setTitle(lab[t][0]); fxKnob[i][1]->setTitle(lab[t][1]);
        }
        for (int i = 0; i < 8; ++i)
        {
            const float a = (m == 0 && i > 0) ? 0.35f : 1.0f;
            row[(size_t) i].num.setAlpha(a); row[(size_t) i].prog.setAlpha(a);
            for (auto* s : { &row[(size_t) i].lvl, &row[(size_t) i].pan, &row[(size_t) i].lo, &row[(size_t) i].hi, &row[(size_t) i].tr }) s->setAlpha(a);
            row[(size_t) i].ch.setAlpha(m == 2 ? 1.0f : 0.35f);
        }
        repaint();
    }

    PcmSynthProcessor& proc;
    juce::AudioProcessorValueTreeState& apvts;
    SynthLook laf;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::unique_ptr<ModeButton> modeBtn[3];
    juce::Slider data; juce::TextButton step[2];
    Row row[8];
    juce::ComboBox fxType[2];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fxAtt[2];
    Knob* fxKnob[2][3] = {};
    JoystickPad joystick;
    juce::MidiKeyboardComponent keyboard;
    juce::String lcd1, lcd2, lcd3;
    float meter = 0.0f; int selPart = 1; bool updating = false;
};

//==============================================================================
PcmSynthEditor::PcmSynthEditor(PcmSynthProcessor& p) : juce::AudioProcessorEditor(&p)
{
    content = std::make_unique<Content>(p);
    addAndMakeVisible(*content);
    constrainer.setFixedAspectRatio((double) kBaseW / kBaseH);
    constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
    setConstrainer(&constrainer);
    setResizable(true, true);
    setSize(kBaseW * 9 / 10, kBaseH * 9 / 10);
}
PcmSynthEditor::~PcmSynthEditor() { setConstrainer(nullptr); }

void PcmSynthEditor::resized()
{
    content->setBounds(0, 0, kBaseW, kBaseH);
    content->setTransform(juce::AffineTransform::scale((float) getWidth() / (float) kBaseW));
}
