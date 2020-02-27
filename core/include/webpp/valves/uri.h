#ifndef WEBPP_VALVE_URI_H
#define WEBPP_VALVE_URI_H

#include "valve.h"
#include "../../../include/webpp/utils/uri.h"
#include <cstddef> // for std::size_t
#include <string_view>

namespace webpp::valves {

    struct path_condition {
      protected:
        const_uri _path;

      public:
        constexpr path_condition(std::string_view str) noexcept
            : _path(std::move(str)) {}

        constexpr path_condition() noexcept = default;

        template <typename RequestType>
        [[nodiscard]] bool operator()(RequestType const& req) const noexcept {
            return equal_path(req.request_uri(), _path);
        }
    };

    struct path : public valve<path_condition> {
        using valve<path_condition>::valve;
    };

    constexpr path operator""_path(const char* str, std::size_t len) {
        return path{std::string_view{str, len}};
    }

    // templated path

    struct tpath_condition {
      protected:
        std::string_view tpath_str;

      public:
        constexpr tpath_condition(std::string_view str) noexcept
            : tpath_str(str) {}

        constexpr tpath_condition() noexcept = default;

        template <typename RequestType>
        [[nodiscard]] bool operator()(RequestType const& req) const noexcept {
            // TODO: change this algorithm
            return req.request_uri() == tpath_str;
        }
    };

    struct tpath : public valve<tpath_condition> {
        using valve<tpath_condition>::valve;
    };

    constexpr tpath operator""_tpath(const char* str, std::size_t len) {
        return tpath{std::string_view{str, len}};
    }

} // namespace webpp::valves

#endif // WEBPP_VALVE_URI_H