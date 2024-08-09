#include "pubsubpp/pubsubpp.hpp"

#include <catch2/catch_test_macros.hpp>

namespace ps {
	/**
	 * @brief struct helper to convert enum value to tagged struct
	 *
	 * @tparam StateID -> enum value
	 */
	template<auto StateID> struct state : public std::integral_constant<std::decay_t<decltype(StateID)>, StateID> {};

	enum struct events {
		A,
		B,
		C,
	};

	using a_event = state<events::A>;
	using b_event = state<events::B>;

	struct c_event {
		using args_t = std::tuple<>;
	};
	
	template<> struct event_traits<a_event> {
		using args_t = std::tuple<int, std::string>;
	};

	template<> struct event_traits<b_event> {
		using args_t = std::tuple<std::string, std::string>;
	};

	struct ab_events_dispatcher : public events_publisher<ab_events_dispatcher, a_event, b_event> {
		int a_dispatch_counter{0};
		int b_dispatch_counter{0};

		using events_publisher<ab_events_dispatcher, a_event, b_event>::events_publisher;
	};

	struct ab_events_consumer : public events_subscriber<ab_events_consumer, a_event, b_event> { // NOLINT
		int a_consume_counter{0};
		int b_consume_counter{0};
		int c_misses{0};

		using events_subscriber<ab_events_consumer, a_event, b_event>::events_subscriber;

		constexpr ab_events_consumer(const ab_events_consumer&) = default;
		constexpr ab_events_consumer(ab_events_consumer&&) = default;
		constexpr ab_events_consumer& operator=(const ab_events_consumer&) = default;
		constexpr ab_events_consumer& operator=(ab_events_consumer&&) = default;

		constexpr ~ab_events_consumer() override = default;

		template<typename TEvent, typename... Args>
			requires(std::is_same_v<TEvent, a_event> || std::is_same_v<TEvent, b_event>)
		void consume_event(Args...) & {
			c_misses++;
		}
	};

	template<> void ab_events_consumer::consume_event<a_event>([[maybe_unused]] int a, std::string b) & {
		a_consume_counter++;
		b = "abc";
	}

	template<>
	void ab_events_consumer::consume_event<b_event>([[maybe_unused]] std::string a, [[maybe_unused]] std::string b) & {
		b_consume_counter++;
	}

	struct a_events_consumer : public events_subscriber<a_events_consumer, a_event> {
		constexpr a_events_consumer(const a_events_consumer&) = default;
		constexpr a_events_consumer(a_events_consumer&&) = default;
		constexpr a_events_consumer& operator=(const a_events_consumer&) = default;
		constexpr a_events_consumer& operator=(a_events_consumer&&) = default;

		constexpr ~a_events_consumer() override = default;

		int a_consume_counter{0};
		int c_misses{0};

		using events_subscriber<a_events_consumer, a_event>::events_subscriber;

		template<typename TEvent, typename... Args>
			requires(std::is_same_v<TEvent, a_event>)
		constexpr void consume_event(Args...) & {
			c_misses++;
		}
	};

	template<> constexpr void a_events_consumer::consume_event<a_event>(int a, std::string b) & {
		a_consume_counter++;
		b = "abc";
	}

	struct a_events_dispatcher : public events_publisher<a_events_dispatcher, a_event> {
		int a_dispatch_counter{0};

		using events_publisher<a_events_dispatcher, a_event>::events_publisher;
	};

	struct c_event_dispatcher : public events_publisher<c_event_dispatcher, c_event> {};

	using ab_events_manager = events_manager<a_event, b_event>;

	TEST_CASE("custom event system") {
		using namespace std::string_literals;
		ab_events_manager manager{};

		a_events_consumer a_consumer{manager};
		ab_events_consumer consumer(manager);
		ab_events_consumer consumer2(manager);

		ab_events_dispatcher dispatcher{manager};
		a_events_dispatcher dispatcher2{manager};

		std::string a;
		int b{5};

		dispatcher2.dispatch<a_event>(b, a);

		dispatcher.dispatch<a_event>(b, a);

		CHECK(consumer.a_consume_counter == 2);
		CHECK(consumer.b_consume_counter == 0);
		CHECK(consumer.c_misses == 0);
	}
} // namespace ps