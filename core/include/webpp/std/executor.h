// Created by moisrex on 4/4/20.
#ifndef WEBPP_STD_EXECUTOR_H
#define WEBPP_STD_EXECUTOR_H
// we rather use boost::asio instead of experimental version of std because
// currently experimental version of internet is buggy. Hopefully that will be
// fixed later.

#include "std.h"

#if __has_include(<executor>)
#include <executor>
#define STD_IO_CONTEXT STLLIB_STANDARD
#elif __has_include(<boost/asio/ts/executor.hpp>)
#define STD_IO_CONTEXT STLLIB_BOOST
#include <boost/asio/ts/executor.hpp>
namespace std {
    namespace net = boost::asio;
}
#elif __has_include(<experimental/executor>)
#define STD_IO_CONTEXT STLLIB_EXPERIMENTAL
#include <experimental/executor>
namespace std {
    namespace net = experimental::net;
}
#else
#error STLLIB_NETWORKING_ERROR
#endif

#endif // WEBPP_STD_EXECUTOR_H