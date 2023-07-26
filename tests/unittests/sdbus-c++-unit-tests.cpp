/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file sdbus-c++-unit-tests.cpp
 *
 * Created on: Nov 27, 2016
 * Project: sdbus-c++
 * Description: High-level D-Bus IPC C++ library based on sd-bus
 *
 * This file is part of sdbus-c++.
 *
 * sdbus-c++ is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * sdbus-c++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with sdbus-c++. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gmock/gmock.h"

#include "sdbus-c++/sdbus-c++.h"

// There are some data races reported thread sanitizer in unit/intg tests. TODO: investigate, it looks like this example is not well written:
class Global
{
public:
    ~Global()
    {
        std::thread t([]() {
            sdbus::Variant v((int) 5);
            printf("%d\n", v.get<int>());
        });
        t.detach();
    }
};

Global g1;
Global g2;
Global g3;

int main(int argc, char **argv)
{
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
