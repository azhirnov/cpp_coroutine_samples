#include "Common.h"

namespace
{
    struct Awaiter
    {
        bool  await_ready () const                       { return false; }    // pause execution
        void  await_resume ()                            { std::cout << "Awaiter.await_resume\n"; }
        bool  await_suspend (std::coroutine_handle<>)    { return true; }    // return 'true' to pause, 'false' to resume current
    };

    [[nodiscard]] Awaiter  Wait ()
    {
        return Awaiter{};
    }
    
    struct Task
    {
        struct promise_type;
        using handle_t = std::coroutine_handle< promise_type >;

        struct promise_type
        {
            Task                get_return_object ()        { return Task{ handle_t::from_promise( *this )}; }
            std::suspend_always initial_suspend () noexcept { return {}; }
            std::suspend_always final_suspend () noexcept   { return {}; }      // must not be 'suspend_never'
            void                return_void ()              {}                  // co_return;
            void                unhandled_exception ()      {}
        };
        
        handle_t    _handle;

        Task (handle_t h) : _handle{h}  {}
        ~Task ()                        { _handle.destroy(); }

        void  Resume ()
        {
            std::cout << "task.Resume\n";
            _handle.resume();
        }

        explicit operator bool () const
        {
            // checks if the coroutine handle is valid and coroutine has not completed
            return _handle and not _handle.done();
        }
    };

    Task  CoroFn ()
    {
        // paused because of 'initial_suspend'
        // co_await promise().initial_suspend();    - implicit
        //   <-- resume

        std::cout << "CoroFn - 1\n";
        co_await Wait();
        
        //   <-- resume
        std::cout << "CoroFn - 2\n";
        co_await Wait();
        
        //   <-- resume
        std::cout << "CoroFn - 3\n";

        //co_return;    // optional
        
        // paused because of 'final_suspend'
        // co_await promise().final_suspend();    - implicit
    }

    void  Run ()
    {
        Task    t = CoroFn();
        while ( t )
        {
            t.Resume();
        }

        // '_handle.destroy()'
    }
}

extern void  SimpleAwaiter2 ()
{
    std::cout << "\n---- 2.SimpleAwaiter2 ----\n";
    Run();
}
