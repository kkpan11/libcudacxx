//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// UNSUPPORTED: libcpp-has-no-threads

// Threading tests are notoriously flaky in libcxx mode so disable them
// UNSUPPORTED: true

// <mutex>

// class timed_mutex;

// bool try_lock();

#include <mutex>
#include <thread>
#include <cstdlib>
#include <cassert>

#include "test_macros.h"

std::timed_mutex m;

typedef std::chrono::system_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef std::chrono::milliseconds ms;
typedef std::chrono::nanoseconds ns;

void f()
{
    time_point t0 = Clock::now();
    assert(!m.try_lock());
    assert(!m.try_lock());
    assert(!m.try_lock());
    while(!m.try_lock())
        ;
    time_point t1 = Clock::now();
    m.unlock();
    ns d = t1 - t0 - ms(250);
    assert(d < ms(200));  // within 200ms
}

int main(int, char**)
{
    m.lock();
    std::thread t(f);
    std::this_thread::sleep_for(ms(250));
    m.unlock();
    t.join();

  return 0;
}
