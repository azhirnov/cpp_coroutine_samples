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
        };

        handle_t    _handle;
        
        Task (handle_t h) : _handle{h}  {}
        ~Task ()                        {}

        void  Resume ()                 { _handle.resume(); }
        void  Destroy ()                { _handle.destroy(); }

        explicit operator bool () const { return _handle and not _handle.done(); }
    };
    

    std::suspend_always  Suspend ()
    {
        return {};
    }


    void  Run ()
    {
        Task    t0 = [] () -> Task
        {
            // <-- resume

            std::string  str = "string memory must be released";
            str += ", even if coroutine is not complete";

            void*  mem = nullptr;
            //mem = ::operator new(1024);  // mem leak

            co_await Suspend();
            
            // <-- resume
            // this part of coroutine has never been executed

            std::cout << "resume 2\n";

            ::operator delete(mem);
            co_return;
        }();
        
        t0.Resume();

        if ( t0 )
            std::cout << "coroutine is not complete\n";

        // anyway destroy the coroutine
        t0.Destroy();
    }
}

extern void  DestroyUncompleteCoro ()
{
    std::cout << "\n---- 8.DestroyUncompleteCoro ----\n";
    Run();
}
