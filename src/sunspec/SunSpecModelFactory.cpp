#include "SunSpecModelFactory.h"

#include <functional>
#include <map>

#include <sunspec/models/SunSpecInverterModelFactory.h>
#include <sunspec/models/SunSpecMpptInverterExtensionModelFactory.h>
#include <sunspec/models/SunSpecWyeConnectMeterModelFactory.h>

namespace sunspec {

static std::map<uint16_t, std::function<void(std::optional<Model>& model,
                                             const std::vector<uint16_t>& buffer,
                                             uint32_t timestamp)>> s_factories = {
    { 103, &InverterModelFactory::updateFromBuffer },
    { 160, &MpptInverterExtensionModelFactory::updateFromBuffer },
    { 203, &WyeConnectMeterModelFactory::updateFromBuffer },
};

void ModelFactory::updateFromBuffer(std::optional<Model>& model,
                                    const std::vector<uint16_t>& buffer,
                                    uint32_t timestamp) {
    if (buffer.empty() || s_factories.count(buffer.at(0)) == 0) {
        return;
    }

    s_factories.at(buffer.at(0))(model, buffer, timestamp);
}

} // namespace sunspec
