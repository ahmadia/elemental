/*
   Copyright (c) 2009-2012, Jack Poulson
   All rights reserved.

   This file is part of Elemental.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include "./Norm/FrobeniusNorm.hpp"
#include "./Norm/InfinityNorm.hpp"
#include "./Norm/MaxNorm.hpp"
#include "./Norm/OneNorm.hpp"

namespace elem {

template<typename F>
inline typename Base<F>::type
Norm( const Matrix<F>& A, NormType type )
{
#ifndef RELEASE
    PushCallStack("Norm");
#endif
    typedef typename Base<F>::type R;

    R norm = 0;
    switch( type )
    {
    case INFINITY_NORM:
        norm = internal::InfinityNorm( A );
        break;
    case FROBENIUS_NORM: 
        norm = internal::FrobeniusNorm( A );
        break;
    case MAX_NORM:
        norm = internal::MaxNorm( A );
        break;
    case ONE_NORM:
        norm = internal::OneNorm( A );
        break;
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return norm;
}

template<typename F> 
inline typename Base<F>::type
Norm( const DistMatrix<F,MC,MR>& A, NormType type )
{
#ifndef RELEASE
    PushCallStack("Norm");
#endif
    typedef typename Base<F>::type R;

    R norm = 0;
    switch( type )
    {
    case INFINITY_NORM:
        norm = internal::InfinityNorm( A );
        break;
    case FROBENIUS_NORM: 
        norm = internal::FrobeniusNorm( A );
        break;
    case MAX_NORM:
        norm = internal::MaxNorm( A );
        break;
    case ONE_NORM:
        norm = internal::OneNorm( A );
        break;
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return norm;
}

} // namespace elem
