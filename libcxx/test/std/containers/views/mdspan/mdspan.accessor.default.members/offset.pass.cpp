//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//UNSUPPORTED: c++11

#include <mdspan>
#include <cassert>

int main(int, char**)
{
    {
        using  element_t = int;
        std::array<element_t, 2> d{42,43};
        std::default_accessor<element_t> a;

        assert( a.offset( d.data(), 0 ) == d.data()     );
        assert( a.offset( d.data(), 1 ) == d.data() + 1 );
    }

    return 0;
}
