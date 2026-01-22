#include <concepts>

template<typename T, typename MC>
concept is_optimizer = requires(T optimizer, MC& mc) {
	{ optimizer.pass(mc) } -> std::same_as<bool>;
};
