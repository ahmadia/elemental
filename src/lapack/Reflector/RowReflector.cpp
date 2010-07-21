/*
   Copyright (C) 1992-2008 The University of Tennessee. 
   All rights reserved.

   Copyright (C) 2009-2010 Jack Poulson <jack.poulson@gmail.com>

   This file is part of Elemental.

   Elemental is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Elemental is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Elemental.  If not, see <http://www.gnu.org/licenses/>.

   This file is partially based upon the LAPACK routines dlarfg.f and zlarfg.f.
*/
#include "elemental/lapack_internal.hpp"
using namespace std;
using namespace elemental;
using namespace elemental::wrappers::mpi;

template<typename R>
R
elemental::lapack::internal::RowReflector
( DistMatrix<R,MC,MR>& chi, DistMatrix<R,MC,MR>& x )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::RowReflector");
    if( chi.GetGrid() != x.GetGrid() )
        throw logic_error( "chi and x must be distributed over the same grid" );
    if( chi.Height() != 1 || chi.Width() != 1 )
        throw logic_error( "chi must be a scalar." );
    if( x.Height() != 1 )
        throw logic_error( "x must be a row vector." );
    if( chi.GetGrid().MCRank() != chi.ColAlignment() )
        throw logic_error( "Reflecting with incorrect row of processes." );
    if( x.GetGrid().MCRank() != x.ColAlignment() )
        throw logic_error( "Reflecting with incorrect row of processes." );
#endif
    
    const Grid& g = x.GetGrid();
    const int c = g.Width();
    const int myCol = g.MRRank();

    if( x.Width() == 0 )
    {
        if( myCol == chi.RowAlignment() )
            chi.LocalEntry(0,0) *= (R)-1;
#ifndef RELEASE
        PopCallStack();
#endif
        return (R)2;
    }

    vector<R> localNorms(c);
    R localNorm = blas::Nrm2( x.LockedLocalMatrix() ); 
    AllGather( &localNorm, 1, &localNorms[0], 1, g.MRComm() );
    R norm = wrappers::blas::Nrm2( c, &localNorms[0], 1 );

    R alpha;
    if( myCol == chi.RowAlignment() )
        alpha = chi.LocalEntry(0,0);
    Broadcast( &alpha, 1, chi.RowAlignment(), g.MRComm() );

    R beta;
    if( alpha <= 0 )
        beta = wrappers::lapack::SafeNorm( alpha, norm );
    else
        beta = -wrappers::lapack::SafeNorm( alpha, norm );

    R safeMin = numeric_limits<R>::min() / numeric_limits<R>::epsilon();
    int count = 0;
    if( Abs( beta ) < safeMin )
    {
        R invOfSafeMin = static_cast<R>(1) / safeMin;
        do
        {
            ++count;
            blas::Scal( invOfSafeMin, x );
            alpha *= invOfSafeMin;
            beta *= invOfSafeMin;
        } while( Abs( beta ) < safeMin );

        localNorm = blas::Nrm2( x.LockedLocalMatrix() );
        AllGather( &localNorm, 1, &localNorms[0], 1, g.MRComm() );
        norm = wrappers::blas::Nrm2( c, &localNorms[0], 1 );
        if( alpha <= 0 )
            beta = wrappers::lapack::SafeNorm( alpha, norm );
        else
            beta = -wrappers::lapack::SafeNorm( alpha, norm );
    }

    R tau = ( beta-alpha ) / beta;
    blas::Scal( static_cast<R>(1)/(alpha-beta), x );

    for( int j=0; j<count; ++j )
        beta *= safeMin;
    if( myCol == chi.RowAlignment() )
        chi.LocalEntry(0,0) = beta;
        
#ifndef RELEASE
    PopCallStack();
#endif
    return tau;
}

#ifndef WITHOUT_COMPLEX
template<typename R>
complex<R>
elemental::lapack::internal::RowReflector
( DistMatrix<complex<R>,MC,MR>& chi, DistMatrix<complex<R>,MC,MR>& x )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::RowReflector");    
    if( chi.GetGrid() != x.GetGrid() )
        throw logic_error( "chi and x must be distributed over the same grid" );
    if( chi.Height() != 1 || chi.Width() != 1 )
        throw logic_error( "chi must be a scalar." );
    if( x.Height() != 1 )
        throw logic_error( "x must be a row vector." );
    if( chi.GetGrid().MCRank() != chi.ColAlignment() )
        throw logic_error( "Reflecting with incorrect row of processes." );
    if( x.GetGrid().MCRank() != x.ColAlignment() )
        throw logic_error( "Reflecting with incorrect row of processes." );
#endif
    typedef complex<R> C;

    const Grid& g = x.GetGrid();
    const int c = g.Width();
    const int myCol = g.MRRank();

    vector<R> localNorms(c);
    R localNorm = blas::Nrm2( x.LockedLocalMatrix() ); 
    AllGather( &localNorm, 1, &localNorms[0], 1, g.MRComm() );
    R norm = wrappers::blas::Nrm2( c, &localNorms[0], 1 );

    C alpha;
    if( myCol == chi.RowAlignment() )
        alpha = chi.LocalEntry(0,0);
    Broadcast( &alpha, 1, chi.RowAlignment(), g.MRComm() );

    if( norm == (R)0 && imag(alpha) == (R)0 )
    {
        if( myCol == chi.RowAlignment() )
            chi.LocalEntry(0,0) *= (R)-1;
#ifndef RELEASE
        PopCallStack();
#endif
        return (C)2;
    }

    R beta;
    if( real(alpha) <= 0 )
        beta = wrappers::lapack::SafeNorm( real(alpha), imag(alpha), norm );
    else
        beta = -wrappers::lapack::SafeNorm( real(alpha), imag(alpha), norm );

    R safeMin = numeric_limits<R>::min() / numeric_limits<R>::epsilon();
    int count = 0;
    if( Abs( beta ) < safeMin )
    {
        R invOfSafeMin = static_cast<R>(1) / safeMin;
        do
        {
            ++count;
            blas::Scal( (C)invOfSafeMin, x );
            alpha *= invOfSafeMin;
            beta *= invOfSafeMin;
        } while( Abs( beta ) < safeMin );

        localNorm = blas::Nrm2( x.LockedLocalMatrix() );
        AllGather( &localNorm, 1, &localNorms[0], 1, g.MRComm() );
        norm = wrappers::blas::Nrm2( c, &localNorms[0], 1 );
        if( real(alpha) <= 0 )
        {
            beta = wrappers::lapack::SafeNorm
                   ( real(alpha), imag(alpha), norm );
        }
        else
        {
            beta = -wrappers::lapack::SafeNorm
                    ( real(alpha), imag(alpha), norm );
        }
    }

    C tau = C( (beta-real(alpha))/beta, -imag(alpha)/beta );
    blas::Scal( static_cast<C>(1)/(alpha-beta), x );

    for( int j=0; j<count; ++j )
        beta *= safeMin;
    if( myCol == chi.RowAlignment() )
        chi.LocalEntry(0,0) = beta;
        
#ifndef RELEASE
    PopCallStack();
#endif
    return tau;
}
#endif // WITHOUT_COMPLEX

template float elemental::lapack::internal::RowReflector
( DistMatrix<float,MC,MR>& chi, DistMatrix<float,MC,MR>& x );

template double elemental::lapack::internal::RowReflector
( DistMatrix<double,MC,MR>& chi, DistMatrix<double,MC,MR>& x );

#ifndef WITHOUT_COMPLEX
template scomplex elemental::lapack::internal::RowReflector
( DistMatrix<scomplex,MC,MR>& chi, DistMatrix<scomplex,MC,MR>& x );

template dcomplex elemental::lapack::internal::RowReflector
( DistMatrix<dcomplex,MC,MR>& chi, DistMatrix<dcomplex,MC,MR>& x );
#endif
