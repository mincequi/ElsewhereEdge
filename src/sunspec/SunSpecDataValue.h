#ifndef SUNSPECDATAVALUE_H
#define SUNSPECDATAVALUE_H

#include <set>
#include <string>
#include <variant>
#include <vector>

#include "SunSpecMeasuredValue.h"
#include "SunSpecTypes.h"

namespace sunspec {

using LiveValue = std::variant<
    uint32_t,   // timestamp
    InverterOperatingState,
    InverterEvents,
    int32_t,    //
    double,     // regular value
    std::vector<Block<double>>,    // block arrays
    std::string>;

class StatsValue {
public:
    StatsValue& operator=(const LiveValue& v);
    StatsValue& operator|=(const InverterOperatingState& v);
    StatsValue& operator|=(const InverterEvents& v);

    bool isDirty() const;

private:
    std::variant<
            uint32_t,
            std::set<InverterOperatingState>,
            InverterEvents,
            MeasuredValue<int32_t>,
            MeasuredValue<double>,
            std::vector<Block<MeasuredValue<double>>>> m_variant;

    bool m_isDirty = false;

    friend std::ostream& operator<<(std::ostream& os, const sunspec::StatsValue& value);
    friend class StatsModel;
};

} // namespace sunspec

#endif // SUNSPECDATAVALUE_H
