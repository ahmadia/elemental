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

#include "./Trsv/TrsvLN.hpp"
#include "./Trsv/TrsvLT.hpp"
#include "./Trsv/TrsvUN.hpp"
#include "./Trsv/TrsvUT.hpp"

namespace elem {

template<typename F>
inline void
Trsv
( UpperOrLower uplo,
  Orientation orientation,
  UnitOrNonUnit diag,
  const DistMatrix<F,MC,MR>& A,
        DistMatrix<F,MC,MR>& x   )
{
#ifndef RELEASE
    PushCallStack("Trsv");
#endif
    if( uplo == LOWER )
    {
        if( orientation == NORMAL )
            internal::TrsvLN( diag, A, x );
        else
            internal::TrsvLT( orientation, diag, A, x );
    }
    else
    {
        if( orientation == NORMAL )
            internal::TrsvUN( diag, A, x );
        else
            internal::TrsvUT( orientation, diag, A, x );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

} // namespace elem
