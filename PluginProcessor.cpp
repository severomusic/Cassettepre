#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout CassettepreampAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("drive",  "Drive",  0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tone",   "Tone",   0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("wow",    "Wow & Flutter", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noise",  "Noise",  0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", 0.0f, 1.0f, 0.8f));

    return { params.begin(), params.end() };
}

CassettepreampAudioProcessor::CassettepreampAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#else
     :
#endif
       apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

CassettepreampAudioProcessor::~CassettepreampAudioProcessor() {}

const juce::String CassettepreampAudioProcessor::getName() const { return JucePlugin_Name; }
bool CassettepreampAudioProcessor::acceptsMidi() const { return false; }
bool CassettepreampAudioProcessor::producesMidi() const { return false; }
bool CassettepreampAudioProcessor::isMidiEffect() const { return false; }
double CassettepreampAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int CassettepreampAudioProcessor::getNumPrograms() { return 1; }
int CassettepreampAudioProcessor::getCurrentProgram() { return 0; }
void CassettepreampAudioProcessor::setCurrentProgram (int) {}
const juce::String CassettepreampAudioProcessor::getProgramName (int) { return {}; }
void CassettepreampAudioProcessor::changeProgramName (int, const juce::String&) {}

void CassettepreampAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Set up delay buffer for wow & flutter (max 50ms)
    delayBufferSize = (int)(sampleRate * 0.05);
    delayBuffer.assign(delayBufferSize, 0.0f);
    delayWritePos = 0;
    wowPhase = 0.0f;
    flutterPhase = 0.0f;
}

void CassettepreampAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CassettepreampAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
}
#endif

void CassettepreampAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Get parameter values
    float drive  = apvts.getRawParameterValue("drive")->load();
    float tone   = apvts.getRawParameterValue("tone")->load();
    float wow    = apvts.getRawParameterValue("wow")->load();
    float noise  = apvts.getRawParameterValue("noise")->load();
    float output = apvts.getRawParameterValue("output")->load();

    // Drive: map 0-1 to a gain of 1-20
    float driveGain = 1.0f + drive * 19.0f;

    // Tone: simple low-pass coefficient (higher tone = more highs)
    float toneCoeff = 0.3f + tone * 0.65f;

    int numSamples  = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Wow: ~0.5Hz, Flutter: ~8Hz
    float wowRate     = 0.5f;
    float flutterRate = 8.0f;
    float wowDepth    = wow * 0.003f;
    float flutterDepth = wow * 0.001f;

    static float toneState = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // --- Wow & Flutter modulation ---
        wowPhase     += wowRate     / (float)currentSampleRate;
        flutterPhase += flutterRate / (float)currentSampleRate;
        if (wowPhase     > 1.0f) wowPhase     -= 1.0f;
        if (flutterPhase > 1.0f) flutterPhase -= 1.0f;

        float modulation = wowDepth     * std::sin(wowPhase     * juce::MathConstants<float>::twoPi)
                         + flutterDepth * std::sin(flutterPhase * juce::MathConstants<float>::twoPi);

        float delaySamples = modulation * (float)currentSampleRate;
        float readPos = (float)delayWritePos - delaySamples;
        while (readPos < 0) readPos += (float)delayBufferSize;

        // --- Noise ---
        noiseState = noiseState * 0.99f + ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.01f;
        float noiseSample = noiseState * noise * 0.05f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            float sample = channelData[i];

            // Write to delay buffer (use left channel for wow/flutter)
            if (ch == 0)
            {
                delayBuffer[delayWritePos] = sample;
            }

            // Read from delay buffer with modulation
            int readIdx = (int)readPos % delayBufferSize;
            if (readIdx < 0) readIdx += delayBufferSize;
            sample = delayBuffer[readIdx];

            // --- Drive / Saturation ---
            sample *= driveGain;

            // Asymmetric soft clipping (cassette-style)
            if (sample > 0.0f)
                sample = std::tanh(sample * 0.8f);
            else
                sample = std::tanh(sample * 1.1f);  // slightly harder on negative side

            // Compensate for drive gain
            sample /= std::sqrt(driveGain);

            // --- Tone (simple one-pole low-pass) ---
            toneState = toneState * (1.0f - toneCoeff) + sample * toneCoeff;
            sample = toneState;

            // --- Noise ---
            sample += noiseSample;

            // --- Output gain ---
            sample *= output;

            channelData[i] = sample;
        }

        if (delayWritePos < delayBufferSize - 1)
            delayWritePos++;
        else
            delayWritePos = 0;
    }
}

bool CassettepreampAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* CassettepreampAudioProcessor::createEditor()
{
    return new CassettepreampAudioProcessorEditor(*this);
}

void CassettepreampAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CassettepreampAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CassettepreampAudioProcessor();
}
