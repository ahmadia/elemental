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

namespace elem {

template<typename F> 
inline typename Base<F>::type
internal::InfinityNorm( const Matrix<F>& A )
{
#ifndef RELEASE
    PushCallStack("internal::InfinityNorm");
#endif
    typedef typename Base<F>::type R;

    R maxRowSum = 0;
    for( int i=0; i<A.Height(); ++i )
    {
        R rowSum = 0;
        for( int j=0; j<A.Width(); ++j )
            rowSum += Abs(A.Get(i,j));
        maxRowSum = std::max( maxRowSum, rowSum );
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return maxRowSum;
}

template<typename F> 
inline typename Base<F>::type
internal::InfinityNorm( const DistMatrix<F,MC,MR>& A )
{
#ifndef RELEASE
    PushCallStack("internal::InfinityNorm");
#endif
    typedef typename Base<F>::type R;

    // Compute the partial row sums defined by our local matrix, A[MC,MR]
    std::vector<R> myPartialRowSums(A.LocalHeight());
    for( int iLocal=0; iLocal<A.LocalHeight(); ++iLocal )
    {
        myPartialRowSums[iLocal] = 0;
        for( int jLocal=0; jLocal<A.LocalWidth(); ++jLocal )
            myPartialRowSums[iLocal] += Abs(A.GetLocalEntry(iLocal,jLocal));
    }

    // Sum our partial row sums to get the row sums over A[MC,* ]
    std::vector<R> myRowSums(A.LocalHeight());
    mpi::AllReduce
    ( &myPartialRowSums[0], &myRowSums[0], A.LocalHeight(), mpi::SUM,
      A.Grid().RowComm() );

    // Find the maximum out of the row sums
    R myMaxRowSum = 0;
    for( int iLocal=0; iLocal<A.LocalHeight(); ++iLocal )
        myMaxRowSum = std::max( myMaxRowSum, myRowSums[iLocal] );

    // Find the global maximum row sum by searching over the MC team
    R maxRowSum = 0;
    mpi::AllReduce( &myMaxRowSum, &maxRowSum, 1, mpi::MAX, A.Grid().ColComm() );
#ifndef RELEASE
    PopCallStack();
#endif
    return maxRowSum;
}

} // namespace elem
