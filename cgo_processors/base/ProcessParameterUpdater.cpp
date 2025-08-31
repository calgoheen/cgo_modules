#include <cgo_plugin/cgo_plugin.h>

namespace cgo
{

void ProcessParameterUpdater::add (juce::RangedAudioParameter& param, std::function<void (float)> onChange)
{
    jassert (onChange != nullptr);
    updaters.add (new Updater (param, std::move (onChange)));
}

void ProcessParameterUpdater::update()
{
    for (auto u : updaters)
        if (u->dirty.exchange (false))
            u->callback (u->parameter.convertFrom0to1 (u->value));
}

void ProcessParameterUpdater::flush()
{
    for (auto u : updaters)
        u->callback (u->parameter.convertFrom0to1 (u->value));
}

} // namespace cgo
