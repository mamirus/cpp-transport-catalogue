#include "domain.h"
#include <iomanip>


namespace transport_catalogue {

    std::ostream& operator<<(std::ostream& os, const BusInfo& bi) {
        using namespace std::literals;
        double length = bi.route_length;

        os  << "Bus "s << bi.bus_name << ": "s << bi.stops_number << " stops on route, "s
            << bi.unique_stops << " unique stops, "s << std::setprecision(6) << length << " route length, "s
            << std::setprecision(6) << bi.curvature << " curvature"s << std::endl;

        return os;
    }


}