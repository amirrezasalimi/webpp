#include <array>
#include <gtest/gtest.h>
#include <string_view>
#include "../core/include/webpp/utils/ipv4.h"
#include "../core/include/webpp/validators/validators.h"

using namespace webpp;

TEST(IPv4Tests, FreeFunctions) {
    EXPECT_EQ(to_prefix("255.255.255.0"), 24);
    EXPECT_EQ(to_prefix("255.255.255.128"), 25);
    EXPECT_EQ(to_prefix("0.255.255.128"), 0);
    EXPECT_EQ(to_prefix({255, 255, 255, 128}), 25);
    EXPECT_EQ(to_prefix({0, 255, 255, 128}), 0);
    EXPECT_EQ(to_prefix(0xFF'FF'FF'00u), 24);
    EXPECT_EQ(to_prefix(0xFF'00'FF'00u), 8);
    EXPECT_EQ(to_subnet(24), 0xFF'FF'FF'00);
    EXPECT_EQ(to_subnet(8), 0xFF'00'00'00);

    auto a = std::array<uint8_t, 4>({255, 0, 0, 0});
    auto b = std::array<uint8_t, 4>({255, 255, 255, 128});
    EXPECT_EQ(to_subnet_array(8), a);
    EXPECT_EQ(to_subnet_array(25), b);
}

TEST(IPv4Tests, Creation) {
    constexpr ipv4 one{192, 168, 2, 1};
    ipv4 two("192.168.2.1");
    EXPECT_EQ(one, two) << "one is: " << one.str() << "; two is: " << two.str();
    EXPECT_EQ(one.integer(), two.integer());
    EXPECT_EQ(one.str(), two.str());
    EXPECT_FALSE(one.is_zero());
}

TEST(IPv4Tests, Methods) {
    ipv4 ip{192, 168, 1, 1};
    EXPECT_TRUE(ip.is_in_subnet({{192, 168, 0, 0}, 16}));
    EXPECT_TRUE(ip.is_in_subnet({{192, 168, 1, 0}, 24}));
    EXPECT_TRUE(ip.in_range({192, 168, 0, 1}, ipv4("192.168.2.1")));
    EXPECT_FALSE(ip.in_range({192, 168, 1, 2}, ipv4("192.168.2.1")));
    EXPECT_TRUE(ip.is_private());
    EXPECT_FALSE(ip.is_public());
    EXPECT_FALSE(ip.is_zero());
    EXPECT_EQ(ip.str(), "192.168.1.1");

    auto octets = ip.octets();
    EXPECT_EQ(octets[0], 192);
    EXPECT_EQ(octets[1], 168);
    EXPECT_EQ(octets[2], 1);
    EXPECT_EQ(octets[3], 1);
}

TEST(IPv4Tests, Validation) {
    auto valid_ipv4s = {"0.0.0.0", "192.168.1.1", "255.255.255.255"};

    auto invalid_ipv4s = {
        "10.168.0001.100", "0.0.0.256",      "256.255.255.255",
        "256.0.0.0",       "192.168. 224.0", "192.168.224.0 1",
    };

    for (auto const& _ip : valid_ipv4s) {
        EXPECT_TRUE(webpp::is::ipv4(_ip));
        EXPECT_TRUE(ipv4(_ip).is_valid())
            << "ip: " << _ip << "; compiled ip: " << ipv4(_ip);
        ipv4 ip{_ip};
        (void)ip.integer(); // just to make sure it's parsed
        EXPECT_TRUE(ip.is_valid());
    }

    for (auto const& _ip : invalid_ipv4s) {
        EXPECT_FALSE(webpp::is::ipv4(_ip));
        EXPECT_FALSE(ipv4(_ip).is_valid())
            << "ip: " << _ip << "; compiled ip: " << ipv4(_ip).str();
        ipv4 ip{_ip};
        (void)ip.integer(); // just to make sure it's parsed
        EXPECT_FALSE(ip.is_valid());
    }
}

TEST(IPv4Tests, CIDR) {
    auto valid_ipv4s = {"192.168.1.1/12", "192.168.1.1/32"};
    auto invalid_ipv4s = {"192.168.1.1/", "192.168.1.1/12.34", "192.168.1.1/01",
                          "192.168.1.1/33"};

    for (auto const& _ip : valid_ipv4s) {
        EXPECT_TRUE(webpp::is::ipv4_prefix(_ip)) << _ip;
        EXPECT_TRUE(ipv4(_ip).is_valid())
            << "ip: " << _ip << "; compiled ip: " << ipv4(_ip).str();
        EXPECT_TRUE(ipv4(_ip).has_prefix()) << _ip;
        EXPECT_GE(ipv4(_ip).prefix(), 0) << _ip;
        EXPECT_LE(ipv4(_ip).prefix(), 32) << _ip;
    }

    for (auto const& _ip : invalid_ipv4s) {
        EXPECT_FALSE(webpp::is::ipv6(_ip)) << _ip;
        EXPECT_FALSE(ipv4(_ip).is_valid()) << _ip;
        EXPECT_FALSE(ipv4(_ip).has_prefix()) << _ip;
        // TODO: check cidr(prefix) method
    }
}
