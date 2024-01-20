#include "Common.h"

namespace
{
    static std::vector< std::coroutine_handle<> >*    taskListRef;

    struct [[nodiscard]] Size
    {
        size_t  value;
    };

    struct Awaiter
    {
        bool    await_ready () const                        { return false; }                               // pause execution
        Size    await_resume ()                             { return Size{taskListRef->size()}; }           // 'co_await' can return value
        bool    await_suspend (std::coroutine_handle<> h)   { taskListRef->push_back( h ); return true; }   // add to task list
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
            std::suspend_never  initial_suspend () noexcept { return {}; }        // must start immediatly
            std::suspend_always final_suspend () noexcept   { return {}; }        // must not be 'suspend_never'
            void                return_void ()              {}
            void                unhandled_exception ()      {}
        };
        
        handle_t     _handle;

        Task (handle_t h) : _handle{h}  {}
        ~Task ()                        { assert( _handle.done() );  _handle.destroy(); }
    };

    Task  CoroFn ()
    {
        #if 0
            co_await Wait();  // - no errors
        #else
            Size  n = co_await Wait();
            (void)( n );
        #endif

        //   <-- resume
        co_await Wait();    // ignoring return value
        
        //   <-- resume
        std::cout << "CoroFn - end\n";
    }

    void  Run ()
    {
        std::vector< std::coroutine_handle<> >    task_list;    // to avoid false positive memleak warning 
        taskListRef  = &task_list;


        Task    t0 = CoroFn();
        Task    t1 = CoroFn();

        for (; not task_list.empty();)
        {
            for (size_t i = 0; i < task_list.size();)
            {
                if ( task_list[i].done() )
                    task_list.erase( task_list.begin() + i );
                else
                {
                    task_list[i].resume();
                    ++i;
                }
            }
        }
    }
}

extern void  SimpleAwaiter3 ()
{
    std::cout << "\n---- 3.SimpleAwaiter3 ----\n";
    Run();
}
