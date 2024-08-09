#pragma once

#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include <memory>
#include <tuple>
#include <cassert>
#include <type_traits>
#include <utility>
#include <vector>

namespace ps {

	template<typename, template<typename...> typename T, typename... Ts> struct is_specialization : std::false_type {};

	template<template<typename...> typename T, typename... Ts>
	struct is_specialization<T<Ts...>, T> : std::true_type {};

	template<typename U, template<typename...> typename T, typename... Ts>
	constexpr auto is_specialization_v = is_specialization<U, T, Ts...>::value;

	namespace detail {
		template<typename T>
		concept CEvent = requires { typename T::args_t; } && is_specialization_v<typename T::args_t, std::tuple>;
	} // namespace detail

	/**
	 * @brief Primary template for event traits
	 * @details specialization must contain std::tuple<Args...>
	 * @tparam TEvent
	 */
	template<typename TEvent> struct event_traits {};

	template<typename TEvent>
		requires detail::CEvent<TEvent>
	struct event_traits<TEvent> {
		using args_t = typename TEvent::args_t;
	};

	/**
	 * @tparam Id -> differ two types with the same arguments set
	 * @tparam TEventArgs -> event arguments
	 */
	template<auto Id, typename... TEventArgs> struct create_event {
		using args_t = std::tuple<TEventArgs...>;
	};

	template<typename TEvent>
	concept CTrait = requires {
		typename event_traits<TEvent>;		  // template defined
		typename event_traits<TEvent>::args_t; // template has arguments
	} && is_specialization_v<typename event_traits<TEvent>::args_t, std::tuple>;

	template<CTrait TEvent> struct single_event_manager;

	template<CTrait... TEvents> struct events_manager;

	template<CTrait TEvents> struct i_single_event_subscriber;

	template<typename T> struct Dummy;

	template<CTrait TEvent> struct single_event_manager {
		using single_event_consumer_t = i_single_event_subscriber<TEvent>;
		using args_t = typename event_traits<TEvent>::args_t;

		constexpr single_event_manager() noexcept = default;

		constexpr single_event_manager(const single_event_manager&) = default;
		constexpr single_event_manager(single_event_manager&&) = default;
		constexpr single_event_manager& operator=(const single_event_manager&) = default;
		constexpr single_event_manager& operator=(single_event_manager&&) = default;

		constexpr void add_subscriber(single_event_consumer_t* subscriber) & { m_subscribers.push_back(subscriber); }

		template<typename... Args> constexpr void single_notify(Args&&... args) & {
			auto arg = std::forward_as_tuple(std::forward<Args>(args)...);
			for (auto&& sub : m_subscribers) {
				sub->single_consume(arg);
			}
		}

		constexpr ~single_event_manager() noexcept = default;

	private:
		std::vector<single_event_consumer_t*> m_subscribers;
	};

	template<CTrait... TEvents> struct events_manager : public single_event_manager<TEvents>... {
		constexpr events_manager() noexcept = default;

		constexpr events_manager(const events_manager&) = default;
		constexpr events_manager(events_manager&&) = default;
		constexpr events_manager& operator=(const events_manager&) = default;
		constexpr events_manager& operator=(events_manager&&) = default;

		constexpr ~events_manager() = default;
	};

	template<typename TSelf, CTrait TEvent> struct single_event_publisher {
		using event_manager_t = single_event_manager<TEvent>;

		constexpr explicit single_event_publisher(event_manager_t& manager) noexcept
			: m_manager(std::addressof(manager)) {}

		constexpr single_event_publisher(const single_event_publisher&) = default;
		constexpr single_event_publisher(single_event_publisher&&) noexcept = default;
		constexpr single_event_publisher& operator=(const single_event_publisher&) = default;
		constexpr single_event_publisher& operator=(single_event_publisher&&) noexcept = default;

		using traits_t = event_traits<TEvent>;
		using notify_args_t = typename traits_t::args_t;

		template<typename... Args> constexpr void single_dispatch(Args&&... args) & {
			assert(static_cast<bool>(m_manager));
			m_manager->single_notify(std::forward<Args>(args)...);
		}

	protected:
		constexpr ~single_event_publisher() noexcept = default;

	private:
		event_manager_t* m_manager{nullptr};
	};

	template<typename TSelf, CTrait... Events> struct events_publisher : public single_event_publisher<TSelf, Events>... {
		using events_manager_t = events_manager<Events...>;

		template<typename TEventManager>
			requires(std::is_base_of_v<single_event_manager<Events>, TEventManager> && ...)
		constexpr explicit events_publisher(TEventManager& manager) noexcept
			: single_event_publisher<TSelf, Events>{manager}... {}

		constexpr explicit events_publisher(events_manager_t& manager) noexcept
			: single_event_publisher<TSelf, Events>{manager}... {}

		constexpr events_publisher(const events_publisher&) = default;
		constexpr events_publisher(events_publisher&&) = default;
		constexpr events_publisher& operator=(const events_publisher&) = default;
		constexpr events_publisher& operator=(events_publisher&&) = default;

		template<CTrait TEvent, typename... Args>
			requires(std::is_same_v<TEvent, Events> || ...)
		constexpr void dispatch(Args&&... args) & {
			single_event_publisher<TSelf, TEvent>::single_dispatch(std::forward<Args>(args)...);
		}

		constexpr ~events_publisher() = default;
	};

	/**
	 * @brief Consumer or *Subscriber* interface
	 *
	 * @tparam TSelf -> Consumer implementation type
	 * @tparam TEvent -> Event which consumer must be able to handle
	 * @tparam Args -> events notification arguments
	 */
	template<typename TSelf, typename TEvent, typename... Args>
	concept CConsumer =
		CTrait<TEvent> && std::is_same_v<typename event_traits<TEvent>::args_t, std::tuple<Args...>> &&
		requires(TSelf self, Args&&... args) { self.template consume_event<TEvent>(std::forward<Args>(args)...); };

	template<CTrait TEvent> struct i_single_event_subscriber {
		using single_event_manager_t = single_event_manager<TEvent>;
		using traits_t = event_traits<TEvent>;
		using notify_args_t = typename traits_t::args_t;

		constexpr explicit i_single_event_subscriber(single_event_manager_t& manager) noexcept {
			manager.add_subscriber(this);
		}

		constexpr i_single_event_subscriber(const i_single_event_subscriber&) = default;
		constexpr i_single_event_subscriber(i_single_event_subscriber&&) = default;
		constexpr i_single_event_subscriber& operator=(const i_single_event_subscriber&) = default;
		constexpr i_single_event_subscriber& operator=(i_single_event_subscriber&&) = default;

		virtual void single_consume(notify_args_t args) & = 0;

		constexpr virtual ~i_single_event_subscriber() noexcept = default;
	};

	template<typename TSelf, CTrait TEvent> struct single_event_subscriber : public i_single_event_subscriber<TEvent> {
		using i_single_event_subscriber<TEvent>::i_single_event_subscriber;

		constexpr single_event_subscriber(const single_event_subscriber&) = default;
		constexpr single_event_subscriber(single_event_subscriber&&) = default;
		constexpr single_event_subscriber& operator=(const single_event_subscriber&) = default;
		constexpr single_event_subscriber& operator=(single_event_subscriber&&) = default;

		using typename i_single_event_subscriber<TEvent>::traits_t;
		using typename i_single_event_subscriber<TEvent>::notify_args_t;

		void single_consume(notify_args_t args) & override {
			constexpr auto tuple_size = std::tuple_size_v<notify_args_t>;
			constexpr auto integer_sequence = std::make_index_sequence<tuple_size>{};
			single_consume_helper(integer_sequence, std::move(args));
		}

		constexpr ~single_event_subscriber() override = default;

	private:
		static constexpr auto tuple_size = std::tuple_size_v<notify_args_t>;

		template<std::size_t... Idx>
		constexpr void single_consume_helper(std::index_sequence<Idx...>, notify_args_t args) & {
			single_consume_impl(std::get<Idx>(std::forward<notify_args_t>(args))...);
		}

		template<typename... Args>
			requires CConsumer<TSelf, TEvent, Args...>
		constexpr void single_consume_impl(Args&&... args) & {
			static_cast<TSelf&>(*this).template consume_event<TEvent>(std::forward<Args>(args)...);
		}
	};

	template<typename TSelf, CTrait... TEvents>
	struct [[nodiscard("dangling pointer on subscription if discarded")]] events_subscriber
		: public single_event_subscriber<TSelf, TEvents>... {
		template<typename TEventManager>
			requires(std::is_base_of_v<single_event_manager<TEvents>, TEventManager> && ...)
		explicit constexpr events_subscriber(TEventManager& manager) noexcept
			: single_event_subscriber<TSelf, TEvents>{manager}... {}

		explicit constexpr events_subscriber(events_manager<TEvents...>& manager) noexcept
			: single_event_subscriber<TSelf, TEvents>{manager}... {}

		constexpr events_subscriber(const events_subscriber&) = default;
		constexpr events_subscriber(events_subscriber&&) = default;
		constexpr events_subscriber& operator=(const events_subscriber&) = default;
		constexpr events_subscriber& operator=(events_subscriber&&) = default;

		template<CTrait TEvent, typename... Args>
			requires(std::is_same_v<TEvent, TEvents> || ...) && CConsumer<TSelf, TEvent, Args...>
		constexpr void consume(Args&&... args) & {
			single_event_subscriber<TSelf, TEvent>::single_consume(std::forward_as_tuple(std::forward<Args>(args)...));
		}

		constexpr ~events_subscriber() override = default;
	};
} // namespace ps

#endif // EVENTDISPATCHER_HPP