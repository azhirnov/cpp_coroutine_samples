#include "Common.h"

namespace
{
    struct Task
    {
        struct promise_type;
        using handle_t = std::coroutine_handle< promise_type >;

        struct promise_type
        {
            Task                get_return_object ()        { return Task{ handle_t::from_promise( *this )}; }
            std::suspend_always initial_suspend () noexcept { return {}; }        // delayed start
            std::suspend_always final_suspend () noexcept   { return {}; }        // must not be 'suspend_never'
            void                return_void ()              {}
            void                unhandled_exception ()      {}
            auto                await_transform (const Task &);
        };

        handle_t    _handle;

        Task (handle_t h) : _handle{h}  {}
        ~Task ()                        { _handle.destroy(); }
        
        void  Resume ()                 { _handle.resume(); }

        explicit operator bool () const { return _handle and not _handle.done(); }
    };
    
    struct Awaiter
    {
        Task const&     _task;  // depend on

        bool  await_ready () const                      { return _task._handle.done(); }
        void  await_resume ()                           {}
        void  await_suspend (std::coroutine_handle<>)   { std::cout << "await_suspend\n"; }
    };
    
    auto  Task::promise_type::await_transform (const Task &task)
    {
        return Awaiter{task};
    }

    void  Run ()
    {
        Task    t0 = [] () -> Task
        {
            std::cout << "task 1\n";
            co_return;
        }();
        
        Task    t1 = [] (Task &t0) -> Task
        {
            std::cout << "task 2 - wait\n";
            co_await t0;             // promise().await_transform( t0 )
            std::cout << "task 2 - end\n";
            co_return;
        }( t0 );

    #if 0
        if ( t0 )    t0.Resume();
        if ( t1 )    t1.Resume();
    #else
        if ( t1 )    t1.Resume();
        if ( t0 )    t0.Resume();
        if ( t1 )    t1.Resume();
    #endif
    }
}

extern void  TaskDeps_v3 ()
{
    std::cout << "\n---- 5.TaskDeps v3 ----\n";
    Run();
}
