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
	template<typename TEvent> struct EventTraits {};

	template<typename TEvent>
		requires detail::CEvent<TEvent>
	struct EventTraits<TEvent> {
		using args_t = typename TEvent::args_t;
	};

	/**
	 * @tparam Id -> differ two types with the same arguments set
	 * @tparam TEventArgs -> event arguments
	 */
	template<auto Id, typename... TEventArgs> struct CreateEvent {
		using args_t = std::tuple<TEventArgs...>;
	};

	template<typename TEvent>
	concept CTrait = requires {
		typename EventTraits<TEvent>;		  // template defined
		typename EventTraits<TEvent>::args_t; // template has arguments
	} && is_specialization_v<typename EventTraits<TEvent>::args_t, std::tuple>;

	template<CTrait TEvent> struct SingleEventManager;

	template<CTrait... TEvents> struct EventsManager;

	template<CTrait TEvents> struct ISingleEventSubscriber;

	template<typename T> struct Dummy;

	template<CTrait TEvent> struct SingleEventManager {
		using single_event_consumer_t = ISingleEventSubscriber<TEvent>;
		using args_t = typename EventTraits<TEvent>::args_t;

		constexpr SingleEventManager() noexcept = default;

		constexpr SingleEventManager(const SingleEventManager&) = default;
		constexpr SingleEventManager(SingleEventManager&&) = default;
		constexpr SingleEventManager& operator=(const SingleEventManager&) = default;
		constexpr SingleEventManager& operator=(SingleEventManager&&) = default;

		constexpr void add_subscriber(single_event_consumer_t* subscriber) & { m_subscribers.push_back(subscriber); }

		template<typename... Args> constexpr void single_notify(Args&&... args) & {
			auto arg = std::forward_as_tuple(std::forward<Args>(args)...);
			for (auto&& sub : m_subscribers) {
				sub->single_consume(arg);
			}
		}

		constexpr ~SingleEventManager() noexcept = default;

	private:
		std::vector<single_event_consumer_t*> m_subscribers;
	};

	template<CTrait... TEvents> struct EventsManager : public SingleEventManager<TEvents>... {
		constexpr EventsManager() noexcept = default;

		constexpr EventsManager(const EventsManager&) = default;
		constexpr EventsManager(EventsManager&&) = default;
		constexpr EventsManager& operator=(const EventsManager&) = default;
		constexpr EventsManager& operator=(EventsManager&&) = default;

		constexpr ~EventsManager() = default;
	};

	template<typename TSelf, CTrait TEvent> struct SingleEventPublisher {
		using event_manager_t = SingleEventManager<TEvent>;

		constexpr explicit SingleEventPublisher(event_manager_t& manager) noexcept
			: m_manager(std::addressof(manager)) {}

		constexpr SingleEventPublisher(const SingleEventPublisher&) = default;
		constexpr SingleEventPublisher(SingleEventPublisher&&) noexcept = default;
		constexpr SingleEventPublisher& operator=(const SingleEventPublisher&) = default;
		constexpr SingleEventPublisher& operator=(SingleEventPublisher&&) noexcept = default;

		using traits_t = EventTraits<TEvent>;
		using notify_args_t = typename traits_t::args_t;

		template<typename... Args> constexpr void single_dispatch(Args&&... args) & {
			assert(static_cast<bool>(m_manager));
			m_manager->single_notify(std::forward<Args>(args)...);
		}

	protected:
		constexpr ~SingleEventPublisher() noexcept = default;

	private:
		event_manager_t* m_manager{nullptr};
	};

	template<typename TSelf, CTrait... Events> struct EventsPublisher : public SingleEventPublisher<TSelf, Events>... {
		using events_manager_t = EventsManager<Events...>;

		template<typename TEventManager>
			requires(std::is_base_of_v<SingleEventManager<Events>, TEventManager> && ...)
		constexpr explicit EventsPublisher(TEventManager& manager) noexcept
			: SingleEventPublisher<TSelf, Events>{manager}... {}

		constexpr explicit EventsPublisher(events_manager_t& manager) noexcept
			: SingleEventPublisher<TSelf, Events>{manager}... {}

		constexpr EventsPublisher(const EventsPublisher&) = default;
		constexpr EventsPublisher(EventsPublisher&&) = default;
		constexpr EventsPublisher& operator=(const EventsPublisher&) = default;
		constexpr EventsPublisher& operator=(EventsPublisher&&) = default;

		template<CTrait TEvent, typename... Args>
			requires(std::is_same_v<TEvent, Events> || ...)
		constexpr void dispatch(Args&&... args) & {
			SingleEventPublisher<TSelf, TEvent>::single_dispatch(std::forward<Args>(args)...);
		}

		constexpr ~EventsPublisher() = default;
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
		CTrait<TEvent> && std::is_same_v<typename EventTraits<TEvent>::args_t, std::tuple<Args...>> &&
		requires(TSelf self, Args&&... args) { self.template consume_event<TEvent>(std::forward<Args>(args)...); };

	template<CTrait TEvent> struct ISingleEventSubscriber {
		using single_event_manager_t = SingleEventManager<TEvent>;
		using traits_t = EventTraits<TEvent>;
		using notify_args_t = typename traits_t::args_t;

		constexpr ISingleEventSubscriber(single_event_manager_t& manager) noexcept { manager.add_subscriber(this); }

		constexpr ISingleEventSubscriber(const ISingleEventSubscriber&) = default;
		constexpr ISingleEventSubscriber(ISingleEventSubscriber&&) = default;
		constexpr ISingleEventSubscriber& operator=(const ISingleEventSubscriber&) = default;
		constexpr ISingleEventSubscriber& operator=(ISingleEventSubscriber&&) = default;

		constexpr virtual void single_consume(notify_args_t args) & = 0;

		constexpr virtual ~ISingleEventSubscriber() noexcept = default;
	};

	template<typename TSelf, CTrait TEvent> struct SingleEventSubscriber : public ISingleEventSubscriber<TEvent> {
		using ISingleEventSubscriber<TEvent>::ISingleEventSubscriber;

		constexpr SingleEventSubscriber(const SingleEventSubscriber&) = default;
		constexpr SingleEventSubscriber(SingleEventSubscriber&&) = default;
		constexpr SingleEventSubscriber& operator=(const SingleEventSubscriber&) = default;
		constexpr SingleEventSubscriber& operator=(SingleEventSubscriber&&) = default;

		using typename ISingleEventSubscriber<TEvent>::traits_t;
		using typename ISingleEventSubscriber<TEvent>::notify_args_t;

		constexpr void single_consume(notify_args_t args) & override {
			constexpr auto tuple_size = std::tuple_size_v<notify_args_t>;
			constexpr auto integer_sequence = std::make_index_sequence<tuple_size>{};
			single_consume_helper(integer_sequence, std::move(args));
		}

		constexpr ~SingleEventSubscriber() override = default;

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
	struct [[nodiscard("dangling pointer on subscription if discarded")]] EventsSubscriber
		: public SingleEventSubscriber<TSelf, TEvents>... {
		template<typename TEventManager>
			requires(std::is_base_of_v<SingleEventManager<TEvents>, TEventManager> && ...)
		explicit constexpr EventsSubscriber(TEventManager& manager) noexcept
			: SingleEventSubscriber<TSelf, TEvents>{manager}... {}

		explicit constexpr EventsSubscriber(EventsManager<TEvents...>& manager) noexcept
			: SingleEventSubscriber<TSelf, TEvents>{manager}... {}

		constexpr EventsSubscriber(const EventsSubscriber&) = default;
		constexpr EventsSubscriber(EventsSubscriber&&) = default;
		constexpr EventsSubscriber& operator=(const EventsSubscriber&) = default;
		constexpr EventsSubscriber& operator=(EventsSubscriber&&) = default;

		template<CTrait TEvent, typename... Args>
			requires(std::is_same_v<TEvent, TEvents> || ...) && CConsumer<TSelf, TEvent, Args...>
		constexpr void consume(Args&&... args) & {
			SingleEventSubscriber<TSelf, TEvent>::single_consume(std::forward_as_tuple(std::forward<Args>(args)...));
		}

		constexpr ~EventsSubscriber() override = default;
	};
} // namespace ps

#endif // EVENTDISPATCHER_HPP