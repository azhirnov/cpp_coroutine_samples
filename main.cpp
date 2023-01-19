
extern void  SimpleAwaiter ();
extern void  SimpleAwaiter2 ();
extern void  SimpleAwaiter3 ();
extern void  SimplePromise ();
extern void  TaskDeps_v1 ();
extern void  TaskDeps_v2 ();
extern void  TaskDeps_v3 ();
extern void  AwaitOverlod ();
extern void  GetCurrentCoro ();
extern void  DestroyUncompleteCoro ();
extern void  TaskSystemSample ();

// to use '_CrtDumpMemoryLeaks()'
#ifdef _MSC_VER
#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#   include <stdlib.h>
#   include <malloc.h>
#endif
#include <cassert>


int  main ()
{
    SimpleAwaiter();        // 1
    SimpleAwaiter2();       // 2
    SimpleAwaiter3();       // 3
    SimplePromise();        // 4
    TaskDeps_v1();          // 5
    TaskDeps_v2();          // 5
    TaskDeps_v3();          // 5
    AwaitOverlod();         // 6
    GetCurrentCoro();       // 7
    DestroyUncompleteCoro();// 8
    TaskSystemSample();     // 9

    // check for memleaks
    #ifdef _MSC_VER
        assert( ::_CrtDumpMemoryLeaks() != 1 );
    #endif

    return 0;
}
