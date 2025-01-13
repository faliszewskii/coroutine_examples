#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>


auto start_async_awaitable(std::jthread& out)
{
    struct awaitable
    {
        std::jthread* p_out;
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> handler)
        {
            std::jthread& out = *p_out;
            if (out.joinable())
                throw std::runtime_error("Output jthread parameter not empty");
            out = std::jthread([handler] {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                std::cout << "Async work done on thread: " << std::this_thread::get_id() << std::endl;
                handler.resume();
            });
            // Potential undefined behavior: accessing potentially destroyed *this
            // std::cout << "New thread ID: " << p_out->get_id() << '\n';
            std::cout << "New thread ID: " << out.get_id() << '\n'; // this is OK
        }
        void await_resume() {}
    };
    return awaitable{&out};
}

struct async_in_coroutine_handler
{
    struct promise_type
    {
        async_in_coroutine_handler get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};


async_in_coroutine_handler async_in_coroutine(std::jthread& out)
{
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << std::endl;
    co_await start_async_awaitable(out); // Execution suspended and control returned to caller.
    std::cout << "Coroutine resumed on thread: " << std::this_thread::get_id() << std::endl;
}

void async_work() {
    std::jthread out;
    std::cout<< "Starting main" << std::endl;
    async_in_coroutine(out);
    std::cout<< "Returned to main on thread: " << std::this_thread::get_id() << std::endl;
    // Some work.
    std::cout<< "Main waiting for all threads" << std::endl;
    // Thread waiting...
    out.join();
    std::cout<< "Main returned";
}

// Starting main
// Coroutine started on thread: 1
// New thread ID: 2
// Returned to main on thread: 1
// Main waiting for all threads
// Async work done on thread: 2
// Coroutine resumed on thread: 2
// Main returned
