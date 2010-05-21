/*
   Copyright 2009-2010 Jack Poulson

   This file is part of Elemental.

   Elemental is free software: you can redistribute it and/or modify it under
   the terms of the GNU Lesser General Public License as published by the
   Free Software Foundation; either version 3 of the License, or 
   (at your option) any later version.

   Elemental is distributed in the hope that it will be useful, but 
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with Elemental. If not, see <http://www.gnu.org/licenses/>.
*/
#include "elemental/lapack_internal.hpp"
using namespace std;
using namespace elemental;

template<typename R>
R
elemental::lapack::internal::Reflector
( DistMatrix<R,MC,MR>& x )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::Reflector");
    if( x.Height() != 1 && x.Width() != 1 )
        throw "x must be a vector.";
#endif
    if( max( x.Height(), x.Width() ) <= 1 )
        return (R)0;

    const Grid& grid = x.GetGrid();

    // For partitioning x
    DistMatrix<R,MC,MR> chi1(grid);
    DistMatrix<R,MC,MR> x2(grid);

    if( x.Width() == 1 )
    {
        PartitionDown( x,  chi1,
                           x2,   1 );
    }
    else
    {
        PartitionRight( x,  chi1, x2, 1 );
    }

    R x2_Norm = blas::Nrm2( x2 );
    R alpha = chi1.Get(0,0);
    R beta;
    if( alpha <= 0 )
        beta = wrappers::lapack::SafeNorm( alpha, x2_Norm );
    else
        beta = -wrappers::lapack::SafeNorm( alpha, x2_Norm );

    R safeMin = numeric_limits<R>::min() / numeric_limits<R>::epsilon();
    int count = 0;
    if( Abs( beta ) < safeMin )
    {
        R invOfSafeMin = static_cast<R>(1) / safeMin;
        do
        {
            ++count;
            blas::Scal( invOfSafeMin, x2 );
            alpha *= invOfSafeMin;
            beta *= invOfSafeMin;
        } while( Abs( beta ) < safeMin );

        x2_Norm = blas::Nrm2( x2 );
        if( alpha <= 0 )
            beta = wrappers::lapack::SafeNorm( alpha, x2_Norm );
        else
            beta = -wrappers::lapack::SafeNorm( alpha, x2_Norm );
    }
    R tau = ( beta-alpha ) / beta;
    blas::Scal( static_cast<R>(1)/(alpha-beta), x2 );

    for( int j=0; j<count; ++j )
        beta *= safeMin;
    chi1.Set(0,0,beta);
        
#ifndef RELEASE
    PopCallStack();
#endif
    return tau;
}

template float elemental::lapack::internal::Reflector
( DistMatrix<float,MC,MR>& x );

template double elemental::lapack::internal::Reflector
( DistMatrix<double,MC,MR>& x );
