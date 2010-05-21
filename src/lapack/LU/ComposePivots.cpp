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
using namespace elemental::utilities;
using namespace elemental::wrappers::mpi;

void
elemental::lapack::internal::ComposePivots
( const DistMatrix<int,Star,Star>& p, 
        vector<int>& image,
        vector<int>& preimage,
        int pivotOffset )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::ComposePivots");
    if( p.Width() != 1 )
        throw "p must be a column vector.";
    if( pivotOffset < 0 )
        throw "pivotOffset must be non-negative.";
#endif
    const int b = p.Height();
    image.resize( b );
    preimage.resize( b );

    // Construct the image of {0,...,b-1} under the permutation
    for( int i=0; i<b; ++i )
    {
        int row = i;
        for( int j=0; j<min(b,row+1); ++j )
        {
            if( p.LocalEntry(j,0)-pivotOffset == row )
                row = j;
            else if( j == row )
                row = p.LocalEntry(j,0)-pivotOffset;
        }
        image[i] = row;
    }

    // Construct the preimage of {0,...,b-1} under the permutation
    for( int i=0; i<b; ++i )
    {
        int row = p.LocalEntry(i,0)-pivotOffset;
        for( int j=i-1; j>=0; --j )
        {
            if( p.LocalEntry(j,0)-pivotOffset == row )
                row = j;
            else if( j == row )
                row = p.LocalEntry(j,0)-pivotOffset;
        }
        preimage[i] = row;
    }

#ifndef RELEASE
    PopCallStack();
#endif
}
