#ifndef WEBPP_IP_H
#define WEBPP_IP_H

#include "../utils/strings.h"
#include "../validators/validators.h"
#include "casts.h"
#include <array>
#include <sstream>
#include <string>
#include <string_view>

namespace webpp {

    /**
     * @brief considers this ip as a subnet and converts it into a int
     * prefix
     */
    constexpr uint8_t to_prefix(uint32_t octets) noexcept {
        uint8_t prefix = 0u;
        for (uint32_t mask = 0x80'00'00'00u; mask != 0u; mask >>= 1u)
            if ((octets & mask) == mask)
                prefix++;
            else
                return prefix;
        return prefix;
    }

    constexpr uint8_t to_prefix(std::array<uint8_t, 4> octets) noexcept {
        uint8_t prefix = 0u;
        for (auto const& octet : octets)
            for (uint8_t mask = 0b1000'0000; mask != 0u; mask >>= 1u)
                if ((octet & mask) == mask)
                    prefix++;
                else
                    return prefix;
        return prefix;
    }

    /**
     * Convert string to prefix
     * @param octets
     */
    constexpr uint8_t to_prefix(std::string_view const& _data) noexcept {
        if (_data.size() > 15 || _data.size() < 7) {
            return 0u;
        }
        std::size_t first_dot = 0u;
        std::size_t len = _data.size();
        while (_data[first_dot] != '.' && first_dot != len)
            first_dot++;

        auto octet_1 = _data.substr(0u, first_dot);
        if (first_dot == len || octet_1.empty() || octet_1.size() > 3 ||
            !is::digit(octet_1) ||
            (starts_with(octet_1, '0') && octet_1 != "0")) {
            return 0u;
        }

        std::size_t second_dot = first_dot + 1;
        while (_data[second_dot] != '.' && second_dot != len)
            second_dot++;

        auto octet_2 =
            _data.substr(first_dot + 1u, second_dot - (first_dot + 1));
        if (second_dot == len || octet_2.empty() || octet_2.size() > 3 ||
            !is::digit(octet_2) ||
            (starts_with(octet_2, '0') && octet_2 != "0")) {
            return 0u;
        }

        std::size_t third_dot = second_dot + 1;
        while (_data[third_dot] != '.' && third_dot != len)
            third_dot++;

        auto octet_3 =
            _data.substr(second_dot + 1u, third_dot - (second_dot + 1));
        if (first_dot == len || octet_3.empty() || octet_3.size() > 3 ||
            !is::digit(octet_3) ||
            (starts_with(octet_3, '0') && octet_3 != "0")) {
            return 0u; // parsing failed.
        }

        auto octet_4 = _data.substr(third_dot + 1u);

        if (octet_4.empty() || octet_4.size() > 3 || !is::digit(octet_4) ||
            (starts_with(octet_4, '0') && octet_4 != "0")) {
            return 0u;
        }

        return to_prefix({to_uint8(octet_1), to_uint8(octet_2),
                          to_uint8(octet_3), to_uint8(octet_4)});
    }

    /**
     * Convert a prefix to a subnet
     * @param prefix
     * @return bool
     */
    constexpr uint32_t to_subnet(uint8_t prefix) noexcept {
        return 0xFF'FF'FF'FFu << (32u - prefix);
    }

    /**
     * Convert a prefix to a subnet
     * @param prefix
     * @return bool
     */
    constexpr std::array<uint8_t, 4> to_subnet_array(uint8_t prefix) noexcept {
        auto subnet = to_subnet(prefix);
        return {static_cast<uint8_t>(subnet >> 24u & 0xFFu),
                static_cast<uint8_t>(subnet >> 16u & 0xFFu),
                static_cast<uint8_t>(subnet >> 8u & 0xFFu),
                static_cast<uint8_t>(subnet & 0xFFu)};
    }

    class ipv4 {
      private:
        mutable uint32_t data = 0u; // all bits are used

        // 255 means that the ip doesn't have a prefix
        // 254 means the ip is not valid
        // 253 means the prefix was not valid
        mutable uint8_t _prefix = 255u;

        constexpr void parse(std::string_view const& _data) const noexcept {
            if (_data.size() > 15 || _data.size() < 7) {
                _prefix = 254u; // the ip is not valid
                return;
            }
            std::size_t first_dot = 0u;
            std::size_t len = _data.size();
            while (_data[first_dot] != '.' && first_dot != len)
                first_dot++;

            auto octet_1 = _data.substr(0u, first_dot);
            if (first_dot == len || octet_1.empty() || octet_1.size() > 3 ||
                !is::digit(octet_1) ||
                (starts_with(octet_1, '0') && octet_1 != "0")) {
                _prefix = 254u; // the ip is not valid
                return;
            }

            std::size_t second_dot = first_dot + 1;
            while (_data[second_dot] != '.' && second_dot != len)
                second_dot++;

            auto octet_2 =
                _data.substr(first_dot + 1u, second_dot - (first_dot + 1));
            if (second_dot == len || octet_2.empty() || octet_2.size() > 3 ||
                !is::digit(octet_2) ||
                (starts_with(octet_2, '0') && octet_2 != "0")) {
                _prefix = 254u; // the ip is not valid
                return;
            }

            std::size_t third_dot = second_dot + 1;
            while (_data[third_dot] != '.' && third_dot != len)
                third_dot++;

            auto octet_3 =
                _data.substr(second_dot + 1u, third_dot - (second_dot + 1));
            if (third_dot == len || octet_3.empty() || octet_3.size() > 3 ||
                !is::digit(octet_3) ||
                (starts_with(octet_3, '0') && octet_3 != "0")) {
                _prefix = 254u; // the ip is not valid
                return;         // parsing failed.
            }

            std::size_t slash = third_dot + 1;
            while (_data[slash] != '/' && slash != len)
                slash++;

            auto octet_4 =
                _data.substr(third_dot + 1u, slash - (third_dot + 1));

            if (octet_4.empty() || octet_4.size() > 3 || !is::digit(octet_4) ||
                (starts_with(octet_4, '0') && octet_4 != "0")) {
                _prefix = 254u; // the ip is not valid
                return;
            }

            if (slash != len) {
                auto prefix_str = _data.substr(slash + 1);
                if (prefix_str.empty() ||
                    (starts_with(prefix_str, '0') && prefix_str != "0") ||
                    !is::digit(prefix_str)) {
                    _prefix = 254u; // the ip is not valid
                    return;
                }
                auto __prefix = to_uint(prefix_str);
                if (__prefix > 32) {
                    _prefix = 254; // the ip is not valid
                    return;
                }
                _prefix = static_cast<uint8_t>(__prefix);
            }

            auto oc1 = to_uint(octet_1);
            auto oc2 = to_uint(octet_2);
            auto oc3 = to_uint(octet_3);
            auto oc4 = to_uint(octet_4);

            if (oc1 > 255 || oc2 > 255 || oc3 > 255 || oc4 > 255) {
                _prefix = 254u; // the ip is not valid
                return;
            }

            data =
                parse({static_cast<uint8_t>(oc1), static_cast<uint8_t>(oc2),
                       static_cast<uint8_t>(oc3), static_cast<uint8_t>(oc4)});

            if (_prefix == 254u)
                _prefix = 255u; // the ip is valid
        }

        constexpr uint32_t parse(std::array<uint8_t, 4u> const& ip) const
            noexcept {
            return static_cast<uint32_t>(ip[0] << 24u) |
                   static_cast<uint32_t>(ip[1] << 16u) |
                   static_cast<uint32_t>(ip[2] << 8u) |
                   static_cast<uint32_t>(ip[3]);
        }

      public:
        constexpr ipv4(ipv4 const& ip) = default;

        constexpr ipv4(ipv4&& ip) = default;

        constexpr explicit ipv4(std::string_view const& ip) noexcept
            : _prefix(255) {
            parse(ip);
        }

        constexpr explicit ipv4(char const* const ip) noexcept : _prefix(255) {
            parse(ip);
        }

        constexpr ipv4(std::string_view const& ip,
                       std::string_view const& subnet) noexcept
            : _prefix(is::subnet(subnet) ? to_prefix(subnet) : 253u) {
            parse(ip);
        }

        constexpr ipv4(std::string_view const& ip,
                       std::array<uint8_t, 4> const& subnet) noexcept
            : _prefix(is::subnet(subnet) ? to_prefix(subnet) : 253u) {
            parse(ip);
        }

        constexpr ipv4(std::string_view const& ip, uint8_t __prefix) noexcept
            : _prefix(__prefix > 32 && __prefix != 255u ? 253u : __prefix) {
            parse(ip);
        }

        constexpr ipv4(uint8_t octet1, uint8_t octet2, uint8_t octet3,
                       uint8_t octet4, uint8_t prefix = 255) noexcept
            : data(parse({octet1, octet2, octet3, octet4})),
              _prefix(prefix > 32 && prefix != 255u ? 253u : prefix) {}

        constexpr ipv4(uint8_t octet1, uint8_t octet2, uint8_t octet3,
                       uint8_t octet4, std::string_view const& subnet) noexcept
            : data(parse({octet1, octet2, octet3, octet4})),
              _prefix(is::subnet(subnet) ? to_prefix(subnet) : 253u) {}

        constexpr explicit ipv4(uint32_t const& ip,
                                uint8_t prefix = 255) noexcept
            : data(ip), _prefix(prefix > 32 && prefix != 255u ? 253u : prefix) {
        }

        constexpr explicit ipv4(uint32_t const& ip,
                                std::string_view subnet) noexcept
            : data(ip), _prefix(is::subnet(subnet) ? to_prefix(subnet) : 253u) {
        }

        constexpr ipv4(std::array<uint8_t, 4> const& ip,
                       uint8_t prefix = 255) noexcept
            : data(parse(ip)),
              _prefix(prefix > 32 && prefix != 255u ? 253u : prefix) {}

        constexpr ipv4(std::array<uint8_t, 4> const& ip,
                       std::string_view const& subnet) noexcept
            : data(parse(ip)),
              _prefix(is::subnet(subnet) ? to_prefix(subnet) : 253u) {}

        constexpr ipv4(std::array<uint8_t, 4> const& ip,
                       std::array<uint8_t, 4> const& subnet) noexcept
            : data(parse(ip)),
              _prefix(is::subnet(subnet) ? to_prefix(subnet) : 253u) {}

        explicit operator std::string() { return str(); }

        explicit operator const char*() { return str().c_str(); }

        explicit operator uint32_t() { return integer(); }

        ipv4& operator=(ipv4 const& ip) = default;
        ipv4& operator=(ipv4&& ip) = default;

        ipv4& operator=(std::string_view const& ip) noexcept {
            parse(ip);
            _prefix = 255u;
            return *this;
        }

        ipv4& operator=(uint32_t ip) noexcept {
            data = ip;
            _prefix = 255u;
            return *this;
        }

        constexpr bool operator==(std::array<uint8_t, 4> const& other) const
            noexcept {
            return data == parse(other);
        }

        constexpr bool operator!=(std::array<uint8_t, 4> const& other) const
            noexcept {
            return data != parse(other);
        }

        constexpr bool operator<(std::array<uint8_t, 4> const& other) const
            noexcept {
            return data < parse(other);
        }

        constexpr bool operator>(std::array<uint8_t, 4> const& other) const
            noexcept {
            return data > parse(other);
        }

        constexpr bool operator<=(std::array<uint8_t, 4> const& other) const
            noexcept {
            return data <= parse(other);
        }

        constexpr bool operator>=(std::array<uint8_t, 4> const& other) const
            noexcept {
            return data >= parse(other);
        }

        constexpr bool operator==(ipv4 const& other) const noexcept {
            return data == other.data && _prefix == other._prefix;
        }

        constexpr bool operator!=(ipv4 const& other) const noexcept {
            return !operator==(other);
        }

        constexpr bool operator<(ipv4 const& other) const noexcept {
            return data < other.data;
        }

        constexpr bool operator>(ipv4 const& other) const noexcept {
            return data > other.data;
        }

        constexpr bool operator>=(ipv4 const& other) const noexcept {
            return data >= other.data;
        }

        constexpr bool operator<=(ipv4 const& other) const noexcept {
            return data <= other.data;
        }

        constexpr bool operator!=(std::string_view ip) const noexcept {
            return operator!=(ipv4(ip));
        }

        constexpr bool operator==(std::string_view ip) const noexcept {
            return operator==(ipv4(ip));
        }

        constexpr bool operator<(std::string_view ip) const noexcept {
            return operator<(ipv4(ip));
        }

        constexpr bool operator>(std::string_view ip) const noexcept {
            return operator>(ipv4(ip));
        }

        constexpr bool operator<=(std::string_view ip) const noexcept {
            return operator<=(ipv4(ip));
        }

        constexpr bool operator>=(std::string_view ip) const noexcept {
            return operator>=(ipv4(ip));
        }

        constexpr bool operator==(uint32_t const& ip) const noexcept {
            return integer() == ip;
        }

        constexpr bool operator!=(uint32_t const& ip) const noexcept {
            return !operator==(ip);
        }

        constexpr bool operator<(uint32_t const& ip) const noexcept {
            return integer() < ip;
        }

        constexpr bool operator>(uint32_t const& ip) const noexcept {
            return integer() > ip;
        }

        constexpr bool operator<=(uint32_t const& ip) const noexcept {
            return integer() <= ip;
        }

        constexpr bool operator>=(uint32_t const& ip) const noexcept {
            return integer() >= ip;
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
        [[nodiscard]] std::string str() const noexcept {
            auto _octets = octets();
            std::ostringstream s;
            s << static_cast<unsigned int>(_octets[0]) << '.'
              << static_cast<unsigned int>(_octets[1]) << '.'
              << static_cast<unsigned int>(_octets[2]) << '.'
              << static_cast<unsigned int>(_octets[3]);
            return s.str();
        }

        /**
         * @brief get the integer representation of the ip address
         * @return
         */
        [[nodiscard]] constexpr uint32_t integer() const noexcept {
            return data;
        }

        /**
         * @brief get the 4 octets of the ip address
         * @return
         */
        [[nodiscard]] constexpr std::array<uint8_t, 4u> octets() const
            noexcept {
            uint32_t _data = integer();
            return std::array<uint8_t, 4u>(
                {static_cast<uint8_t>(_data >> 24u),
                 static_cast<uint8_t>(_data >> 16u & 0x0FFu),
                 static_cast<uint8_t>(_data >> 8u & 0x0FFu),
                 static_cast<uint8_t>(_data & 0x0FFu)});
        }

        /**
         * @brief check if the ip is in the specified range or not
         * @param start
         * @param finish
         * @return
         */
        [[nodiscard]] constexpr bool in_range(ipv4 const& start,
                                              ipv4 const& finish) const
            noexcept {
            return *this >= start && *this <= finish;
        }

        /**
         * Get the prefix you specified in the constructor
         * @return
         */
        [[nodiscard]] constexpr auto prefix() const noexcept { return _prefix; }

        /**
         * Change the prefix of the ip
         * @param __prefix
         */
        ipv4& prefix(uint8_t __prefix) noexcept {
            _prefix = __prefix > 32 && __prefix != 255u ? 253u : __prefix;
            return *this;
        }

        /**
         * Set prefix with a subnet string
         * @param _subnet
         */
        ipv4& prefix(std::string_view const& _subnet) noexcept {
            return prefix(to_prefix(_subnet));
        }

        /**
         * Set prefix with a subnet array
         * @param _subnet
         */
        ipv4& prefix(std::array<uint8_t, 4> const& _subnet) noexcept {
            return prefix(to_prefix(_subnet));
        }

        /**
         * Remove prefix from the ip address
         */
        ipv4& clear_prefix() noexcept { return prefix(255u); }

        /**
         * Check if the ip contains a prefix or not
         * @return bool an indication on weather or not the ip contains a prefix
         * or not
         */
        [[nodiscard]] constexpr bool has_prefix() const noexcept {
            return _prefix <= 32;
        }

        /**
         * Check if the specified subnet or prefix was valid or not
         * @return bool
         */
        [[nodiscard]] constexpr bool has_valid_prefix() const noexcept {
            return _prefix != 253u;
        }

        /**
         * @brief checks if the ip in this class is in the specified subnet or
         * not regardless of the the prefix that is specified in the ctor
         * @param ip
         * @param prefix
         * @return bool
         */
        [[nodiscard]] constexpr bool is_in_subnet(ipv4 const& ip) const
            noexcept {
            auto uint_val = integer();
            auto uint_ip = ip.integer();
            uint_val &= 0xFFFFFFFFu << (32u - ip.prefix());
            uint_ip &= 0xFFFFFFFFu << (32u - ip.prefix());
            return uint_val == uint_ip;
        }

        /**
         * @brief checks if the ip is in private range or not regardless of the
         * prefix
         * @return
         */
        [[nodiscard]] constexpr bool is_private() const noexcept {
            constexpr ipv4 class_C(std::array<uint8_t, 4u>{192, 168, 0, 0}, 16);
            constexpr ipv4 class_B_start(
                std::array<uint8_t, 4u>{172, 16, 0, 0});
            constexpr ipv4 class_B_finish(
                std::array<uint8_t, 4u>{172, 31, 255, 255});
            constexpr ipv4 class_A(std::array<uint8_t, 4u>{10, 0, 0, 0}, 8);
            return is_in_subnet(class_C) ||
                   in_range(class_B_start, class_B_finish) ||
                   is_in_subnet(class_A);
        }

        /**
         * @brief checks if the ip address is in public range or not
         * @return
         */
        [[nodiscard]] constexpr bool is_public() const noexcept {
            return !is_private();
        }

        /**
         * @brief check if all the octets are zero or not
         * @return true if all the octets are zero
         */
        [[nodiscard]] constexpr bool is_zero() const noexcept {
            return data == 0;
        }

        /**
         * Check if the ip you specified is valid or not (the ctor will not
         * throw an error if the specified string is not a valid ipv4 address)
         * @return bool
         */
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return _prefix != 254u;
        }

        /**
         * Get the ip in reversed order
         * @return
         */
        [[nodiscard]] constexpr ipv4 reversed() const noexcept {
            return std::array<uint8_t, 4>{
                static_cast<uint8_t>(data & 0xFFu),
                static_cast<uint8_t>(data >> 8u & 0xFFu),
                static_cast<uint8_t>(data >> 16u & 0xFFu),
                static_cast<uint8_t>(data >> 24u & 0xFFu)};
        }

        /**
         * TODO: implement this thing
         * @brief get the geographical location of the ip address based on
         * predefined rules
         * @return coordinates or string location
         */
        [[nodiscard]] std::string geographic_location() const noexcept;
    };

    constexpr bool operator==(uint32_t const& one, ipv4 const& two) {
        return two == one;
    }

    constexpr bool operator!=(uint32_t const& one, ipv4 const& two) {
        return two == one;
    }

    constexpr bool operator<(uint32_t const& one, ipv4 const& two) {
        return one < two.integer();
    }

    constexpr bool operator>(uint32_t const& one, ipv4 const& two) {
        return one > two.integer();
    }

    constexpr bool operator<=(uint32_t const& one, ipv4 const& two) {
        return one <= two.integer();
    }

    constexpr bool operator>=(uint32_t const& one, ipv4 const& two) {
        return one >= two.integer();
    }

    constexpr bool operator==(std::string_view const& one, ipv4 const& two) {
        return two == one;
    }

    constexpr bool operator!=(std::string_view const& one, ipv4 const& two) {
        return two != one;
    }

    constexpr bool operator<(std::string_view const& one, ipv4 const& two) {
        return ipv4(one) < two;
    }

    constexpr bool operator>(std::string_view const& one, ipv4 const& two) {
        return ipv4(one) > two;
    }

    constexpr bool operator<=(std::string_view const& one, ipv4 const& two) {
        return ipv4(one) <= two;
    }

    constexpr bool operator>=(std::string_view const& one, ipv4 const& two) {
        return ipv4(one) >= two;
    }

    constexpr bool operator==(std::array<uint8_t, 4> const& one,
                              ipv4 const& two) {
        return two == one;
    }

    constexpr bool operator!=(std::array<uint8_t, 4> const& one,
                              ipv4 const& two) {
        return two != one;
    }

    constexpr bool operator<(std::array<uint8_t, 4> const& one,
                             ipv4 const& two) {
        return ipv4(one) < two;
    }

    constexpr bool operator>(std::array<uint8_t, 4> const& one,
                             ipv4 const& two) {
        return ipv4(one) > two;
    }

    constexpr bool operator<=(std::array<uint8_t, 4> const& one,
                              ipv4 const& two) {
        return ipv4(one) <= two;
    }

    constexpr bool operator>=(std::array<uint8_t, 4> const& one,
                              ipv4 const& two) {
        return ipv4(one) >= two;
    }

} // namespace webpp

#endif // WEBPP_IP_H
