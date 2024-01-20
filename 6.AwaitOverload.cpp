#include "Common.h"

namespace
{
    struct TaskA
    {
        struct promise_type;
        using handle_t = std::coroutine_handle< promise_type >;

        struct promise_type
        {
            int        _data;

            TaskA               get_return_object ()        { return TaskA{ handle_t::from_promise( *this )}; }
            std::suspend_always initial_suspend () noexcept { return {}; }        // delayed start
            std::suspend_always final_suspend () noexcept   { return {}; }        // must not be 'suspend_never'
            void                return_void ()              {}
            void                unhandled_exception ()      {}
        };

        handle_t    _handle;
        
        TaskA (handle_t h) : _handle{h} {}
        ~TaskA ()                       { _handle.destroy(); }

        void  Resume ()                 { _handle.resume(); }

        explicit operator bool () const { return _handle and not _handle.done(); }
    };
    
    struct TaskB
    {
        struct promise_type;
        using handle_t = std::coroutine_handle< promise_type >;

        struct promise_type
        {
            std::string        _data;

            TaskB               get_return_object ()        { return TaskB{ handle_t::from_promise( *this )}; }
            std::suspend_always initial_suspend () noexcept { return {}; }        // delayed start
            std::suspend_always final_suspend () noexcept   { return {}; }        // must not be 'suspend_never'
            void                return_void ()              {}
            void                unhandled_exception ()      {}
        };

        handle_t    _handle;
        
        TaskB (handle_t h) : _handle{h} {}
        ~TaskB ()                       { _handle.destroy(); }

        void  Resume ()                 { _handle.resume(); }

        explicit operator bool () const { return _handle and not _handle.done(); }
    };

    struct Awaiter
    {
        bool  await_ready () const  { return false; }
        void  await_resume ()       {}

        void  await_suspend (std::coroutine_handle< TaskA::promise_type > h)
        {
            std::cout << "await_suspend - A\n";
            h.promise()._data = 0;
        }
        
        void  await_suspend (std::coroutine_handle< TaskB::promise_type > h)
        {
            std::cout << "await_suspend - B\n";
            h.promise()._data = "123";
        }
    };
    
    Awaiter  operator co_await (const TaskA &)  { return {}; }

    void  Run ()
    {
        TaskA    t0 = [] () -> TaskA
        {
            std::cout << "task A\n";
            co_return;
        }();
        
        TaskB    t1 = [] (TaskA &t0) -> TaskB
        {
            co_await t0;
            std::cout << "task B\n";
            co_return;
        }( t0 );
        
        if ( t0 )    t0.Resume();
        if ( t1 )    t1.Resume();
        if ( t0 )    t0.Resume();
        if ( t1 )    t1.Resume();
    }
}

extern void  AwaitOverload ()
{
    std::cout << "\n---- 6.AwaitOverload ----\n";
    Run();
}
