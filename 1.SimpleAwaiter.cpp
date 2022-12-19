#include "Common.h"

namespace
{
    struct Awaiter
    {
        bool  await_ready () const                      { return true; }    // 5  - used in co_await
        void  await_resume ()                           {}                  // 6  - co_await result:  void tmp = co_await Wait();
        void  await_suspend (std::coroutine_handle<>)   {}                  // unused
    };
    
    struct Task
    {
        struct promise_type
        {
            Task                get_return_object ()        { return {}; }        // 1
            std::suspend_never  initial_suspend () noexcept { return {}; }        // 2

            void                return_void ()              {}                    // 8    - co_return;
            std::suspend_never  final_suspend () noexcept   { return {}; }        // 9

            void                unhandled_exception ()      {}                    // unused
        };
    };

    Task  Run ()
    {
        co_await Awaiter{};     // 3  - ctor
                                // 4  - co_await call
        //co_return;            // 7  - optional
    }
}

extern void  SimpleAwaiter ()
{
    std::cout << "\n---- 1.SimpleAwaiter ---- \n";
    Run();
}
