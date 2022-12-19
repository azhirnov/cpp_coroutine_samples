# C++20 coroutine samples

1. [SimpleAwaiter](1.SimpleAwaiter.cpp) - how to use `Awaiter`
1. [SimpleAwaiter2](2.SimpleAwaiter2.cpp) - how to suspend and resume coroutine execution
1. [SimpleAwaiter3](3.SimpleAwaiter3.cpp) - simple task system on coroutine, without dependencies between tasks
1. [SimplePromise](4.SimplePromise.cpp) - how to return result from coroutine
1. [TaskDeps](5.TaskDeps-v1.cpp), [v2](5.TaskDeps-v2.cpp), [v3](5.TaskDeps-v3.cpp) - how to make dependencies between coroutines
1. [AwaitOverload](6.AwaitOverload.cpp) - how to specialise `Awaiter` for different coroutine types
1. [GetCurrentCoro](7.GetCurrentCoro.cpp) - how to get coroutine handle inside the coroutine
1. [DestroyUncompleteCoro](8.DestroyUncompleteCoro.cpp) - what happens if uncomplete coroutine has been destroyed
