#ifndef WEBPP_IPV6_H
#define WEBPP_IPV6_H

#include "../std/string_view.h"
#include "../validators/validators.h"
#include "ipv4.h"
#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace webpp {

    class ipv6 {
      public:
        static constexpr auto IPV6_ADDR_SIZE = 16u; // Bytes
        using octets8_t = std::array<uint8_t, 16u>;
        using octets16_t = std::array<uint16_t, 8u>;
        using octets32_t = std::array<uint32_t, 4u>;
        using octets64_t = std::array<uint64_t, 2u>;
        using octets_t = octets8_t;

        /**
         * IPv6 Address Scopes
         */
        enum class scope {
            node_local = 0u,      // Node-Local scope
            interface_local = 1u, // Interface-Local scope
            link_local = 2u,      // Link-Local scope
            realm_local = 3u,     // Realm-Local scope
            admin_local = 4u,     // Admin-Local scope
            site_local = 5u,      // Site-Local scope
            org_local = 8u,       // Organization-Local scope
            global = 14u,         // Global scope
        };

      private:
        static constexpr auto interface_identifier_offset =
            8u; // Interface Identifier offset in bytes
        static constexpr auto interface_identifier_size =
            8u; // Interface Identifier size in bytes

        // I didn't go with a union because in OpenThread project they did and
        // they had to deal with endianness of their data. I rather use shifts
        // and host's byte order instead of getting my hands dirty with host's
        // byte order. Network's byte order is big endian btw, but here we just
        // have to worry about the host's byte order because we are not sending
        // these data over the network.
        mutable octets_t data = {}; // filled with zeros

        // 255 means it's doesn't have prefix
        // 254 means the ip is not valid
        // 253 means the prefix is not valid (it's not being used for now)
        mutable uint8_t _prefix = 255u;

        /**
         * converts 16/32/64/... bit arrays to 8bit
         * @tparam OCTET
         * @param _octets
         * @return octets8_t so I could put it in the "data"
         */
        template <typename OCTET>
        static constexpr octets_t to_octets_t(OCTET const& _octets) noexcept {
            octets_t _data = {};
            auto _octets_it = _octets.cbegin();
            auto _data_it = _data.begin();
            auto each_octet_size = _data.size() / _octets.size();
            for (; _octets_it != _octets.cend(); ++_octets_it) {
                auto _octet = *_octets_it;
                for (std::size_t i = 0u; i < each_octet_size; i++) {
                    _octet >>= i * 8u;
                    _octet |= 0xFFu;
                    *_data_it++ = static_cast<uint8_t>(*_octets_it);
                }
            }
            return _data;
        }

        /**
         * parses the string_view to the uint8 structure
         */
        constexpr void parse(std::string_view ipv6_data) const noexcept {
            constexpr auto hexes = HEXDIG.string_view();

            if (ipv6_data.empty()) {
                _prefix = 254u;
                return;
            }

            data = {}; // all zero
            auto it = data.begin();
            auto double_colon_point = data.end();

            do {
                if (it == data.cend() && !ipv6_data.starts_with('/')) {
                    _prefix = 254u; // the ip has too many octets
                    return;
                }
                auto colon = ipv6_data.find_first_not_of(hexes);
                if (colon == std::string_view::npos ||
                    ipv6_data[colon] == ':' ||
                    (colon != 0 && ipv6_data[colon] == '/')) {
                    // it's an octet
                    switch (colon == std::string_view::npos ? ipv6_data.size()
                                                            : colon) {
                    case 4:
                        *(it++) = std::stoul(
                            std::string(ipv6_data.substr(0, 2)), nullptr, 16);
                        *(it++) = std::stoul(
                            std::string(ipv6_data.substr(2, 2)), nullptr, 16);
                        break;
                    case 3:
                        *(it++) = std::stoul(
                            std::string(ipv6_data.substr(0, 1)), nullptr, 16);
                        *(it++) = std::stoul(
                            std::string(ipv6_data.substr(1, 2)), nullptr, 16);
                        break;
                    case 2:
                    case 1:
                        *((++it)++) =
                            std::stoul(std::string(ipv6_data.substr(0, colon)),
                                       nullptr, 16);
                        break;
                    case 0:
                        // we've reached the double colon rule
                        if (double_colon_point != data.end()) {
                            _prefix = 254u; // we can't have two double colons
                            return;
                        } else {
                            double_colon_point = it;
                            if (it == data.begin())
                                ipv6_data.remove_prefix(1);
                        }
                        break;
                    default:
                        _prefix = 254u; // the ip is invalid
                        return;
                    }
                    if (ipv6_data[colon] == '/')
                        colon--;
                } else if (ipv6_data[colon] == '.') {
                    // we found an ipv4 address
                    ipv4 ip(ipv6_data);
                    if (!ip.is_valid()) {
                        _prefix = 254u; // the ip is not valid
                        return;
                    }
                    for (auto const& octet : ip.octets())
                        *(it++) = octet;
                    break;
                } else if (ipv6_data[colon] == '/') {
                    // we have a prefix
                    auto prefix_str = ipv6_data.substr(colon + 1);
                    if (!is::digit(prefix_str)) {
                        _prefix = 253u; // the prefix is invalid
                        break;          // let's not go all crazy just yet
                    }
                    auto __prefix = std::stoul(std::string(prefix_str));
                    // if (prefix_str.starts_with('0') && __prefix != 0) {
                    //     // there can't be a leading zero in the prefix string
                    //     _prefix = 253u;
                    //     return;
                    // }
                    _prefix = __prefix > 128u
                                  ? 253u
                                  : static_cast<decltype(_prefix)>(__prefix);
                    ipv6_data.remove_prefix(prefix_str.size() + 1);
                    if (!ipv6_data.empty()) {
                        _prefix = 254u; // there can't be more stuff in the ip
                                        // from now on.
                        return;
                    }
                    break; // there's nothing in the loop for us anymore
                } else {
                    _prefix = 254u; // the ip is not valid
                    return;
                }
                if (colon != std::string_view::npos)
                    ipv6_data.remove_prefix(colon + 1);
                else
                    break;
            } while (!ipv6_data.empty());

            if (double_colon_point != data.end() && it != data.end()) {
                // XXXX XXXX XXXX XXXX YYYY YYYY 0000 0000
                //                    ^         ^
                //               double colon   it
                // shift the values to the last
                std::rotate(double_colon_point, it, data.end());
            } else if (it != data.end()) {
                _prefix = 254u; // the string does not contain the whole ip
                return;
            }
        }

      public:
        constexpr explicit ipv6(std::string_view const& str,
                                uint8_t __prefix = 255u) noexcept
            : _prefix(__prefix > 128u && __prefix != 255u ? 253u : __prefix) {
            parse(str);
        }
        constexpr explicit ipv6(octets8_t const& _octets,
                                uint8_t __prefix = 255u) noexcept
            : data(_octets),
              _prefix(__prefix > 128u && __prefix != 255u ? 253u : __prefix) {}
        constexpr explicit ipv6(octets16_t const& _octets,
                                uint8_t __prefix = 255u) noexcept
            : data{to_octets_t(_octets)},
              _prefix(__prefix > 128u && __prefix != 255u ? 253u : __prefix) {}

        constexpr explicit ipv6(octets32_t const& _octets,
                                uint8_t __prefix = 255u) noexcept
            : data{to_octets_t(_octets)},
              _prefix(__prefix > 128u && __prefix != 255u ? 253u : __prefix) {}
        constexpr explicit ipv6(octets64_t const& _octets,
                                uint8_t __prefix = 255u) noexcept
            : data{to_octets_t(_octets)},
              _prefix(__prefix > 128u && __prefix != 255u ? 253u : __prefix) {}
        constexpr ipv6(ipv6 const& ip) noexcept = default;
        constexpr ipv6(ipv6&& ip) noexcept = default;

        ipv6& operator=(ipv6 const& ip) noexcept = default;

        ipv6& operator=(std::string_view const& str) noexcept {
            parse(str);
            _prefix = 255u;
            return *this;
        }

        ipv6& operator=(octets8_t const& _octets) noexcept {
            data = _octets;
            _prefix = 255u;
            return *this;
        }

        ipv6& operator=(octets16_t const& _octets) noexcept {
            data = to_octets_t(_octets);
            _prefix = 255u;
            return *this;
        }

        ipv6& operator=(octets32_t const& _octets) noexcept {
            data = to_octets_t(_octets);
            _prefix = 255u;
            return *this;
        }

        ipv6& operator=(octets64_t const& _octets) noexcept {
            data = to_octets_t(_octets);
            _prefix = 255u;
            return *this;
        }

        bool operator==(ipv6 const& other) const noexcept {
            return data == other.data && _prefix == other._prefix;
        }

        bool operator!=(ipv6 const& other) const noexcept {
            return !operator==(other);
        }

        // TODO: add other operators

        explicit operator octets8_t() { return octets8(); }

        explicit operator octets16_t() { return octets16(); }

        explicit operator octets32_t() { return octets32(); }

        explicit operator octets64_t() { return octets64(); }

        explicit operator const char*() { return short_str().c_str(); }

        explicit operator std::string() { return short_str(); }

        /**
         * @brief get the octets in 8bit format
         * @return the octets in 8bit format
         */
        constexpr octets8_t octets8() const noexcept { return data; }

        /**
         * @brief get all the octets in 8bit format
         * @details same as octets8 method
         */
        constexpr octets_t octets() const noexcept { return octets8(); }

        /**
         * @brief return all the octets in 16bit format
         */
        constexpr octets16_t octets16() const noexcept {
            // IP: XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
            // 08: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
            // 16: --0-- --1-- --2-- --3-- --4-- --5-- --6-- --7--
            // 32: -----0----- -----1----- -----2----- -----3-----
            // 64: -----------0----------- -----------1-----------

            auto _octets = octets8();
            octets16_t ndata = {};
            constexpr std::size_t len = ndata.size();
            using t = uint16_t;
            for (std::size_t i = 0; i < len; i++) {
                ndata[i] =
                    (static_cast<t>(_octets[i * 2u + 0u]) << (16u - 8u * 1u)) |
                    (static_cast<t>(_octets[i * 2u + 1u]) << (16u - 8u * 2u));
            }
            return ndata;
        }

        /**
         * @brief return all octets in 32bit format
         */
        constexpr octets32_t octets32() const noexcept {
            // IP: XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
            // 08: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
            // 16: --0-- --1-- --2-- --3-- --4-- --5-- --6-- --7--
            // 32: -----0----- -----1----- -----2----- -----3-----
            // 64: -----------0----------- -----------1-----------

            auto _octets = octets8();
            octets32_t ndata = {};
            constexpr std::size_t len = ndata.size();
            using t = uint32_t;
            for (std::size_t i = 0; i < len; i++) {
                ndata[i] =
                    (static_cast<t>(_octets[i * 2u + 0u]) << (32u - 8u * 1u)) |
                    (static_cast<t>(_octets[i * 2u + 1u]) << (32u - 8u * 2u)) |
                    (static_cast<t>(_octets[i * 2u + 2u]) << (32u - 8u * 3u)) |
                    (static_cast<t>(_octets[i * 2u + 3u]) << (32u - 8u * 4u));
            }
            return ndata;
        }

        /**
         * @brief return all octets in 64bit format
         */
        constexpr octets64_t octets64() const noexcept {
            // IP: XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX
            // 08: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
            // 16: --0-- --1-- --2-- --3-- --4-- --5-- --6-- --7--
            // 32: -----0----- -----1----- -----2----- -----3-----
            // 64: -----------0----------- -----------1-----------

            auto _octets = octets8();
            octets64_t ndata = {};
            constexpr std::size_t len = ndata.size();
            using t = uint64_t;
            for (std::size_t i = 0; i < len; i++) {
                ndata[i] =
                    (static_cast<t>(_octets[i * 2u + 0u]) << (64u - 8u * 1u)) |
                    (static_cast<t>(_octets[i * 2u + 1u]) << (64u - 8u * 2u)) |
                    (static_cast<t>(_octets[i * 2u + 2u]) << (64u - 8u * 3u)) |
                    (static_cast<t>(_octets[i * 2u + 3u]) << (64u - 8u * 4u)) |
                    (static_cast<t>(_octets[i * 2u + 4u]) << (64u - 8u * 5u)) |
                    (static_cast<t>(_octets[i * 2u + 5u]) << (64u - 8u * 6u)) |
                    (static_cast<t>(_octets[i * 2u + 6u]) << (64u - 8u * 7u)) |
                    (static_cast<t>(_octets[i * 2u + 7u]) << (64u - 8u * 8u));
            }
            return ndata;
        }

        /**
         * This method returns the IPv6 address scope.
         * @returns The IPv6 address scope.
         */
        constexpr uint8_t scope() const noexcept {
            if (is_multicast()) {
                return octets8()[1] & 0xfu;
            } else if (is_link_local()) {
                return static_cast<uint8_t>(scope::link_local);
            } else if (is_loopback()) {
                return static_cast<uint8_t>(scope::node_local);
            }
            return static_cast<uint8_t>(scope::global);
        }

        /**
         * This method indicates whether or not the IPv6 address is the
         * Unspecified Address.
         * Unspecified IPv6 Address == ::0
         *
         * @retval TRUE   If the IPv6 address is the Unspecified Address.
         * @retval FALSE  If the IPv6 address is not the Unspecified Address.
         *
         */
        constexpr bool is_unspecified() const noexcept {
            auto _octets = octets8();
            return (_octets[0] == 0) && (_octets[1] == 0) &&
                   (_octets[2] == 0) && (_octets[3] == 0) &&
                   (_octets[4] == 0) && (_octets[5] == 0) &&
                   (_octets[6] == 0) && (_octets[7] == 0) &&
                   (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0) && (_octets[11] == 0) &&
                   (_octets[12] == 0) && (_octets[13] == 0) &&
                   (_octets[14] == 0) && (_octets[15] == 0);
        }

        /**
         * This method indicates whether or not the IPv6 address is the Loopback
         * Address.
         *
         * @retval TRUE   If the IPv6 address is the Loopback Address.
         * @retval FALSE  If the IPv6 address is not the Loopback Address.
         *
         */
        constexpr bool is_loopback() const noexcept {
            auto _octets = octets8();
            return (_octets[0] == 0) && (_octets[1] == 0) &&
                   (_octets[2] == 0) && (_octets[3] == 0) &&
                   (_octets[4] == 0) && (_octets[5] == 0) &&
                   (_octets[6] == 0) && (_octets[7] == 0) &&
                   (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0) && (_octets[11] == 0) &&
                   (_octets[12] == 0) && (_octets[13] == 0) &&
                   (_octets[14] == 0) && (_octets[15] == 1);
        }

        /**
         * This method indicates whether or not the IPv6 address scope is
         * Interface-Local.
         *
         * @retval TRUE   If the IPv6 address scope is Interface-Local.
         * @retval FALSE  If the IPv6 address scope is not Interface-Local.
         *
         */
        constexpr bool is_link_local() const noexcept {
            auto _octets = octets8();
            return (_octets[0] == 0xfeu) && ((_octets[1] & 0xc0u) == 0x80u);
        }

        /**
         * This method indicates whether or not the IPv6 address is multicast
         * address.
         *
         * @retval TRUE   If the IPv6 address is a multicast address.
         * @retval FALSE  If the IPv6 address scope is not a multicast address.
         *
         */
        constexpr bool is_multicast() const noexcept {
            auto _octets = octets8();
            return _octets[0] == 0xffu;
        }

        /**
         * Determine whether the address is a global multicast address
         * @return bool
         */
        constexpr bool is_multicast_global() const noexcept {
            auto _octets = octets8();
            return ((_octets[0] == 0xffu) && ((_octets[1] & 0x0fu) == 0x0eu));
        }

        /**
         * Determine whether the address is a link-local multicast address
         * @return bool
         */
        constexpr bool is_multicast_link_local() const noexcept {
            auto _octets = octets8();
            return ((_octets[0] == 0xffu) && ((_octets[1] & 0x0fu) == 0x02u));
        }

        /**
         * Determine whether the address is a node-local multicast address
         * @return bool
         */
        constexpr bool is_multicast_node_local() const noexcept {
            auto _octets = octets8();
            return ((_octets[0] == 0xffu) && ((_octets[1] & 0x0fu) == 0x01u));
        }

        /**
         * Determine whether the address is a org-local multicast address
         * @return bool
         */
        constexpr bool is_multicast_org_local() const noexcept {
            auto _octets = octets8();
            return ((_octets[0] == 0xffu) && ((_octets[1] & 0x0fu) == 0x08u));
        }

        /**
         * Determine whether the address is a site-local multicast address
         * @return bool
         */
        constexpr bool is_multicast_site_local() const noexcept {
            auto _octets = octets8();
            return ((_octets[0] == 0xffu) && ((_octets[1] & 0x0fu) == 0x05u));
        }

        /**
         * This method indicates whether or not the IPv6 address scope is
         * Interface-Local.
         *
         * @retval TRUE   If the IPv6 address scope is Interface-Local.
         * @retval FALSE  If the IPv6 address scope is not Interface-Local.
         *
         */
        bool IsInterfaceLocal() const noexcept;

        /**
         * Determine whether the address is site local
         * @return bool
         */
        constexpr bool is_site_local() const noexcept {
            auto _octets = octets8();
            return (_octets[0] == 0xfeu) && ((_octets[1] & 0xc0u) == 0xc0u);
        }

        /**
         * Determine whether the address is a mapped IPv4 address
         * @return bool
         */
        constexpr bool is_v4_mapped() const noexcept {
            auto _octets = octets8();
            return (_octets[0] == 0) && (_octets[1] == 0) &&
                   (_octets[2] == 0) && (_octets[3] == 0) &&
                   (_octets[4] == 0) && (_octets[5] == 0) &&
                   (_octets[6] == 0) && (_octets[7] == 0) &&
                   (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0xff) && (_octets[11] == 0xff);
        }

        /**
         * This method indicates whether or not the IPv6 address is a link-local
         * multicast address.
         *
         * @retval TRUE   If the IPv6 address is a link-local multicast address.
         * @retval FALSE  If the IPv6 address scope is not a link-local
         * multicast address.
         *
         */
        constexpr bool is_link_local_multicast() const noexcept {
            return is_multicast() &&
                   scope() == static_cast<uint8_t>(scope::link_local);
        }

        /**
         * This method indicates whether or not the IPv6 address is a link-local
         * all nodes multicast address.
         *
         * @retval TRUE   If the IPv6 address is a link-local all nodes
         * multicast address.
         * @retval FALSE  If the IPv6 address is not a link-local all nodes
         * multicast address.
         *
         */
        constexpr bool is_link_local_all_nodes_multicast() const noexcept {
            auto _octets = octets8();
            return _octets[0] == 0xFFu && _octets[1] == 0x02u &&
                   (_octets[2] == 0) && (_octets[3] == 0) &&
                   (_octets[4] == 0) && (_octets[5] == 0) &&
                   (_octets[6] == 0) && (_octets[7] == 0) &&
                   (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0) && (_octets[11] == 0) &&
                   (_octets[12] == 0) && (_octets[13] == 0) &&
                   (_octets[14] == 0) && (_octets[15] == 0x01u);
        }

        /**
         * This method indicates whether or not the IPv6 address is a link-local
         * all routers multicast address.
         *
         * @retval TRUE   If the IPv6 address is a link-local all routers
         * multicast address.
         * @retval FALSE  If the IPv6 address is not a link-local all routers
         * multicast address.
         *
         */
        constexpr bool is_link_local_all_routers_multicast() const noexcept {
            auto _octets = octets();
            return _octets[0] == 0xFFu && _octets[1] == 0x02u &&
                   (_octets[2] == 0) && (_octets[3] == 0) &&
                   (_octets[4] == 0) && (_octets[5] == 0) &&
                   (_octets[6] == 0) && (_octets[7] == 0) &&
                   (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0) && (_octets[11] == 0) &&
                   (_octets[12] == 0) && (_octets[13] == 0) &&
                   (_octets[14] == 0) && (_octets[15] == 0x02u);
        }

        /**
         * This method indicates whether or not the IPv6 address is a
         * realm-local multicast address.
         *
         * @retval TRUE   If the IPv6 address is a realm-local multicast
         * address.
         * @retval FALSE  If the IPv6 address scope is not a realm-local
         * multicast address.
         *
         */
        constexpr bool is_realm_local_multicast() const noexcept {
            return is_multicast() &&
                   (scope() == static_cast<uint8_t>(scope::realm_local));
        }

        /**
         * This method indicates whether or not the IPv6 address is a
         * realm-local all nodes multicast address.
         *
         * @retval TRUE   If the IPv6 address is a realm-local all nodes
         * multicast address.
         * @retval FALSE  If the IPv6 address is not a realm-local all nodes
         * multicast address.
         *
         */
        constexpr bool is_realm_local_all_nodes_multicast() const noexcept {
            return is_multicast() &&
                   scope() == static_cast<uint8_t>(scope::realm_local);
        }

        /**
         * This method indicates whether or not the IPv6 address is a
         * realm-local all routers multicast address.
         *
         * @retval TRUE   If the IPv6 address is a realm-local all routers
         * multicast address.
         * @retval FALSE  If the IPv6 address is not a realm-local all routers
         * multicast address.
         *
         */
        constexpr bool is_realm_local_all_routers_multicast() const noexcept {
            auto _octets = octets();
            return _octets[0] == 0xFFu && _octets[1] == 0x03u &&
                   (_octets[2] == 0) && (_octets[3] == 0) &&
                   (_octets[4] == 0) && (_octets[5] == 0) &&
                   (_octets[6] == 0) && (_octets[7] == 0) &&
                   (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0) && (_octets[11] == 0) &&
                   (_octets[12] == 0) && (_octets[13] == 0) &&
                   (_octets[14] == 0) && (_octets[15] == 0x02u);
        }

        /**
         * This method indicates whether or not the IPv6 address is a
         * realm-local all MPL forwarders address.
         *
         * @retval TRUE   If the IPv6 address is a realm-local all MPL
         * forwarders address.
         * @retval FALSE  If the IPv6 address is not a realm-local all MPL
         * forwarders address.
         *
         */
        constexpr bool is_realm_local_all_mpl_forwarders() const noexcept {
            auto _octets = octets8();
            return _octets[0] == 0xFFu && _octets[1] == 0x03u &&
                   (_octets[2] == 0) && (_octets[3] == 0) &&
                   (_octets[4] == 0) && (_octets[5] == 0) &&
                   (_octets[6] == 0) && (_octets[7] == 0) &&
                   (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0) && (_octets[11] == 0) &&
                   (_octets[12] == 0) && (_octets[13] == 0) &&
                   (_octets[14] == 0) && (_octets[15] == 0xfcu);
        }

        /**
         * This method indicates whether or not the IPv6 address is multicast
         * larger than realm local.
         *
         * @retval TRUE   If the IPv6 address is multicast larger than realm
         * local.
         * @retval FALSE  If the IPv6 address is not multicast or the scope is
         * not larger than realm local.
         *
         */
        constexpr bool is_multicast_larger_than_realm_local() const noexcept {
            return is_multicast() &&
                   scope() > static_cast<uint8_t>(scope::realm_local);
        }

        /**
         * This method indicates whether or not the IPv6 address is a RLOC
         * address.
         *
         * @retval TRUE   If the IPv6 address is a RLOC address.
         * @retval FALSE  If the IPv6 address is not a RLOC address.
         *
         */
        constexpr bool is_routing_locator() const noexcept {
            constexpr auto aloc_16_mask = 0xFCu; // The mask for Aloc16
            constexpr auto rloc16_reserved_bit_mask =
                0x02u; // The mask for the reserved bit of Rloc16
            auto _octets = octets();
            // XX XX XX XX XX XX XX XX 00 00 00 FF FE 00 YY YY
            // 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
            // --0-- --1-- --2-- --3-- --4-- --5-- --6-- --7--
            return _octets[8] == 0 && _octets[9] == 0 && _octets[10] == 0 &&
                   _octets[11] == 0xFF && _octets[12] == 0xFE &&
                   _octets[13] == 0 && (_octets[14] < aloc_16_mask) &&
                   ((_octets[14] & rloc16_reserved_bit_mask) == 0);
        }

        /**
         * This method indicates whether or not the IPv6 address is an Anycast
         * RLOC address.
         *
         * @retval TRUE   If the IPv6 address is an Anycast RLOC address.
         * @retval FALSE  If the IPv6 address is not an Anycast RLOC address.
         *
         */
        constexpr bool is_anycast_routing_locator() const noexcept {
            constexpr auto aloc_16_mask = 0xFC; // The mask for Aloc16
            auto _octets = octets();

            // XX XX XX XX XX XX XX XX 00 00 00 FF FE 00 FC XX
            // 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
            // --0-- --1-- --2-- --3-- --4-- --5-- --6-- --7--

            return _octets[8] == 0 && _octets[9] == 0 && _octets[10] == 0 &&
                   _octets[11] == 0xFF && _octets[12] == 0xFE &&
                   _octets[13] == 0 && _octets[14] == aloc_16_mask;
        }

        /**
         * This method indicates whether or not the IPv6 address is an Anycast
         * Service Locator.
         *
         * @retval TRUE   If the IPv6 address is an Anycast Service Locator.
         * @retval FALSE  If the IPv6 address is not an Anycast Service Locator.
         *
         */
        constexpr bool is_anycast_service_locator() const noexcept {
            constexpr auto aloc8_service_start = 0x10;
            constexpr auto aloc8_service_end = 0x2f;
            auto _octets = octets();
            return is_anycast_routing_locator() &&
                   (_octets[IPV6_ADDR_SIZE - 2] == 0xfc) &&
                   (_octets[IPV6_ADDR_SIZE - 1] >= aloc8_service_start) &&
                   (_octets[IPV6_ADDR_SIZE - 1] <= aloc8_service_end);
        }

        /**
         * This method indicates whether or not the IPv6 address is
         * Subnet-Router Anycast (RFC 4291),
         *
         * @retval TRUE   If the IPv6 address is a Subnet-Router Anycast
         * address.
         * @retval FALSE  If the IPv6 address is not a Subnet-Router Anycast
         * address.
         *
         */
        constexpr bool is_subnet_router_anycast() const noexcept {
            // IP: XX XX XX XX XX XX XX XX 00 00 00 00 00 00 00 00
            // 08: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
            // 16: --0-- --1-- --2-- --3-- --4-- --5-- --6-- --7--
            // 32: -----0----- -----1----- -----2----- -----3-----
            // 64: -----------0----------- -----------1-----------
            auto _octets = octets();
            return (_octets[8] == 0) && (_octets[9] == 0) &&
                   (_octets[10] == 0) && (_octets[11] == 0) &&
                   (_octets[12] == 0) && (_octets[13] == 0) &&
                   (_octets[14] == 0) && (_octets[15] == 0);
        }

        /**
         * This method indicates whether or not the IPv6 address is Reserved
         * Subnet Anycast (RFC 2526),
         *
         * @retval TRUE   If the IPv6 address is a Reserved Subnet Anycast
         * address.
         * @retval FALSE  If the IPv6 address is not a Reserved Subnet Anycast
         * address.
         *
         */
        constexpr bool is_reserved_subnet_anycast() const noexcept {
            // IP: XX XX XX XX XX XX XX XX FD FF FF FF FF FF FF 80
            // 08: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
            // 16: --0-- --1-- --2-- --3-- --4-- --5-- --6-- --7--
            // 32: -----0----- -----1----- -----2----- -----3-----
            // 64: -----------0----------- -----------1-----------
            auto _octets = octets8();
            return _octets[8] == 0xFD && _octets[15] == 0x80 &&
                   (_octets[9] == 0xFFu) && (_octets[10] == 0xFFu) &&
                   (_octets[11] == 0xFFu) && (_octets[12] == 0xFFu) &&
                   (_octets[13] == 0xFFu) && (_octets[14] == 0xFFu);
        }

        /**
         * This method indicates whether or not the IPv6 address contains
         * Reserved IPv6 IID (RFC 5453),
         *
         * @retval TRUE   If the IPv6 address contains a reserved IPv6 IID.
         * @retval FALSE  If the IPv6 address does not contain a reserved IPv6
         * IID.
         *
         */
        constexpr bool is_iid_reserved() const noexcept {
            return is_subnet_router_anycast() || is_reserved_subnet_anycast() ||
                   is_anycast_routing_locator();
        }

        /**
         * This method returns a pointer to the Interface Identifier.
         * @returns A pointer to the Interface Identifier.
         */
        octets8_t::iterator iid() noexcept {
            return octets8().begin() + interface_identifier_offset;
        }

        /**
         * This method returns a pointer to the Interface Identifier.
         * @returns A pointer to the Interface Identifier.
         */
        const octets8_t::const_iterator iid() const noexcept {
            return octets8().cbegin() + interface_identifier_offset;
        }

        /**
         * This method sets the Interface Identifier.
         * @param piid A reference to the Interface Identifier.
         */
        void iid(const uint8_t* piid) noexcept {
            auto _end = piid + interface_identifier_size;
            auto _iid = iid();
            for (auto it = piid; it != _end; it++) {
                *_iid++ = *it;
            }
        }

        /**
         * This method sets the Interface Identifier.
         * @param A reference to the Interface Identifier.
         */
        void iid(const octets8_t::const_iterator& piid) noexcept {
            auto _iid = iid();
            auto _end = _iid + interface_identifier_size;
            auto pit = piid;
            for (auto it = _iid; it != _end; it++) {
                *it = *pit++;
            }
        }

        //        /**
        //         * This method sets the Interface Identifier.
        //         *
        //         * @param[in]  aExtAddress  A reference to the extended
        //         address.
        //         *
        //         */
        //        void SetIid(const Mac::ExtAddress& aExtAddress) {
        //            // TODO
        //        }
        //
        //        /**
        //         * This method converts the IPv6 Interface Identifier to an
        //         IEEE
        //         * 802.15.4 Extended Address.
        //         * @param[out]  aExtAddress  A reference to the extended
        //         address.
        //         */
        //        void ToExtAddress(Mac::ExtAddress& aExtAddress) const {
        //            // TODO
        //        }
        //
        //        /**
        //         * This method converts the IPv6 Interface Identifier to an
        //         IEEE
        //         * 802.15.4 MAC Address.
        //         * @param[out]  aMacAddress  A reference to the MAC address.
        //         */
        //        void ToExtAddress(Mac::Address& aMacAddress) const {
        //            // TODO
        //        }
        //
        //        /**
        //         * This method returns the number of IPv6 prefix bits that
        //         match.
        //         * @param[in]  aOther  The IPv6 address to match against.
        //         * @returns The number of IPv6 prefix bits that match.
        //         */
        //        uint8_t PrefixMatch(const otIp6Address& aOther) const {
        //            // TODO
        //        }

        /**
         * @brief checks if the specified ip is valid or not
         * @return true if it is an unspecified ip address.
         */
        constexpr bool is_valid() const noexcept {
            return _prefix != 254 && _prefix != 253;
        }

        /**
         * @brief long string representation of the ip
         */
        std::string str() const noexcept {
            char buffer[40] = {};
            auto _octets = octets16();
            sprintf(buffer, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                    _octets[0], _octets[1], _octets[2], _octets[3], _octets[4],
                    _octets[5], _octets[6], _octets[7]);
            buffer[39] = '\0';
            return std::string(buffer);
        }

        /**
         * @brief return the short string representation of ip version 6
         * TODO: all zero ip prints in a wrong format
         */
        std::string short_str() const noexcept {
            auto _octets = octets16();

            // finding all of the ranges that are zero filled
            decltype(_octets)::const_iterator range_start = _octets.cend(),
                                              range_end = _octets.cend(), start,
                                              finish = _octets.cbegin();
            do {
                start = std::find(finish, _octets.cend(), 0u);
                if (start == _octets.cend()) {
                    break;
                }
                finish = std::find_if(start, _octets.cend(),
                                      [](auto const& a) { return a != 0; });
                if (range_start == _octets.cend() ||
                    std::distance(start, finish) >
                        std::distance(range_start, range_end)) {
                    range_start = start;
                    range_end = finish;
                }
            } while (finish != _octets.cend());

            // generating short string representation of the ip version 6
            std::ostringstream ostr;
            ostr << std::hex;

            auto it = _octets.cbegin();

            // [0, range_start)
            while (it != range_start) {
                ostr << *it;
                if (++it != range_start)
                    ostr << ':';
            }

            // [range_start, range_end)
            if (it != range_end) {
                ostr << "::";
                it = range_end;
            }

            // [range_end, end)
            while (it != _octets.cend()) {
                ostr << *it;
                if (++it != _octets.cend())
                    ostr << ':';
            }

            return ostr.str();
        }

        /**
         * Get the prefix if exists or 255 otherwise
         */
        constexpr uint8_t prefix() const noexcept { return _prefix; }

        /**
         * Check if the ip has a prefix or not
         * @return bool an indication of weather or not the ip has a prefix or
         * not
         */
        constexpr bool has_prefix() const noexcept { return _prefix <= 128; }

        /**
         * Set prefix for this ip address
         * @param prefix
         */
        ipv6& prefix(uint8_t __prefix) noexcept {
            if (__prefix == 254u) {
                data = {}; // reset the ip if it was not valid
            }
            _prefix = __prefix > 128u && __prefix != 255u ? 253u : __prefix;
            return *this;
        }

        /**
         * Clears the prefix from this ip
         */
        ipv6& clear_prefix() noexcept { return prefix(255u); }

    }; // namespace webpp

} // namespace webpp

#endif // WEBPP_IPV6_H
