/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file ScopeGuard.h
 *
 * Created on: Apr 29, 2015
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

#ifndef SDBUS_CPP_INTERNAL_SCOPEGUARD_H_
#define SDBUS_CPP_INTERNAL_SCOPEGUARD_H_

#include <exception>
#include <utility>

// Straightforward, modern, easy-to-use RAII utility to perform work on scope exit in an exception-safe manner.
//
// The utility helps providing basic exception safety guarantee by ensuring that the resources are always
// released in face of an exception and released or kept when exiting the scope normally.
//
// Use SCOPE_EXIT if you'd like to perform an (mostly clean-up) operation when the scope ends, either due
// to an exception or because it just ends normally.
// Use SCOPE_EXIT_NAMED if you'd like to conditionally deactivate given scope-exit operation. This is useful
// if, for example, we want the operation to be executed only in face of an exception.
//
// Example usage (maybe a bit contrived):
// SqlDb* g_pDb = nullptr;
// void fnc() {
//     auto* pDb = open_database(...); // Resource to be released when scope exits due to whatever reason
//     SCOPE_EXIT{ close_database(pDb); }; // Executes body when exiting the scope due to whatever reason
//     g_pDb = open_database(...); // Resource to be released only in face of an exception
//     SCOPE_EXIT_NAMED(releaseGlobalDbOnException) // Executes body when exiting the scope in face of an exception
//     {
//         close_database(g_pDb);
//         g_pDb = nullptr;
//     };
//     //... do operations (that may or may not possibly throw) on the database here
//     releaseGlobalDbOnException.dismiss(); // Don't release global DB on normal scope exit
//     return; // exiting scope normally
// }

#define SCOPE_EXIT                                                \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE)                     \
        = ::sdbus::internal::ScopeGuardOnExitTag{} + [&]()        \
    /**/
#define SCOPE_EXIT_NAMED(NAME)                                    \
    auto NAME                                                     \
        = ::sdbus::internal::ScopeGuardOnExitTag{} + [&]()        \
    /**/
#define SCOPE_EXIT_SUCCESS                                        \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE)                     \
        = ::sdbus::internal::ScopeGuardOnExitSuccessTag{} + [&]() \
    /**/
#define SCOPE_EXIT_SUCCESS_NAMED(NAME)                            \
    auto NAME                                                     \
        = ::sdbus::internal::ScopeGuardOnExitSuccessTag{} + [&]() \
    /**/
#define SCOPE_EXIT_FAILURE                                        \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE)                     \
        = ::sdbus::internal::ScopeGuardOnExitFailureTag{} + [&]() \
    /**/
#define SCOPE_EXIT_FAILURE_NAMED(NAME)                            \
    auto NAME                                                     \
        = ::sdbus::internal::ScopeGuardOnExitFailureTag{} + [&]() \
    /**/

namespace sdbus::internal {

    struct ScopeGuardOnExitTag
    {
        static bool holds(int /*originalExceptions*/)
        {
            return true; // Always holds
        }
    };
    struct ScopeGuardOnExitSuccessTag
    {
        static bool holds(int originalExceptions)
        {
            return originalExceptions == std::uncaught_exceptions(); // Only holds when no exception occurred within the scope
        }
    };
    struct ScopeGuardOnExitFailureTag
    {
        static bool holds(int originalExceptions)
        {
            return originalExceptions != std::uncaught_exceptions(); // Only holds when an exception occurred within the scope
        }
    };

    template <class _Fun, typename _Tag>
    class ScopeGuard
    {
    public:
        ScopeGuard(_Fun f) : fnc_(std::move(f))
        {
        }

        ScopeGuard() = delete;
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard(ScopeGuard&& rhs) : fnc_(std::move(rhs.fnc_)), active_(rhs.active_), exceptions_(rhs.exceptions_)
        {
            rhs.dismiss();
        }

        void dismiss()
        {
            active_ = false;
        }

        ~ScopeGuard()
        {
            if (active_ && _Tag::holds(exceptions_))
                fnc_();
        }

    private:
        _Fun fnc_;
        int exceptions_{std::uncaught_exceptions()};
        bool active_{true};
    };

    template <typename _Fun>
    ScopeGuard<_Fun, ScopeGuardOnExitTag> operator+(ScopeGuardOnExitTag, _Fun&& fnc)
    {
        return ScopeGuard<_Fun, ScopeGuardOnExitTag>(std::forward<_Fun>(fnc));
    }

    template <typename _Fun>
    ScopeGuard<_Fun, ScopeGuardOnExitSuccessTag> operator+(ScopeGuardOnExitSuccessTag, _Fun&& fnc)
    {
        return ScopeGuard<_Fun, ScopeGuardOnExitSuccessTag>(std::forward<_Fun>(fnc));
    }

    template <typename _Fun>
    ScopeGuard<_Fun, ScopeGuardOnExitFailureTag> operator+(ScopeGuardOnExitFailureTag, _Fun&& fnc)
    {
        return ScopeGuard<_Fun, ScopeGuardOnExitFailureTag>(std::forward<_Fun>(fnc));
    }

}

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)
#endif

#endif /* SDBUS_CPP_INTERNAL_SCOPEGUARD_H_ */
