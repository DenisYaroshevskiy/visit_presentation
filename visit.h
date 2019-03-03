#include <variant>

namespace tools {

template <typename ...Fs>
struct overload : Fs... {
    using Fs::operator()...;
};

template<class... Fs> overload(Fs...) -> overload<Fs...>;

namespace simplified {

template <typename F, typename... Ts>
constexpr void visit(F f, const std::variant<Ts...>& v) {
  using signature = void (*)(F, const std::variant<Ts...>&);

  // clang-format off
  signature vtable[] {
      [](F f, const std::variant<Ts...>& v) { f(std::get<Ts>(v)); } ...
  };
  // clang-format on

  vtable[v.index()](f, v);
}

}  // simplified

}   // namespace tools