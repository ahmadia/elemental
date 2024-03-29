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

namespace internal {

template<typename R>
struct IndexValuePair {
    int index;
    R value;    

    static bool Compare
    ( const IndexValuePair<R>& a, const IndexValuePair<R>& b )
    { return a.value < b.value; }
};

template<typename R>
struct IndexValuePair<Complex<R> > {
    int index;
    Complex<R> value;    

    static bool Compare
    ( const IndexValuePair<R>& a, const IndexValuePair<R>& b )
    { return Abs(a.value) < Abs(b.value); }
};

} // namespace internal

template<typename R>
inline void
SortEig( DistMatrix<R,VR,STAR>& w )
{
#ifndef RELEASE
    PushCallStack("SortEig");
#endif
    const int k = w.Height();

    // Gather a full copy of w on each process and locally sort
    DistMatrix<R,STAR,STAR> w_STAR_STAR( w );
    R* wBuffer = w_STAR_STAR.LocalBuffer();
    std::sort( &wBuffer[0], &wBuffer[k] );

    // Refill the distributed w with the sorted values
    w = w_STAR_STAR;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R>
inline void
SortEig( DistMatrix<R,VR,STAR>& w, DistMatrix<R,MC,MR>& Z )
{
#ifndef RELEASE
    PushCallStack("SortEig");
#endif
    const int n = Z.Height();
    const int k = Z.Width();
    const Grid& g = Z.Grid();

    DistMatrix<R,VC,STAR> Z_VC_STAR( Z );
    DistMatrix<R,STAR,STAR> w_STAR_STAR( w );

    // Initialize the pairs of indices and eigenvalues
    std::vector<internal::IndexValuePair<R> > pairs( k );
    for( int i=0; i<k; ++i )
    {
        pairs[i].index = i;
        pairs[i].value = w_STAR_STAR.GetLocalEntry(i,0);
    }

    // Sort the eigenvalues and simultaneously form the permutation
    std::sort
    ( pairs.begin(), pairs.end(), internal::IndexValuePair<R>::Compare );

    // Locally reorder the eigenvectors and eigenvalues using the new ordering
    const int mLocal = Z_VC_STAR.LocalHeight();
    DistMatrix<R,VC,STAR> ZPerm_VC_STAR( n, k, g );
    for( int j=0; j<k; ++j )
    {
        const int source = pairs[j].index;
        MemCopy
        ( ZPerm_VC_STAR.LocalBuffer(0,j), 
          Z_VC_STAR.LockedLocalBuffer(0,source), mLocal );
        w_STAR_STAR.SetLocalEntry(j,0,pairs[j].value);
    }
    Z_VC_STAR.Empty();

    Z = ZPerm_VC_STAR;
    w = w_STAR_STAR;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R> 
inline void
SortEig( DistMatrix<R,VR,STAR>& w, DistMatrix<Complex<R>,MC,MR>& Z )
{
#ifndef RELEASE
    PushCallStack("SortEig");
#endif
    const int n = Z.Height();
    const int k = Z.Width();
    const Grid& g = Z.Grid();

    DistMatrix<Complex<R>,VC,STAR> Z_VC_STAR( Z );
    DistMatrix<R,STAR,STAR> w_STAR_STAR( w );

    // Initialize the pairs of indices and eigenvalues
    std::vector<internal::IndexValuePair<R> > pairs( k );
    for( int i=0; i<k; ++i )
    {
        pairs[i].index = i;
        pairs[i].value = w_STAR_STAR.GetLocalEntry(i,0);
    }

    // Sort the eigenvalues and simultaneously form the permutation
    std::sort
    ( pairs.begin(), pairs.end(), internal::IndexValuePair<R>::Compare );

    // Locally reorder the eigenvectors and eigenvalues using the new ordering
    const int mLocal = Z_VC_STAR.LocalHeight();
    DistMatrix<Complex<R>,VC,STAR> ZPerm_VC_STAR( n, k, g );
    for( int j=0; j<k; ++j )
    {
        const int source = pairs[j].index;
        MemCopy
        ( ZPerm_VC_STAR.LocalBuffer(0,j), 
          Z_VC_STAR.LockedLocalBuffer(0,source), mLocal );
        w_STAR_STAR.SetLocalEntry(j,0,pairs[j].value);
    }
    Z_VC_STAR.Empty();

    Z = ZPerm_VC_STAR;
    w = w_STAR_STAR;
#ifndef RELEASE
    PopCallStack();
#endif
}

} // namespace elem
