/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file ConvenienceClasses.h
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
    class Object;
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
        friend Object;
        MethodResult() = default;
        MethodResult(const MethodCall& msg, Object& object);

        template <typename... _Results> void returnResults(const _Results&... results) const;
        void returnError(const Error& error) const;

    private:
        void send(const MethodReply& reply) const;

    private:
        MethodCall call_;
        Object* object_{};
    };

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
        send(reply);
    }

    inline void MethodResult::returnError(const Error& error) const
    {
        auto reply = call_.createErrorReply(error);
        send(reply);
    }

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
        Result(MethodResult result);
        void returnResults(const _Results&... results) const;
        void returnError(const Error& error) const;
    };

    template <typename... _Results>
    inline Result<_Results...>::Result(MethodResult result)
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

//    class AsyncResultBase
//    {
//    protected:
//        AsyncResultBase(const MethodCall& msg);
//        void send(const MethodReply& reply) const;
//
//        MethodCall call_;
//    };
//
//    /********************************************//**
//     * @class AsyncResult
//     *
//     * Represents result of an asynchronous server-side method.
//     * An instance is provided to the method and shall be set
//     * by the method to either method return value or an error.
//     *
//     ***********************************************/
//    template <typename... _Results>
//    class AsyncResult : protected AsyncResultBase
//    {
//    public:
//        using AsyncResultBase::AsyncResultBase;
//        void returnResults(const _Results&... results) const;
//        void returnError(const Error& error) const;
//    };
//
//    template <typename... _Results>
//    inline void AsyncResult<_Results...>::returnResults(const _Results&... results) const
//    {
//        auto reply = call_.createReply();
//        (reply << ... << results);
//        send(reply);
//    }
//
//    template <typename... _Results>
//    inline void AsyncResult<_Results...>::returnError(const Error& error) const
//    {
//        auto reply = call_.createErrorReply(error);
//        send(reply);
//    }
}

#endif /* SDBUS_CXX_METHODRESULT_H_ */
