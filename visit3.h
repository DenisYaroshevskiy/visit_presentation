#pragma once

#include <array>
#include <type_traits>
#include <utility>

namespace v3 {

#define FWD(x) std::forward<decltype(x)>(x)

template <size_t... idxs> using idxs_ = std::index_sequence<idxs...>;
template <size_t s> using make_idxs_ = std::make_index_sequence<s>;

struct error {};

template <size_t... dims> struct index_conversions {
  static constexpr auto pows = [] {
    std::array dims_a = {dims...};
    std::array res = dims_a;
    for (size_t running = 1, i = dims_a.size(); i > 0; --i) {
      res[i - 1] = running;
      running *= dims_a[i - 1];
    }
    return res;
  }();

  static constexpr size_t linear(decltype(pows) multi) {
    size_t res = 0;
    for (size_t i = 0; i < pows.size(); ++i) res += multi[i] * pows[i];
    return res;
  }

  template <size_t idx, size_t... for0_n>
  static constexpr auto _multi(idxs_<for0_n...>) {
    constexpr auto as_array = [] {
      auto res = pows;
      for (size_t _idx = idx, i = 0; i < pows.size(); ++i) {
        res[i] = _idx / pows[i];
        _idx %= pows[i];
      }
      return res;
    }();
    return idxs_<as_array[for0_n]...>{};
  }

  template <size_t idx> static constexpr auto multi() {
    return _multi<idx>(make_idxs_<pows.size()>{});
  };
};

template <class R, class Fop, class... Fvs> struct _visit_r_impl {
  using vtable_entry = R (*)(Fop, Fvs...);

  template <size_t... idxs> struct vtable_generator {
    constexpr vtable_entry operator()(idxs_<idxs...>) {
      return [](Fop op, Fvs... vs) -> R { return Fwd(op)(Fwd(vs)...); };
    }
  };
};

template <class T> struct type_ { using type = T; };

template <class... Ts> struct types {};

template <class... Ts> auto make_types() {
  if constexpr ((std::is_same_v<error, Ts> || ...))
    return error{};
  else
    return types<Ts...>{};
}

template <class... Ts> type_<std::common_type_t<Ts...>> _common(types<Ts...>);
error _common(...);

template <typename...> using visit_rt = void;

template <class F, class... Vs> constexpr auto _visit(F&& f, Vs&&... vs) {
  return 0;
}

template <class F, class... Vs>
constexpr auto visit(F&& f, Vs&&... vs) -> std::enable_if_t<
    !std::is_same_v<error, decltype(_visit(FWD(f), FWD(vs)...))>,
    decltype(_visit(FWD(f), FWD(vs)...))> {
  return _visit(FWD(f), FWD(vs)...);
}

#undef FWD

} // namespace v3
