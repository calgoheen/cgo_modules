#include <cgo_plugin/cgo_plugin.h>

namespace cgo
{

std::unique_ptr<juce::AudioProcessorParameterGroup> ParamHolder::createParameterGroup (const juce::String& groupId, const juce::String& groupName)
{
    if (hasBeenCalled.exchange (true))
    {
        jassertfalse;
        return nullptr;
    }

    const juce::String fullIdPrefix = groupId + (groupId.isEmpty() ? "" : "_") + idPrefix + (idPrefix.isEmpty() ? "" : "_");
    const juce::String fullNamePrefix = groupName + (groupName.isEmpty() ? "" : " ") + namePrefix + (namePrefix.isEmpty() ? "" : " ");

    auto paramGroup = std::make_unique<juce::AudioProcessorParameterGroup> (fullIdPrefix, "", "");

    for (auto ptr : paramPtrs)
    {
        auto& pp = *ptr;
        auto& paramId = const_cast<juce::String&> (pp->paramID);
        auto& paramName = const_cast<juce::String&> (pp->name);

        paramId = fullIdPrefix + paramId;
        paramName = fullNamePrefix + paramName;

        paramGroup->addChild (pp.release());
    }

    for (auto ptr : paramHolderPtrs)
        paramGroup->addChild (ptr->createParameterGroup (groupId, groupName));

    return std::move (paramGroup);
}

void ParamHolder::pushPrefixes (const juce::String& newIdPrefix, const juce::String& newNamePrefix)
{
    static const auto pushPrefix = [] (juce::String& str, const juce::String& prefix)
    {
        if (str.isEmpty())
            str = prefix;
        else if (prefix.isNotEmpty())
            str = prefix + "_" + str;
    };

    pushPrefix (idPrefix, newIdPrefix);
    pushPrefix (namePrefix, newNamePrefix);

    for (auto paramHolder : paramHolderPtrs)
        paramHolder->pushPrefixes (newIdPrefix, newNamePrefix);
}

} // namespace cgo
