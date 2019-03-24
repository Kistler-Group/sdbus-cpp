/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file MethodResult.inl
 *
 * Created on: Mar 21, 2019
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

#ifndef SDBUS_CXX_METHODRESULT_INL_
#define SDBUS_CXX_METHODRESULT_INL_

#include <sdbus-c++/MethodResult.h>
#include <cassert>

namespace sdbus {

    inline MethodResult::MethodResult(MethodCall msg)
        : call_(std::move(msg))
    {
    }

    template <typename... _Results>
    inline void MethodResult::returnResults(const _Results&... results) const
    {
        assert(call_.isValid());
        auto reply = call_.createReply();
#ifdef __cpp_fold_expressions
        (reply << ... << results);
#else
        using _ = std::initializer_list<int>;
        (void)_{(void(reply << results), 0)...};
#endif
        reply.send();
    }

    inline void MethodResult::returnError(const Error& error) const
    {
        auto reply = call_.createErrorReply(error);
        reply.send();
    }

    template <typename... _Results>
    inline Result<_Results...>::Result(MethodResult&& result)
        : MethodResult(std::move(result))
    {
    }

    template <typename... _Results>
    inline void Result<_Results...>::returnResults(const _Results&... results) const
    {
        MethodResult::returnResults(results...);
    }

    template <typename... _Results>
    inline void Result<_Results...>::returnError(const Error& error) const
    {
        MethodResult::returnError(error);
    }

}

#endif /* SDBUS_CXX_METHODRESULT_INL_ */
