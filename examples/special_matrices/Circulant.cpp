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
#include "elemental.hpp"
using namespace elem;

void Usage()
{
    std::cout << "Circulant <n>\n"
              << "  n: Height and width of matrix, n >= 1\n"
              << std::endl;
}

int 
main( int argc, char* argv[] )
{
    Initialize( argc, argv );
    mpi::Comm comm = mpi::COMM_WORLD;
    const int commRank = mpi::CommRank( comm );
    const int commSize = mpi::CommSize( comm );

    if( argc < 2 )
    {
        if( commRank == 0 )
            Usage();
        Finalize();
        return 0;
    }
    const int n = atoi( argv[1] );

    try
    {
        // Create a circulant matrix
        DistMatrix<Complex<double> > A;
        std::vector<Complex<double> > a( n );
        for( int j=0; j<n; ++j )
            a[j] = j;
        Circulant( a, A );
        A.Print("Circulant matrix:");

        // Create a discrete Fourier matrix, which can be used to diagonalize
        // circulant matrices
        DistMatrix<Complex<double> > F;
        DiscreteFourier( n, F );
        F.Print("DFT matrix (F):");
        
        // Form B := A F
        DistMatrix<Complex<double> > B;
        Zeros( n, n, B );
        Gemm( NORMAL, NORMAL, 
              Complex<double>(1), A, F, Complex<double>(0), B );

        // Form A := F^H B = F^H \hat A F
        Gemm( ADJOINT, NORMAL,
              Complex<double>(1), F, B, Complex<double>(0), A );
        A.Print("A := F^H A F");

        // Form the thresholded result
        const int localHeight = A.LocalHeight();
        const int localWidth = A.LocalWidth();
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            for( int iLocal=0; iLocal<localHeight; ++iLocal )
            {
                const double absValue = Abs(A.GetLocalEntry(iLocal,jLocal));
                if( absValue < 1e-13 )
                    A.SetLocalEntry(iLocal,jLocal,0);
            }
        }
        A.Print("A with values below 1e-13 removed");
    }
    catch( std::exception& e )
    {
#ifndef RELEASE
        DumpCallStack();
#endif
        std::cerr << "Process " << commRank << " caught error message:\n"
                  << e.what() << std::endl;
    }

    Finalize();
    return 0;
}

