#ifndef WEBPP_IP_H
#define WEBPP_IP_H

#include "../std/string_view.h"
#include "casts.h"
#include <array>
#include <sstream>
#include <string>
#include <variant>

namespace webpp {

    class ipv4 {
      private:
        mutable std::variant<uint32_t, std::string_view> data;

        constexpr uint32_t parse(std::string_view const& _data) const noexcept {
            std::size_t first_dot = 0;
            std::size_t len = _data.size();
            while (_data[first_dot] != '.' && first_dot != len)
                first_dot++;
            std::size_t second_dot = first_dot;
            while (_data[second_dot] != '.' && second_dot != len)
                second_dot++;
            std::size_t third_dot = second_dot;
            while (_data[third_dot] != '.' && third_dot != len)
                third_dot++;

            //            if (first_dot == std::string_view::npos ||
            //                second_dot == std::string_view::npos ||
            //                third_dot == std::string_view::npos)
            //                throw std::invalid_argument("ill formed IPv4
            //                address.");
            return parse({to_uint8(_data.substr(0, first_dot)),
                          to_uint8(_data.substr(first_dot + 1, second_dot)),
                          to_uint8(_data.substr(second_dot + 1, third_dot)),
                          to_uint8(_data.substr(third_dot + 1))});
        }

        constexpr uint32_t parse(std::array<uint8_t, 4> const& ip) const
            noexcept {
            return static_cast<uint32_t>(ip[0] << 24) |
                   static_cast<uint32_t>(ip[1] << 16) |
                   static_cast<uint32_t>(ip[2] << 8) |
                   static_cast<uint32_t>(ip[3]);
        }

      public:
        constexpr explicit ipv4(std::string_view const& ip) noexcept
            : data(ip) {}
        constexpr explicit ipv4(std::string_view&& ip) noexcept
            : data(parse(ip)) {}
        constexpr explicit ipv4(ipv4 const& ip) noexcept : data(ip.data) {}
        constexpr explicit ipv4(ipv4&& ip) noexcept
            : data(std::move(ip.data)) {}
        constexpr explicit ipv4(uint32_t const& ip) noexcept : data(ip) {}
        constexpr explicit ipv4(std::array<uint8_t, 4> const& ip) noexcept
            : data(parse(ip)) {}

        ipv4& operator=(ipv4 const& ip) noexcept {
            data = ip.data;
            return *this;
        }

        ipv4& operator=(ipv4&& ip) noexcept {
            if (ip.data != data)
                data = std::move(ip.data);
            return *this;
        }

        ipv4& operator=(std::string_view const& ip) noexcept {
            data = ip;
            return *this;
        }

        ipv4& operator=(uint32_t ip) noexcept {
            data = ip;
            return *this;
        }

        ipv4& operator=(std::string&& ip) noexcept {
            data = parse(ip);
            return *this;
        }

        constexpr bool operator==(ipv4 const& other) const noexcept {
            return integer() == other.integer();
        }

        constexpr bool operator!=(ipv4 const& other) const noexcept {
            return !operator==(other);
        }

        constexpr bool operator<(ipv4 const& other) const noexcept {
            return integer() < other.integer();
        }

        constexpr bool operator>(ipv4 const& other) const noexcept {
            return integer() > other.integer();
        }

        constexpr bool operator>=(ipv4 const& other) const noexcept {
            return integer() >= other.integer();
        }

        constexpr bool operator<=(ipv4 const& other) const noexcept {
            return integer() <= other.integer();
        }

        friend std::ostream& operator<<(std::ostream& stream, ipv4 const& ip) {
            stream << ip.str();
            return stream;
        }

        friend std::istream& operator>>(std::istream& stream, ipv4& ip) {
            std::string str;
            stream >> str;
            ip = str;
            return stream;
        }

        /**
         * @brief get string representation of the ip
         * @return
         */
        std::string str() const noexcept {
            if (std::holds_alternative<uint32_t>(data)) {
                auto _octets = octets();
                std::ostringstream s;
                s << std::move(_octets[0]) << '.' << std::move(_octets[1])
                  << '.' << std::move(_octets[2]) << '.'
                  << std::move(_octets[3]);
                return s.str();
            }
            return std::string(std::get<std::string_view>(data));
        }

        /**
         * @brief get the integer representation of the ip address
         * @return
         */
        constexpr uint32_t integer() const noexcept {
            if (std::holds_alternative<std::string_view>(data)) {
                data = parse(std::get<std::string_view>(data));
            }
            return std::get<uint32_t>(data);
        }

        /**
         * @brief get the 4 octets of the ip address
         * @return
         */
        constexpr std::array<uint8_t, 4> octets() const noexcept {
            uint32_t _data = integer();
            return std::array<uint8_t, 4>(
                {static_cast<uint8_t>(_data >> 24),
                 static_cast<uint8_t>(_data >> 16 & 0x0FFu),
                 static_cast<uint8_t>(_data >> 8 & 0x0FFu),
                 static_cast<uint8_t>(_data & 0x0FFu)});
        }

        /**
         * @brief check if the ip is in the specified range or not
         * @param start
         * @param finish
         * @return
         */
        constexpr bool in_range(ipv4 const& start, ipv4 const& finish) const
            noexcept {
            return *this >= start && *this <= finish;
        }

        /**
         * @brief checks if the ip in this class is in the specified subnet or
         * not
         * @param ip
         * @param prefix
         * @return
         */
        constexpr bool in_subnet(ipv4 const& ip, unsigned int prefix) const
            noexcept {
            auto uint_val = integer();
            auto uint_ip = ip.integer();
            uint_val &= 0xFFFFFFFFu >> (32u - prefix);
            uint_ip &= 0xFFFFFFFFu >> (32u - prefix);
            return uint_val == uint_ip;
        }

        /**
         * @brief checks if the ip in this class is in the specified subnet or
         * not
         * @param ip
         * @param subnet_mask
         * @return
         */
        constexpr bool in_subnet(ipv4 const& ip, ipv4 const& subnet_mask) const
            noexcept {
            return in_subnet(ip, subnet_mask.to_prefix());
        }

        /**
         * @brief considers this ip as a subnet and converts it into a int
         * prefix
         * @return
         */
        constexpr unsigned int to_prefix() const noexcept {
            uint32_t val = integer();
            unsigned int prefix = 0;
            for (; val != 0; prefix++)
                val <<= 1;
            return prefix;
        }

        /**
         * @brief checks if the ip is in private range or not
         * @return
         */
        constexpr bool is_private() const noexcept {
            constexpr ipv4 class_C({192, 168, 0, 0});
            constexpr ipv4 class_B_start({172, 16, 0, 0});
            constexpr ipv4 class_B_finish({172, 31, 255, 255});
            constexpr ipv4 class_A({10, 0, 0, 0});
            return in_subnet(class_C, 16) ||
                   in_range(class_B_start, class_B_finish) ||
                   in_subnet(class_A, 8);
        }

        /**
         * @brief checks if the ip address is in public range or not
         * @return
         */
        constexpr bool is_public() const noexcept { return !is_private(); }

        std::string geographic_location() const noexcept {
            // TODO: find a way to get this info
            return "";
        }
    };

} // namespace webpp

#endif // WEBPP_IP_H