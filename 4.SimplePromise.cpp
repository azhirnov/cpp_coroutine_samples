#include "Common.h"

namespace
{
    template <typename ResultType>
    struct Promise
    {
        using Self = Promise< ResultType >;

        struct promise_type;
        using handle_t = std::coroutine_handle< promise_type >;

        struct promise_type
        {
            ResultType          _value;
            std::exception_ptr  _exception;

            Self                get_return_object ()            { return Self{ handle_t::from_promise( *this )}; }
            std::suspend_always initial_suspend () noexcept     { return {}; }                                  // delayed start
            std::suspend_always final_suspend () noexcept       { return {}; }                                  // must not be 'suspend_never'
            void                return_value (ResultType value) { _value = value; }                             // set value by 'co_return'
            std::suspend_always yield_value (ResultType value)  { _value = value;  return {}; }                 // set value by 'co_yield' and suspend
            void                unhandled_exception ()          { _exception = std::current_exception(); }      // save exception
        };

        handle_t    _handle;

        Promise (handle_t h) : _handle{h}   {}
        ~Promise ()                         { _handle.destroy(); }

        [[nodiscard]] ResultType  operator () ()
        {
            if ( not _handle )
                return {};

            if ( not _handle.done() )
            {
                _handle.resume();
                // <-- after co_yield / co_return / exception
            }

            if ( _handle.promise()._exception )
                std::rethrow_exception( _handle.promise()._exception );

            return _handle.promise()._value;
        }
    };

    void  Run ()
    {
        auto    create_promise = [] (int initial) -> Promise<int>
        {
            // paused because of 'initial_suspend'

            if ( initial == 0 )
                co_return -1;       // promise().return_value(...)

            co_yield initial - 2;   // promise().yield_value(...) and pause
            co_yield initial - 1;   // promise().yield_value(...) and pause
            co_return initial;      // promise().return_value(...)
        };

        auto    p0 = create_promise( 0 );
        auto    p1 = create_promise( 5 );

        int     r0_0 = p0();    // -1
        int     r0_1 = p0();    // -1
        int     r0_2 = p0();    // -1

        int     r1_0 = p1();    // 3
        int     r1_1 = p1();    // 4
        int     r1_2 = p1();    // 5

        std::cout << "Promise0: " << r0_0 << ", " << r0_1 << ", " << r0_2 << "\n";
        std::cout << "Promise1: " << r1_0 << ", " << r1_1 << ", " << r1_2 << "\n";
    }
}

extern void  SimplePromise ()
{
    std::cout << "\n---- 4.SimplePromise ----\n";
    Run();
}
