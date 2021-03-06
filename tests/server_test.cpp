#include <gtest/gtest.h>
#include <string>
#include "../core/include/webpp/interfaces/basic_interface.h"
#include "../core/include/webpp/server.h"
#include "../core/include/webpp/valves/uri.h"

using namespace webpp;
using namespace webpp::valves;

namespace webpp {

    class fake_interface;

    template <>
    class request_t<fake_interface> : public basic_request_t {
      public:
        std::string_view request_uri() const noexcept { return __path; }

        void set_path(std::string_view ___path) noexcept { __path = ___path; }

      protected:
        std::string __path;
    };

    class fake_interface : public basic_interface<fake_interface> {
      public:
        webpp::router<fake_interface> router;
        std::string body_result;
        std::string header_result;
        request_t<fake_interface> req;

        fake_interface() noexcept = default;

        void operator()() noexcept {
            auto res = this->router(req);
            body_result = res.body.str();
            header_result = res.header.str();
        }
    };

} // namespace webpp

TEST(Server, Init) {
    server<fake_interface> app;
    app.req.set_path("/");
    app.router.on("/"_path, [] { return "hello world"; });

    app.run();
    EXPECT_EQ(app.body_result, "hello world");

    app.router.on("/home"_path, [](request_t<fake_interface> const& req,
                                   response& res) { res << "Coding"; });
    app.req.set_path("/home");
    app.run();
    EXPECT_EQ(app.body_result, "Coding");

    app.router.on("/about"_path,
                  [](request_t<fake_interface> const& req, response& res) {
                      res << "Something";
                      // stop reformating this into one line clang-format!
                  });
    app.req.set_path("/about/");
    app.run();
    EXPECT_EQ(app.body_result, "Something");
}
