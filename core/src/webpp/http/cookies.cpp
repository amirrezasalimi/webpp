#include "../../../include/webpp/http/cookies.h"
#include "../../../include/webpp/utils/strings.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <string_view>

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

using namespace webpp;
/*
cookie::cookie(const cookie& c) noexcept
    : attrs{c.attrs}, _name{c._name}, _value{c._value}, _domain{c._domain},
      _path{c._path},
      _expires(c._expires ? std::make_unique<date_t>(*c._expires) : nullptr),
      _max_age{c._max_age}, _same_site{c._same_site}, _secure{c._secure},
      _host_only{c._host_only},
      _encrypted{c._encrypted}, _prefix{c._prefix}, _comment{c._comment} {}

cookie::cookie(cookie&& c) noexcept
    : attrs{std::move(c.attrs)}, _name{std::move(c._name)}, _value{std::move(
                                                                c._value)},
      _domain{std::move(c._domain)}, _path{std::move(c._path)},
      _expires(std::move(c._expires)), _max_age{c._max_age},
      _same_site{c._same_site}, _secure{c._secure}, _host_only{c._host_only},
      _encrypted{c._encrypted}, _prefix{c._prefix}, _comment{std::move(
                                                        c._comment)} {}

cookie::cookie(cookie::name_t __name, cookie::value_t __value) noexcept {
    // these two will trim the strings
    name(std::move(__name));
    value(std::move(__value));
}

cookie& cookie::operator=(const cookie& c) noexcept {
    attrs = c.attrs;
    _name = c._name;
    _value = c._value;
    _domain = c._domain;
    _path = c._path;
    _expires = std::make_it_unique<date_t>(*c._expires);
    _max_age = c._max_age;
    _same_site = c._same_site;
    _secure = c._secure;
    _host_only = c._host_only;
    _encrypted = c._encrypted;
    _prefix = c._prefix;
    _comment = c._comment;
    return *this;
}

cookie& cookie::operator=(cookie&& c) noexcept {
    attrs = std::move(c.attrs);
    _name = std::move(c._name);
    _value = std::move(c._value);
    _domain = std::move(c._domain);
    _path = std::move(c._path);
    _expires = std::move(c._expires);
    _max_age = c._max_age;
    _same_site = c._same_site;
    _secure = c._secure;
    _host_only = c._host_only;
    _encrypted = c._encrypted;
    _prefix = c._prefix;
    _comment = std::move(c._comment);
    return *this;
}
*/

cookie& cookie::name(cookie::name_t __name) noexcept {
    trim(__name);
    _name = std::move(__name);
    return *this;
}

cookie& cookie::value(cookie::value_t __value) noexcept {
    trim(__value);
    _value = std::move(__value);
    return *this;
}

cookie::value_t cookie::encrypted_value() const noexcept {
    // TODO
}

std::ostream& cookie::operator<<(std::ostream& out) const noexcept {
    using namespace std::chrono;
    if (_prefix) {
        if (_secure)
            out << "__Secure-";
        else if (_host_only)
            out << "__Host-";
    }
    if (!_name.empty()) {

        // FIXME: encode/... name and value here. Programmers are dumb!
        out << _name << "=" << _value;

        if (!_comment.empty())
            out << "; Comment=" << _comment;

        if (!_domain.empty())
            out << "; Domain=" << _domain;

        if (!_path.empty())
            out << "; Path=" << _path;

        if (_expires) {
            std::time_t expires_c = system_clock::to_time_t(*_expires);
            std::tm expires_tm = *std::localtime(&expires_c);
            char buff[30];
            // FIXME: check time zone and see if it's ok
            //            setlocale(LC_ALL, "en_US.UTF-8");
            if (strftime(buff, sizeof buff, "%a, %d %b %Y %H:%M:%S GMT",
                         &expires_tm))
                out << "; Expires=" << buff;
        }

        if (_secure)
            out << "; Secure";

        if (_host_only)
            out << "; HttpOnly";

        if (_max_age)
            out << "; Max-Age=" << _max_age;

        if (_same_site != same_site_value::NONE)
            out << "; SameSite="
                << (_same_site == same_site_value::STRICT ? "Strict" : "Lax");

        // TODO: encode value and check the key here:
        if (!attrs.empty())
            for (auto const& attr : attrs)
                out << "; " << attr.first << "=" << attr.second;
    }
    return out;
}

bool cookie::operator==(const cookie& c) const noexcept {
    return _name == c._name && _value == c._value && _prefix == c._prefix &&
           _encrypted == c._encrypted && _secure == c._secure &&
           _host_only == c._host_only && _same_site == c._same_site &&
           _comment == c._comment && _expires == c._expires &&
           _path == c._path && _domain == c._domain && attrs == c.attrs;
}

bool cookie::operator<(const cookie& c) const noexcept {
    return _expires < c._expires;
}

bool cookie::operator>(const cookie& c) const noexcept {
    return _expires > c._expires;
}

bool cookie::operator<=(const cookie& c) const noexcept {
    return _expires <= c._expires;
}

bool cookie::operator>=(const cookie& c) const noexcept {
    return _expires >= c._expires;
}

std::string cookie::render() const noexcept {
    std::ostringstream os;
    this->operator<<(os);
    return os.str();
}

bool cookie::same_as(const cookie& c) const noexcept {
    return _name == c._name && _path == c._path && c._domain == _domain;
}

cookie& cookie::remove(bool __remove) noexcept {
    using namespace std::chrono;
    if (__remove) {
        // set the expire date one year before now:
        expires(system_clock::now() -
                duration<int, std::ratio<60 * 60 * 24 * 365>>(1));
    } else if (is_removed()) {
        // set the expire date one year from now:
        expires(system_clock::now() +
                duration<int, std::ratio<60 * 60 * 24 * 365>>(1));
    }
    // remove max-age if it exists because we're going with expires
    max_age(0);
    return *this;
}

cookie& cookie::expires(cookie::date_t __expires) noexcept {
    _expires = __expires;
    return *this;
}

bool cookie::is_removed() const noexcept {
    using namespace std::chrono;
    return *_expires < system_clock::now();
}

cookie& cookie::host_only(cookie::host_only_t __host_only) noexcept {
    _host_only = __host_only;
    return *this;
}

cookie& cookie::secure(cookie::secure_t __secure) noexcept {
    _secure = __secure;
    return *this;
}

cookie& cookie::same_site(cookie::same_site_t __same_site) noexcept {
    _same_site = __same_site;
    return *this;
}

cookie& cookie::prefix(cookie::prefix_t __prefix) noexcept {
    _prefix = __prefix;
    return *this;
}

cookie& cookie::max_age(cookie::max_age_t __max_age) noexcept {
    _max_age = __max_age;
    return *this;
}

cookie& cookie::path(cookie::path_t __path) noexcept {
    _path = std::move(__path);
    return *this;
}

cookie& cookie::domain(cookie::domain_t __domain) noexcept {
    _domain = std::move(__domain);
    return *this;
}

cookie& cookie::comment(cookie::comment_t __comment) noexcept {
    _comment = std::move(__comment);
    return *this;
}

cookie& cookie::encrypted(cookie::encrypted_t __encrypted) noexcept {
    _encrypted = __encrypted;
    return *this;
}

cookie_hash::result_type
cookie_hash::operator()(const cookie_hash::argument_type& c) const noexcept {
    // change the "same_as" method too if you ever touch this function
    cookie_hash::result_type seed = 0;
    hash_combine(seed, c._name);
    hash_combine(seed, c._domain);
    hash_combine(seed, c._path);
    //    hash_combine(seed, c._value);
    //    hash_combine(seed, c._prefix);
    //    hash_combine(seed, c._secure);
    //    if (c._expires)
    //        hash_combine(seed, c._expires->time_since_epoch().count());
    //    hash_combine(seed, c._max_age);
    //    hash_combine(seed, c._same_site);
    //    hash_combine(seed, c._comment);
    //    hash_combine(seed, c._host_only);
    //    hash_combine(seed, c._encrypted);
    return seed;
}

bool cookie_equals::operator()(const cookie& lhs, const cookie& rhs) const
    noexcept {
    return lhs.name() == rhs.name() && lhs.domain() == rhs.domain() &&
           lhs.path() == rhs.path();
}

cookie_jar::const_iterator cookie_jar::find(cookie::name_t const& name) const
    noexcept {
    return std::find_if(cbegin(), cend(),
                        [&](auto const& a) { return a.name() == name; });
}

cookie_jar::const_iterator cookie_jar::find(const cookie& c) const noexcept {
    return std::find_if(cbegin(), cend(),
                        [&](auto const& a) { return a.same_as(c); });
}

std::pair<cookie_jar::iterator, bool>
cookie_jar::insert(const value_type& value) {
    auto found = find(value);
    if (found != cend())
        erase(found);
    return static_cast<super*>(this)->insert(value);
}

std::pair<cookie_jar::iterator, bool> cookie_jar::insert(value_type&& value) {
    auto found = find(value);
    if (found != cend())
        erase(found);
    return static_cast<super*>(this)->insert(std::move(value));
}

cookie_jar::iterator cookie_jar::insert(const_iterator hint,
                                        const value_type& value) {
    auto found = find(value);
    if (found != cend())
        erase(found);
    return static_cast<super*>(this)->insert(hint, value);
}

cookie_jar::iterator cookie_jar::insert(const_iterator hint,
                                        value_type&& value) {
    auto found = find(value);
    if (found != cend())
        erase(found);
    return static_cast<super*>(this)->insert(hint, std::move(value));
}

void cookie_jar::insert(std::initializer_list<value_type> ilist) {
    for (const auto& it : ilist) {
        auto found = find(it);
        if (found != cend())
            erase(found);
    }
    return static_cast<super*>(this)->insert(ilist);
}

cookie_jar&
cookie_jar::encrypted(cookie::encrypted_t const& _encrypted) noexcept {
    for (auto& c : *this)
        c._encrypted = _encrypted;
    return *this;
}
cookie_jar&
cookie_jar::encrypted(condition const& _condition,
                      cookie::encrypted_t const& _encrypted) noexcept {
    change_if(_condition, [&](auto& it) { it->_encrypted = _encrypted; });
    return *this;
}
cookie_jar&
cookie_jar::encrypted(cookie::name_t const& _name,
                      cookie::encrypted_t const& _encrypted) noexcept {
    change_if(_name, [&](auto& it) { it->_encrypted = _encrypted; });
    return *this;
}
cookie_jar& cookie_jar::encrypted(const_iterator const& it,
                                  cookie::encrypted_t _encrypted) noexcept {
    it->_encrypted = _encrypted;
    return *this;
}

cookie_jar& cookie_jar::secure(cookie::secure_t const& _secure) noexcept {
    for (auto& c : *this)
        c._secure = _secure;
    return *this;
}
cookie_jar& cookie_jar::secure(condition const& _condition,
                               cookie::secure_t const& _secure) noexcept {
    change_if(_condition, [&](auto& it) { it->_secure = _secure; });
    return *this;
}
cookie_jar& cookie_jar::secure(cookie::name_t const& _name,
                               cookie::secure_t const& _secure) noexcept {
    change_if(_name, [&](auto& it) { it->_secure = _secure; });
    return *this;
}
cookie_jar& cookie_jar::secure(const_iterator const& it,
                               cookie::secure_t _secure) noexcept {
    it->_secure = _secure;
    return *this;
}

cookie_jar&
cookie_jar::host_only(cookie::host_only_t const& _host_only) noexcept {
    for (auto& c : *this)
        c._host_only = _host_only;
    return *this;
}

cookie_jar&
cookie_jar::host_only(condition const& _condition,
                      cookie::host_only_t const& _host_only) noexcept {
    change_if(_condition, [&](auto& it) { it->_host_only = _host_only; });
    return *this;
}

cookie_jar&
cookie_jar::host_only(cookie::name_t const& _name,
                      cookie::host_only_t const& _host_only) noexcept {
    change_if(_name, [&](auto& it) { it->_host_only = _host_only; });
    return *this;
}

cookie_jar& cookie_jar::host_only(const_iterator const& it,
                                  cookie::host_only_t _host_only) noexcept {
    it->_host_only = _host_only;
    return *this;
}

cookie_jar& cookie_jar::prefix(cookie::prefix_t const& _prefix) noexcept {
    for (auto& c : *this)
        c._prefix = _prefix;
    return *this;
}

cookie_jar& cookie_jar::prefix(cookie::name_t const& _name,
                               cookie::prefix_t const& _prefix) noexcept {
    change_if(_name, [&](auto& it) { it->_prefix = _prefix; });
    return *this;
}

cookie_jar& cookie_jar::prefix(condition const& _condition,
                               cookie::prefix_t const& _prefix) noexcept {
    change_if(_condition, [&](auto& it) { it->_prefix = _prefix; });
    return *this;
}

cookie_jar& cookie_jar::prefix(const_iterator const& it,
                               cookie::prefix_t _prefix) noexcept {
    it->_prefix = _prefix;
    return *this;
}

cookie_jar& cookie_jar::comment(cookie::comment_t const& _comment) noexcept {
    for (auto& c : *this)
        c._comment = _comment;
    return *this;
}
cookie_jar& cookie_jar::comment(condition const& _condition,
                                cookie::comment_t const& _comment) noexcept {
    change_if(_condition, [&](auto& it) { it->_comment = _comment; });
    return *this;
}
cookie_jar& cookie_jar::comment(cookie::name_t const& _name,
                                cookie::comment_t const& _comment) noexcept {
    change_if(_name, [&](auto& it) { it->_comment = _comment; });
    return *this;
}
cookie_jar& cookie_jar::comment(const_iterator const& it,
                                cookie::comment_t&& _comment) noexcept {
    it->_comment = std::move(_comment);
    return *this;
}
cookie_jar& cookie_jar::comment(const_iterator const& it,
                                cookie::comment_t const& _comment) noexcept {
    return comment(it, cookie::comment_t{_comment});
}

cookie_jar&
cookie_jar::same_site(cookie::same_site_t const& _same_site) noexcept {
    for (auto& c : *this)
        c._same_site = _same_site;
    return *this;
}
cookie_jar&
cookie_jar::same_site(cookie::name_t const& _name,
                      cookie::same_site_t const& _same_site) noexcept {
    change_if(_name, [&](auto& it) { it->_same_site = _same_site; });
    return *this;
}
cookie_jar&
cookie_jar::same_site(condition const& _condition,
                      cookie::same_site_t const& _same_site) noexcept {
    change_if(_condition, [&](auto& it) { it->_same_site = _same_site; });
    return *this;
}
cookie_jar& cookie_jar::same_site(const_iterator const& it,
                                  cookie::same_site_t _same_site) noexcept {
    it->_same_site = _same_site;
    return *this;
}

cookie_jar& cookie_jar::expires(cookie::date_t const& _expires) noexcept {
    for (auto& c : *this)
        c._expires = _expires;
    return *this;
}

cookie_jar& cookie_jar::expires(cookie::name_t const& _name,
                                cookie::date_t const& _expires) noexcept {
    change_if(_name, [&](auto& it) { it->_expires = _expires; });
    return *this;
}

cookie_jar& cookie_jar::expires(condition const& _condition,
                                cookie::date_t const& _expires) noexcept {
    change_if(_condition, [&](auto& it) { it->_expires = _expires; });
    return *this;
}

cookie_jar& cookie_jar::expires(const_iterator const& it,
                                cookie::date_t&& _expires) noexcept {
    it->_expires = _expires;
    return *this;
}

cookie_jar& cookie_jar::expires(const_iterator const& it,
                                cookie::date_t const& _expires) noexcept {
    return expires(it, cookie::date_t(_expires));
}

cookie_jar& cookie_jar::max_age(cookie::max_age_t const& _max_age) noexcept {
    for (auto& c : *this)
        c._max_age = _max_age;
    return *this;
}
cookie_jar& cookie_jar::max_age(cookie::name_t const& _name,
                                cookie::max_age_t const& _max_age) noexcept {
    change_if(_name, [&](auto& it) { it->_max_age = _max_age; });
    return *this;
}
cookie_jar& cookie_jar::max_age(condition const& _condition,
                                cookie::max_age_t const& _max_age) noexcept {
    change_if(_condition, [&](auto& it) { it->_max_age = _max_age; });
    return *this;
}

cookie_jar& cookie_jar::max_age(const_iterator const& it,
                                cookie::max_age_t _max_age) noexcept {
    it->_max_age = _max_age;
    return *this;
}

cookie_jar& cookie_jar::value(cookie::value_t const& _value) noexcept {
    for (auto& c : *this)
        c._value = _value;
    return *this;
}

cookie_jar& cookie_jar::value(cookie::name_t const& _name,
                              cookie::value_t const& _value) noexcept {
    change_if(_name, [&](auto& it) { it->_value = _value; });
    return *this;
}

cookie_jar& cookie_jar::value(const_iterator const& it,
                              cookie::value_t const& _value) noexcept {
    return value(it, cookie::value_t(_value));
}

cookie_jar& cookie_jar::value(const_iterator const& it,
                              cookie::value_t&& _value) noexcept {
    it->_value = std::move(_value);
    return *this;
}

cookie_jar& cookie_jar::value(condition const& _condition,
                              cookie::value_t const& _value) noexcept {
    change_if(_condition, [&](auto& it) { it->_value = _value; });
    return *this;
}

cookie_jar& cookie_jar::path(cookie::path_t const& _path) noexcept {
    change_all([&](auto& it) {
        it->_path = _path;
        make_it_unique(it, [&](auto const& c) { return c._path == _path; });
    });
    return *this;
}

cookie_jar& cookie_jar::path(cookie::name_t const& _name,
                             cookie::path_t const& _path) noexcept {
    change_if(_name, [&](auto& it) {
        it->_path = _path;
        make_it_unique(it, [&](auto const& c) { return c._path == _path; });
    });
    return *this;
}

cookie_jar& cookie_jar::path(condition const& _condition,
                             cookie::path_t const& _path) noexcept {
    change_if(_condition, [&](auto& it) {
        it->_path = _path;
        make_it_unique(it, [&](auto const& c) { return c._path == _path; });
    });
    return *this;
}

cookie_jar& cookie_jar::path(const_iterator const& it,
                             cookie::path_t&& _path) noexcept {
    it->_path = _path;
    make_it_unique(it, [&](auto const& c) { return c._path == it->_path; });
    return *this;
}

cookie_jar& cookie_jar::path(const_iterator const& it,
                             cookie::path_t const& _path) noexcept {
    return path(it, cookie::path_t(_path));
}

cookie_jar& cookie_jar::domain(cookie::domain_t const& _domain) noexcept {
    change_all([&](auto& it) {
        it->_domain = _domain;
        make_it_unique(it, [&](auto const& c) { return c._domain == _domain; });
    });
    return *this;
}

cookie_jar& cookie_jar::domain(cookie::name_t const& _name,
                               cookie::domain_t const& new_domain) noexcept {
    change_if(_name, [&](auto& it) {
        it->_domain = new_domain;
        make_it_unique(it,
                       [&](auto const& c) { return c._domain == new_domain; });
    });
    return *this;
}

cookie_jar& cookie_jar::domain(const_iterator const& it,
                               cookie::domain_t&& new_domain) noexcept {
    it->_domain = std::move(new_domain);
    make_it_unique(it, [&](auto const& c) { return c._domain == it->_domain; });
    return *this;
}

cookie_jar& cookie_jar::domain(const_iterator const& it,
                               cookie::domain_t const& new_domain) noexcept {
    return domain(it, cookie::domain_t(new_domain));
}

cookie_jar& cookie_jar::domain(condition const& _condition,
                               cookie::domain_t const& new_domain) noexcept {
    change_if(_condition, [&](auto& it) { it->_domain = new_domain; });
    return *this;
}

cookie_jar& cookie_jar::name(cookie::name_t const& __name) noexcept {
    if (auto first = begin(); first != end()) {
        first->_name = __name;
        erase(std::next(first), end()); // remove the rest
    }
    return *this;
}

cookie_jar& cookie_jar::name(cookie::name_t const& old_name,
                             cookie::name_t const& new_name) noexcept {
    erase(find(new_name));
    if (auto found = find(old_name); found != end())
        found->_name = new_name;
    return *this;
}

cookie_jar& cookie_jar::name(const_iterator const& it,
                             cookie::name_t&& new_name) noexcept {
    erase(find(new_name));
    it->_name = std::move(new_name);
    return *this;
}

cookie_jar& cookie_jar::name(const_iterator const& it,
                             cookie::name_t const& new_name) noexcept {
    return name(it, cookie::name_t(new_name));
}

cookie_jar& cookie_jar::name(condition const& _condition,
                             cookie::name_t const& new_name) noexcept {
    erase(find(new_name));
    if (auto found = std::find_if(begin(), end(), _condition); found != end()) {
        found->_name = new_name;
    }
    return *this;
}
