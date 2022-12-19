#include "Common.h"

namespace
{
    struct Task
    {
        struct promise_type;
        using handle_t = std::coroutine_handle< promise_type >;

        struct promise_type
        {
            Task*   task = nullptr;

            Task                get_return_object ()        { return Task{ handle_t::from_promise( *this )}; }
            std::suspend_always initial_suspend () noexcept { return {}; }        // delayed start
            std::suspend_always final_suspend () noexcept   { return {}; }        // must not be 'suspend_never'
            void                return_void ()              {}
            void                unhandled_exception ()      {}
        };

        handle_t    _handle;
        
        Task (handle_t h) : _handle{h}  { _handle.promise().task = this; }
        ~Task ()                        { _handle.destroy(); }

        void  Resume ()                 { _handle.resume(); }

        explicit operator bool () const { return _handle and not _handle.done(); }
    };
    

    struct Task_Get
    {
        constexpr Task_Get () {}

        [[nodiscard]] auto  operator co_await () const
        {
            struct Awaiter
            {
            private:
                Task*	_task = nullptr;

            public:
                bool	await_ready ()		const	{ return false; }   // call 'await_suspend()' to get coroutine handle
                Task &	await_resume ()		    	{ return *_task; }

                bool    await_suspend (std::coroutine_handle<Task::promise_type> curCoro)
                {
                    _task = curCoro.promise().task;
                    return false;   // always resume coroutine
                }
            };
            return Awaiter{};
        }
    };
    static constexpr Task_Get   Task_Get2 {};


    void  Run ()
    {
        Task    t0 = [] () -> Task
        {
            // <-- resume

            // implicitly used:
            //   Task_Get::Awaiter::await_ready()
            //   Task_Get::Awaiter::await_suspend()
            //   Task_Get::Awaiter::await_resume()  - returns Task&

            Task&   self  = co_await Task_Get{};
            Task&   self2 = co_await Task_Get2;

            (void)(self);
            (void)(self2);
            co_return;
        }();
        
        t0.Resume();
    }
}

extern void  GetCurrentCoro ()
{
    std::cout << "\n---- 7.GetCurrentCoro ----\n";
    Run();
}
