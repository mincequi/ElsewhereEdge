/*
 * Copyright (C) 2021 Manuel Weichselbaumer <mincequi@web.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <optional>
#include <string>

struct HostConfig {
    std::string host;
    uint16_t port;
};

class Config {
public:
    Config(const std::string& configFile = "/etc/elsewhere.conf");

    void parse();

    std::optional<HostConfig> hostConfig(const std::string& ) const;

private:
    std::string m_configFile;
};
