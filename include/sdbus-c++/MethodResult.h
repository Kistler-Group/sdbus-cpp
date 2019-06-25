/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file MethodResult.h
 *
 * Created on: Nov 8, 2016
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

#ifndef SDBUS_CXX_METHODRESULT_H_
#define SDBUS_CXX_METHODRESULT_H_

#include <sdbus-c++/Message.h>
#include <cassert>

// Forward declaration
namespace sdbus {
    class Error;
}

namespace sdbus {

    /********************************************//**
     * @class Result
     *
     * Represents result of an asynchronous server-side method.
     * An instance is provided to the method and shall be set
     * by the method to either method return value or an error.
     *
     ***********************************************/
    template <typename... _Results>
    class Result
    {
    public:
        Result() = default;
        Result(MethodCall call);

        Result(const Result&) = delete;
        Result& operator=(const Result&) = delete;

        Result(Result&& other) = default;
        Result& operator=(Result&& other) = default;

        void returnResults(const _Results&... results) const;
        void returnError(const Error& error) const;

    private:
        MethodCall call_;
    };

    template <typename... _Results>
    inline Result<_Results...>::Result(MethodCall call)
        : call_(std::move(call))
    {
    }

    template <typename... _Results>
    inline void Result<_Results...>::returnResults(const _Results&... results) const
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

    template <typename... _Results>
    inline void Result<_Results...>::returnError(const Error& error) const
    {
        auto reply = call_.createErrorReply(error);
        reply.send();
    }

}

#endif /* SDBUS_CXX_METHODRESULT_H_ */
