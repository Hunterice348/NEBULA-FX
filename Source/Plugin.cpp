#include <JuceHeader.h>

namespace Nebula {
    struct StateManager {
        static void save(juce::AudioProcessorValueTreeState& apvts, juce::MemoryBlock& dest, int w, int h) {
            auto root = apvts.copyState();
            auto ui = juce::ValueTree("UI"); 
            ui.setProperty("w", w, nullptr); 
            ui.setProperty("h", h, nullptr);
            if (auto ex = root.getChildWithName("UI"); ex.isValid()) root.removeChild(ex, nullptr);
            root.appendChild(ui, nullptr);
            if (auto xml = root.createXml()) dest.replaceAll(xml->createDocumentToString("", true).toRawUTF8());
        }
        static void restore(juce::AudioProcessorValueTreeState& apvts, const void* data, int size, int& w, int& h) {
            if (size > 0) {
                if (auto xml = juce::XmlDocument::parse(juce::String(static_cast<const char*>(data), size))) {
                    if (xml->hasTagName(apvts.state.getType())) {
                        auto state = juce::ValueTree::fromXml(*xml);
                        if (auto ui = state.getChildWithName("UI"); ui.isValid()) { 
                            w = (int)ui.getProperty("w", 800); 
                            h = (int)ui.getProperty("h", 600); 
                            state.removeChild(ui, nullptr); 
                        }
                        apvts.replaceState(state);
                    }
                }
            }
        }
    };
}

class NebulaCompressor {
public:
    void prepare(double sr) { sampleRate = sr; z1 = 0.0f; env = 0.0f; }
    void process(juce::AudioBuffer<float>& buf, float thDb, float ratio, float attMs, float relMs) {
        float th = juce::Decibels::decibelsToGain(thDb);
        float aA = juce::jmin(1.0f, (float)(1.0 - std::exp(-1.0 / (sampleRate * attMs / 1000.0))));
        float aR = juce::jmin(1.0f, (float)(1.0 - std::exp(-1.0 / (sampleRate * relMs / 1000.0))));
        float slope = 1.0f - (1.0f / ratio);
        for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
            float* d = buf.getWritePointer(ch);
            for (int i = 0; i < buf.getNumSamples(); ++i) {
                float in = d[i]; 
                if (!std::isfinite(in)) in = 0.0f; // Защита от NaN
                float rect = std::abs(in);
                env = (rect > th) ? (aA * (rect - env) + env) : (aR * (rect - env) + env);
                float os = juce::jmax(0.0f, env - th);
                float gr = juce::Decibels::gainToDecibels(1.0f - (os * slope));
                z1 = (0.99f * z1) + (0.01f * gr); // Сглаживание
                d[i] = in * juce::Decibels::decibelsToGain(z1);
            }
        }
    }
private:
    double sampleRate = 44100.0; 
    float z1 = 0.0f, env = 0.0f;
};

class NebulaFXAudioProcessor : public juce::AudioProcessor {
public:
    NebulaFXAudioProcessor() : AudioProcessor(juce::AudioProcessor::BusesProperties()
        .withInput("In", juce::AudioChannelSet::stereo(), true)
        .withOutput("Out", juce::AudioChannelSet::stereo(), true)),
        apvts(*this, nullptr, "Params", {
            std::make_unique<juce::AudioParameterFloat>("THRESH", "Threshold", -60.0f, 0.0f, -12.0f),
            std::make_unique<juce::AudioParameterFloat>("RATIO", "Ratio", 1.0f, 20.0f, 4.0f),
            std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack", 0.1f, 100.0f, 10.0f),
            std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release", 10.0f, 1000.0f, 100.0f),
            std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", 0.0f, 24.0f, 0.0f)
        }) {}
    ~NebulaFXAudioProcessor() override {}

    void prepareToPlay(double sr, int) override { comp.prepare(sr); }
    void releaseResources() override {}
    
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals noDenormals;
        comp.process(buf, *apvts.getRawParameterValue("THRESH"), *apvts.getRawParameterValue("RATIO"), 
                     *apvts.getRawParameterValue("ATTACK"), *apvts.getRawParameterValue("RELEASE"));
        buf.applyGain(juce::Decibels::decibelsToGain(*apvts.getRawParameterValue("GAIN")));
    }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return new Editor(*this); }
    
    const juce::String getName() const override { return "NEBULA FX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock& dest) override { Nebula::StateManager::save(apvts, dest, uiW, uiH); }
    void setStateInformation(const void* data, int size) override { Nebula::StateManager::restore(apvts, data, size, uiW, uiH); }

    juce::AudioProcessorValueTreeState apvts;
    int uiW = 800, uiH = 600;
    NebulaCompressor comp;

private:
    class Editor : public juce::AudioProcessorEditor {
    public:
        Editor(NebulaFXAudioProcessor& p) : AudioProcessorEditor(&p), proc(p) {
            setResizable(true, true); 
            setResizeLimits(600, 450, 1600, 1200); 
            setSize(proc.uiW, proc.uiH);
            
            auto setup = [](juce::Slider& s) { 
                s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); 
                s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20); 
                s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFFF0055)); // Neon Red
                s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF333333)); 
                s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white); 
            };
            
            setup(thresh); setup(ratio); setup(att); setup(rel); setup(gain);
            addAndMakeVisible(thresh); addAndMakeVisible(ratio); addAndMakeVisible(att); addAndMakeVisible(rel); addAndMakeVisible(gain);
            
            att1.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.apvts, "THRESH", thresh));
            att2.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.apvts, "RATIO", ratio));
            att3.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.apvts, "ATTACK", att));
            att4.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.apvts, "RELEASE", rel));
            att5.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.apvts, "GAIN", gain));
        }
        ~Editor() override {}
        
        void paint(juce::Graphics& g) override {
            // 1. Темный металлический фон
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF1A1A1A), 0, 0, juce::Colour(0xFF0D0D0D), 0, (float)getHeight(), false)); 
            g.fillRect(getLocalBounds());
            
            auto area = getLocalBounds().reduced(20);
            
            // 2. 3D-эффект панели через тени
            g.setDropShadow(juce::DropShadow(juce::Colours::black.withAlpha(0.7f), 12, juce::Point<int>(0, 6))); 
            g.fillRoundedRectangle(area.toFloat(), 16.0f);
            
            // 3. Внутренний градиент металла
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF222222), 0, (float)area.getY(), juce::Colour(0xFF111111), 0, (float)area.getBottom(), false)); 
            g.fillRoundedRectangle(area.reduced(2).toFloat(), 14.0f);
            
            // 4. Неоновая акцентная линия
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xFFFF0055), (float)area.getX(), 0, juce::Colour(0xFF7000FF), (float)area.getRight(), 0, false)); 
            g.fillRect(area.getX(), area.getY(), area.getWidth(), 3);
            
            g.setColour(juce::Colours::white.withAlpha(0.8f)); 
            g.setFont(juce::Font(24.0f, juce::Font::bold)); 
            g.drawText("NEBULA FX", area.removeFromTop(60), juce::Justification::centred);
        }
        
        void resized() override {
            auto area = getLocalBounds().reduced(30, 80); 
            int sz = juce::jmin(120, area.getWidth() / 5 - 20), sp = (area.getWidth() - (sz * 5)) / 6;
            auto place = [&](juce::Slider& s) { s.setBounds(area.removeFromLeft(sz)); area.removeFromLeft(sp); };
            
            place(thresh); place(ratio); place(att); place(rel); place(gain); 
            
            // Сохраняем текущий размер для FL Studio
            proc.uiW = getWidth(); 
            proc.uiH = getHeight();
        }
        
    private:
        NebulaFXAudioProcessor& proc; 
        juce::Slider thresh, ratio, att, rel, gain;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att1, att2, att3, att4, att5;
    };
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NebulaFXAudioProcessor(); }
