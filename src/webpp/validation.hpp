#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>

namespace webpp {

namespace is {

/**
 * @brief check if the string is empty or not
 * @return true if the string is empty
 */
bool empty(std::string const& str) noexcept {
  return str.empty();
}

/**
 * @brief check if these two are equal
 */
template <typename T1, typename T2>
bool equals(T1 const& first, T2 const& second) noexcept {
  return first == second;
}

/**
 * @brief check if str contains seed
 * @param str
 * @param seed
 * @return true if it does contain it
 */
bool contains(std::string const& str, std::string const& seed) noexcept {
  return str.find(seed) == std::string::npos;
}

/**
 * @brief check if the container contains value
 */
template <typename T>
bool contains(std::initializer_list<T> const& container,
              T const& value) noexcept {
  return std::find(std::begin(container), std::end(container), value) !=
         std::end(container);
}

/**
 * @brief check if the container contains key
 */
template <template <class, class...> class Container, class T1, class... Args>
bool contains_key(Container<T1, Args...> const& container,
                  T1 const& key) noexcept {
  return container.find(key) != std::end(container);
}

/**
 * @brief check if the container contains the value
 */
template <template <class, class, class...> class Container,
          class T1,
          class T2,
          class... Args>
bool contains_value(Container<T1, T2, Args...> const& container,
                    T2 const& value) noexcept {
  for (auto pair : container)
    if (pair.second == value)
      return true;
  return false;
}

bool trimmed(std::string const& str) noexcept;
bool rtrimmed(std::string const& str) noexcept;
bool ltrimmed(std::string const& str) noexcept;

bool FQDN(std::string const& str) noexcept;

bool number(std::string const& str) noexcept;
bool integer(std::string const& str) noexcept;
bool email(std::string const& str) noexcept;
bool phone(std::string const& str) noexcept;
bool url(std::string const& str) noexcept;
bool ipv4(std::string const& str) noexcept;
bool ipv6(std::string const& str) noexcept;
bool ip(std::string const& str) noexcept;
bool ip_range(std::string const& str) noexcept;
bool ipv4_range(std::string const& str) noexcept;
bool ipv6_range(std::string const& str) noexcept;
// bool isImage(something) noexcept;
bool color(std::string const& str) noexcept;
bool hex_color(std::string const& str) noexcept;
bool name_color(std::string const& str) noexcept;
bool mimetype(std::string const& str) noexcept;
bool UUID(std::string const& str) noexcept;
bool port(std::string const& str) noexcept;
bool mongoid(std::string const& str) noexcept;
bool lowercase(std::string const& str) noexcept;
bool uppercase(std::string const& str) noexcept;

// you may want to change the string to a date of some sort or add both
bool today(std::string const& str) noexcept;
bool tomorrow(std::string const& str) noexcept;
bool yesterday(std::string const& str) noexcept;
bool this_year(std::string const& str) noexcept;
bool next_year(std::string const& str) noexcept;
bool prev_year(std::string const& str) noexcept;
bool this_month(std::string const& str) noexcept;
bool next_month(std::string const& str) noexcept;
bool prev_month(std::string const& str) noexcept;
bool this_week(std::string const& str) noexcept;
bool next_week(std::string const& str) noexcept;
bool prev_week(std::string const& str) noexcept;
bool between(std::string const& str,
             std::string const& from,
             std::string const& after) noexcept;
bool after(std::string const& str, std::string const& pointintime) noexcept;
bool before(std::string const& str, std::string const& pointintime) noexcept;

bool base64(std::string const& str) noexcept;
bool escaped(std::string const& str) noexcept;

bool username(std::string const& str) noexcept;
}  // namespace is
}  // namespace webpp

#endif  // VALIDATION_H