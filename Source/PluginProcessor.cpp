#include "PluginProcessor.h"

AudioPluginProcessor::AudioPluginProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters",
                {
                    std::make_unique<juce::AudioParameterFloat>("cutoff",
                                                              "Cutoff",
                                                              juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
                                                              1000.0f),
                    std::make_unique<juce::AudioParameterFloat>("resonance",
                                                              "Resonance",
                                                              juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f),
                                                              0.707f)
                })
{
    parameters.addParameterListener("cutoff", this);
    parameters.addParameterListener("resonance", this);
    
    filterChain = std::make_unique<juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>>>();
}

AudioPluginProcessor::~AudioPluginProcessor()
{
    parameters.removeParameterListener("cutoff", this);
    parameters.removeParameterListener("resonance", this);
}

void AudioPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    filterChain->prepare(spec);
    
    auto& filter = filterChain->get<0>();
    filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1000.0f, 0.707f);
}

void AudioPluginProcessor::releaseResources()
{
}

void AudioPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filterChain->process(context);
}

void AudioPluginProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "cutoff" || parameterID == "resonance")
    {
        auto cutoff = parameters.getRawParameterValue("cutoff")->load();
        auto resonance = parameters.getRawParameterValue("resonance")->load();
        
        auto& filter = filterChain->get<0>();
        *filter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), cutoff, resonance);
    }
}

juce::AudioProcessorEditor* AudioPluginProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void AudioPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginProcessor();
} 