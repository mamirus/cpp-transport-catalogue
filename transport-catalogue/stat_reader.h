#pragma once

#include "transport_catalogue.h"

#include <iostream>
#include <ostream>
#include <string>

void OutputQuery(const transport_catalogue::TransportCatalogue& transport_catalogue, const std::string& query, std::ostream& out = std::cout);