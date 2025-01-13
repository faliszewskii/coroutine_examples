//
// https://downloads.ctfassets.net/oxjq45e8ilak/4kVoTkxYBiVM9lPBZiG2HO/a5e36dc80fd898885269bd6320c96196/Pavel_Novikov_Uchimsya_gotovit_C_korutiny_na_praktike_2020_06_28_18_49_49.pdf
//

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <coroutine>
#include <any>
#include <variant>
#include <typeinfo>
#include <iostream>
#include <string>

// Event wrapper
template<typename... Events>
struct Event {};

// StateMachine class
struct StateMachine {
    struct promise_type;

    template<typename E>
    void onEvent(E&& e) {
        auto& promise = coro.promise();
        if (promise.isWantedEvent && promise.isWantedEvent(typeid(E))) {
            promise.currentEvent = std::forward<E>(e);
            coro.resume();
        }
    }

    ~StateMachine() { if (coro) coro.destroy(); }
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&& other) noexcept : coro(other.coro) { other.coro = nullptr; }
    StateMachine& operator=(StateMachine&& other) noexcept {
        if (this != &other) {
            if (coro) coro.destroy();
            coro = other.coro;
            other.coro = nullptr;
        }
        return *this;
    }

private:
    explicit StateMachine(std::coroutine_handle<promise_type> coro) : coro{ coro } {}
    std::coroutine_handle<promise_type> coro;
};

// Promise type
struct StateMachine::promise_type {
    using CoroHandle = std::coroutine_handle<promise_type>;

    StateMachine get_return_object() noexcept {
        return StateMachine{ CoroHandle::from_promise(*this) };
    }

    std::suspend_never initial_suspend() const noexcept { return {}; }
    std::suspend_always final_suspend() const noexcept { return {}; }

    template<typename... E>
    auto await_transform(Event<E...>) noexcept {
        isWantedEvent = [](const std::type_info& type) -> bool {
            return ((type == typeid(E)) || ...);
        };
        struct Awaitable {
            const std::any* currentEvent;
            bool await_ready() const noexcept { return false; }
            void await_suspend(CoroHandle) noexcept {}
            std::variant<E...> await_resume() const {
                std::variant<E...> event;
                (void)((currentEvent->type() == typeid(E) ? (event = std::move(*std::any_cast<E>(currentEvent)), true) : false) || ...);
                return event;
            }
        };
        return Awaitable{ &currentEvent };
    }

    void return_void() noexcept {}
    void unhandled_exception() noexcept {
        std::exit(1); // Replace with proper error handling
    }

    std::any currentEvent;
    bool (*isWantedEvent)(const std::type_info&) = nullptr;
};

// Example events
struct Open {};
struct Close {};
struct Knock {};

// Coroutine function
StateMachine getDoor(std::string answer) {
    closed:
    for (;;) {
        auto e = co_await Event<Open, Knock>{};
        if (std::holds_alternative<Knock>(e)) {
            std::cout << answer << "\n";
        } else if (std::holds_alternative<Open>(e)) {
            std::cout << "*Door opened*\n";
            goto open;
        }
    }
    open:
    co_await Event<Close>{};
    std::cout << "*Door closed*\n";
    goto closed;
}

// Main function
int state_machine() {
    auto door = getDoor("Come in, it's open!");

    std::cout << "Trying to open\n";
    door.onEvent(Open{});  // Closed -> Open
    std::cout << "Trying to knock\n";
    door.onEvent(Knock{}); // It's open.
    std::cout << "Trying to close\n";
    door.onEvent(Close{}); // Open -> Closed
    std::cout << "Trying to knock\n";
    door.onEvent(Knock{}); // Shout.
    std::cout << "Trying to close\n";
    door.onEvent(Close{}); // Closed -> Closed

    return 0;
}


#endif //STATE_MACHINE_H
