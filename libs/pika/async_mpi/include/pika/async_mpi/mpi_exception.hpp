//  Copyright (c) 2020 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/local/config.hpp>
#include <pika/errors/exception.hpp>
#include <pika/mpi_base/mpi.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace pika::mpi::experimental {

    namespace detail {

        // extract MPI error message
        std::string error_message(int code)
        {
            int N = 1023;
            std::unique_ptr<char[]> err_buff(new char[std::size_t(N) + 1]);
            err_buff[0] = '\0';

            MPI_Error_string(code, err_buff.get(), &N);

            return err_buff.get();
        }

    }    // namespace detail

    // -------------------------------------------------------------------------
    // exception type for failed launch of MPI functions
    struct mpi_exception : pika::exception
    {
        explicit mpi_exception(int err_code, const std::string& msg = "")
          : err_code_(err_code)
        {
            pika::exception(pika::bad_function_call,
                msg + std::string(" MPI returned with error: ") +
                    detail::error_message(err_code));
        }

        int get_mpi_errorcode()
        {
            return err_code_;
        }

    protected:
        int err_code_;
    };

}    // namespace pika::mpi::experimental
