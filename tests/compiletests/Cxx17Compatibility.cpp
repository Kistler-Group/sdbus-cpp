/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 * (C) 2026 - Kai-Uwe Hermann <kai-uwe.hermann@pionix.de>
 *
 * @file Cxx17Compatibility.cpp
 *
 * Created on: Oct 7, 2026
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

#if __cplusplus != 201703L
#error "This test must be compiled as C++17"
#endif

#include <sdbus-c++/sdbus-c++.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

    template <typename T>
    constexpr std::string_view signatureOf()
    {
        return {sdbus::signature_of_v<T>.data(), sdbus::signature_of_v<T>.size()};
    }

    template <std::size_t N>
    constexpr std::string_view view(const std::array<char, N>& chars)
    {
        return {chars.data(), chars.size()};
    }

    static_assert(signatureOf<int32_t>() == "i");
    static_assert(signatureOf<std::vector<std::string>>() == "as");
    static_assert(signatureOf<std::map<std::string, sdbus::Variant>>() == "a{sv}");
    static_assert(signatureOf<sdbus::Struct<int32_t, std::string, std::vector<uint8_t>>>() == "(isay)");
    static_assert(signatureOf<std::tuple<bool, double>>() == "bd");

    constexpr auto concatenated = sdbus::operator+(std::array{'a', 'b'}, std::array{'c'});
    constexpr auto emptyLhs = sdbus::operator+(std::array<char, 0>{}, std::array{'x'});
    constexpr auto emptyRhs = sdbus::operator+(std::array{'x'}, std::array<char, 0>{});
    static_assert(view(concatenated) == "abc");
    static_assert(view(emptyLhs) == "x");
    static_assert(view(emptyRhs) == "x");

    [[maybe_unused]] void instantiateClientApi(sdbus::IProxy& proxy)
    {
        constexpr auto interfaceName = "org.sdbuscpp.Cxx17";

        int32_t product{};
        proxy.callMethod("Multiply").onInterface(interfaceName).withArguments(int32_t{2}, int32_t{3}).storeResultsTo(product);

        proxy.callMethodAsync("Multiply")
             .onInterface(interfaceName)
             .withArguments(int32_t{2}, int32_t{3})
             .uponReplyInvoke([](const std::optional<sdbus::Error>&, int32_t){});

        static_cast<void>(proxy.callMethodAsync("Concatenate")
                               .onInterface(interfaceName)
                               .withArguments(std::map<std::string, sdbus::Variant>{})
                               .getResultAsFuture<std::string>());

        proxy.getPropertyAsync("State")
             .onInterface(interfaceName)
             .uponReplyInvoke([](const std::optional<sdbus::Error>&, const sdbus::Variant&){});

        proxy.uponSignal("StateChanged")
             .onInterface(interfaceName)
             .call([](const std::string&, const std::vector<sdbus::Struct<int32_t, std::string>>&){});

        auto methodCall = proxy.createMethodCall(sdbus::InterfaceName{interfaceName}, sdbus::MethodName{"Multiply"});
        methodCall << int32_t{2} << int32_t{3};
        proxy.callMethodAsync(methodCall, [](sdbus::MethodReply reply, const std::optional<sdbus::Error>&)
        {
            int32_t result{};
            reply >> result;
        });
    }

} // namespace
