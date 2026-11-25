#include <JuceHeader.h>

class NebulaFXAudioProcessor : public juce::AudioProcessor {
public:
    NebulaFXAudioProcessor() : AudioProcessor(juce::AudioProcessor::BusesProperties()
        .withInput("In", juce::AudioChannelSet::stereo(), true)
        .withOutput("Out", juce::AudioChannelSet::stereo(), true)),
        apvts(*this, nullptr, "Params", {
            std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", -48.0f, 24.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("MIX", "Mix", 0.0f, 100.0f, 100.0f)
        }) {}
    ~NebulaFXAudioProcessor() override {}

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    
    void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals noDenormals;
        float gain = juce::Decibels::decibelsToGain(*apvts.getRawParameterValue("GAIN"));
        buf.applyGain(gain);
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
    
    void getStateInformation(juce::MemoryBlock& dest) override {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        if (xml) dest.replaceAll(xml->createDocumentToString("", true).toRawUTF8());
    }
    
    void setStateInformation(const void* data, int size) override {
        std::unique_ptr<juce::XmlElement> xmlState(juce::XmlDocument::parse(juce::String(static_cast<const char*>(data), size)));
        if (xmlState && xmlState->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }

    juce::AudioProcessorValueTreeState apvts;
    int uiW = 600, uiH = 400;

private:
    class Editor : public juce::AudioProcessorEditor {
    public:
        Editor(NebulaFXAudioProcessor& p) : AudioProcessorEditor(&p), proc(p) {
            setResizable(true, true);
            setResizeLimits(400, 300, 1200, 800);
            setSize(proc.uiW, proc.uiH);
            
            gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
            gainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFFF0055));
            gainSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF333333));
            gainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
            
            addAndMakeVisible(gainSlider);
            attachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.apvts, "GAIN", gainSlider));
        }
        ~Editor() override {}
        
        void paint(juce::Graphics& g) override {
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF1A1A1A), 0, 0, juce::Colour(0xFF0D0D0D), 0, (float)getHeight(), false));
            g.fillRect(getLocalBounds());
            
            auto area = getLocalBounds().reduced(20);
            g.setDropShadow(juce::DropShadow(juce::Colours::black.withAlpha(0.7f), 12, juce::Point<int>(0, 6)));
            g.fillRoundedRectangle(area.toFloat(), 16.0f);
            
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF222222), 0, (float)area.getY(), juce::Colour(0xFF111111), 0, (float)area.getBottom(), false));
            g.fillRoundedRectangle(area.reduced(2).toFloat(), 14.0f);
            
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xFFFF0055), (float)area.getX(), 0, juce::Colour(0xFF7000FF), (float)area.getRight(), 0, false));
            g.fillRect(area.getX(), area.getY(), area.getWidth(), 3);
            
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(juce::Font(24.0f, juce::Font::bold));
            g.drawText("NEBULA FX", area.removeFromTop(60), juce::Justification::centred);
        }
        
        void resized() override {
            gainSlider.setBounds(getWidth() / 2 - 40, getHeight() / 2 - 20, 80, 100);
            proc.uiW = getWidth();
            proc.uiH = getHeight();
        }
        
    private:
        NebulaFXAudioProcessor& proc;
        juce::Slider gainSlider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NebulaFXAudioProcessor(); }
