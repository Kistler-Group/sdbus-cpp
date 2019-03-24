/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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
    namespace internal {
        class Object;
    }
    class Error;
}

namespace sdbus {

    /********************************************//**
     * @class MethodResult
     *
     * Represents result of an asynchronous server-side method.
     * An instance is provided to the method and shall be set
     * by the method to either method return value or an error.
     *
     ***********************************************/
    class MethodResult
    {
    protected:
        friend sdbus::internal::Object;

        MethodResult() = default;
        MethodResult(MethodCall msg);

        MethodResult(const MethodResult&) = delete;
        MethodResult& operator=(const MethodResult&) = delete;

        MethodResult(MethodResult&& other) = default;
        MethodResult& operator=(MethodResult&& other) = default;

        template <typename... _Results> void returnResults(const _Results&... results) const;
        void returnError(const Error& error) const;

    private:
        MethodCall call_;
    };

    /********************************************//**
     * @class Result
     *
     * Represents result of an asynchronous server-side method.
     * An instance is provided to the method and shall be set
     * by the method to either method return value or an error.
     *
     ***********************************************/
    template <typename... _Results>
    class Result : protected MethodResult
    {
    public:
        Result() = default;
        Result(MethodResult&& result);

        void returnResults(const _Results&... results) const;
        void returnError(const Error& error) const;
    };

}

#include <sdbus-c++/MethodResult.inl>

#endif /* SDBUS_CXX_METHODRESULT_H_ */
