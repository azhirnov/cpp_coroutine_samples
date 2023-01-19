#include "TaskSystem.h"

namespace
{
    static std::mutex   consoleGuard;

    Task<int>  Coro1 ()
    {
        {
            std::scoped_lock  lock {consoleGuard};
            std::cout << "Coro1, tid:" << std::hex << TID() <<"\n";
        }
        co_return 111;
    }

    Task<float>  Coro2 (Task<int> t0)
    {
        {
            std::scoped_lock  lock {consoleGuard};
            std::cout << "Coro2, tid:" << std::hex << TID() <<"\n";
        }
        int  v = co_await t0;
        {
            std::scoped_lock  lock {consoleGuard};
            std::cout << "Coro2, wait: " << std::dec << v << ", tid:" << std::hex << TID() << "\n";
        }
        co_return 22.7f;
    }

    Task<>  Coro3 (Task<int> t0, Task<float> t1)
    {
        {
            std::scoped_lock  lock {consoleGuard};
            std::cout << "Coro3, tid:" << std::hex << TID() << "\n";
        }
    #if 1
        auto [i, f] = co_await std::tuple{ t0, t1 };
    #else
        // slower version, because suspends 2 times
        auto    i = co_await t0;
        auto    f = co_await t1;
    #endif
        {
            std::scoped_lock  lock {consoleGuard};
            std::cout << "Coro3, wait{ " << std::dec << i << ", " << f << " }, tid:" << std::hex << TID() << "\n";
        }
        co_return;
    }


    void  Run ()
    {
        auto&   ts = TaskSystem::create( 4 );

        auto    t0 = Coro1();
        auto    t1 = Coro2( t0 );
        auto    t2 = Coro3( t0, t1 );

        ts.add( t2 );
        ts.add( t1 );
        ts.add( t0 );

        ts.wait();

        assert( t0.is_complete() );
        assert( t1.is_complete() );
        assert( t2.is_complete() );

        TaskSystem::destroy();
    }
}

extern void  TaskSystemSample ()
{
    std::cout << "\n---- 9.TaskSystem ----\n";
    Run();
}
