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
using namespace std;
using namespace elem;

void Usage()
{
    cout << "SYmmetric Rank-2K update.\n\n"
         << "  Syr2k <r> <c> <uplo> <trans?> <m> <k> <nb> <rank2K local nb> "
            "<print?>\n\n"
         << "  r: number of process rows\n"
         << "  c: number of process cols\n"
         << "  uplo: {L,U}\n"
         << "  trans: {N,T}\n"
         << "  m: height of C\n"
         << "  k: inner dimension\n"
         << "  nb: algorithmic blocksize\n"
         << "  rank2K local nb: local blocksize for rank-2k update\n"
         << "  print?: false iff 0\n" << endl;
}

template<typename T> // represents a real or complex ring
void TestSyr2k
( bool printMatrices, UpperOrLower uplo, Orientation orientation,
  int m, int k, T alpha, T beta, const Grid& g )
{
    double startTime, endTime, runTime, gFlops;
    DistMatrix<T,MC,MR> A(g), B(g), C(g);

    if( orientation == NORMAL )
    {
        Uniform( m, k, A );
        Uniform( m, k, B );
    }
    else
    {
        Uniform( k, m, A );
        Uniform( k, m, B );
    }
    Uniform( m, m, C );
    MakeTrapezoidal( LEFT, uplo, 0, C );
    if( printMatrices )
    {
        A.Print("A");
        B.Print("B");
        C.Print("C");
    }

    if( g.Rank() == 0 )
    {
        cout << "  Starting Syr2k...";
        cout.flush();
    }
    mpi::Barrier( g.Comm() );
    startTime = mpi::Time();
    Syr2k( uplo, orientation, alpha, A, B, beta, C );
    mpi::Barrier( g.Comm() );
    endTime = mpi::Time();
    runTime = endTime - startTime;
    gFlops = internal::Syr2kGFlops<T>(m,k,runTime);
    if( g.Rank() == 0 )
    {
        cout << "DONE. " << endl
             << "  Time = " << runTime << " seconds. GFlops = " 
             << gFlops << endl;
    }
    if( printMatrices )
    {
        ostringstream msg;
        if( orientation == NORMAL )
            msg << "C := " << alpha << " A B' + B A'" << beta << " C";
        else
            msg << "C := " << alpha << " A' B + B' A" << beta << " C";
        C.Print( msg.str() );
    }
}

int 
main( int argc, char* argv[] )
{
    Initialize( argc, argv );
    mpi::Comm comm = mpi::COMM_WORLD;
    const int rank = mpi::CommRank( comm );

    if( argc < 10 )
    {
        if( rank == 0 )
            Usage();
        Finalize();
        return 0;
    }

    try
    {
        int argNum = 0;
        const int r = atoi(argv[++argNum]);
        const int c = atoi(argv[++argNum]);
        const UpperOrLower uplo = CharToUpperOrLower(*argv[++argNum]);
        const Orientation orientation = CharToOrientation(*argv[++argNum]);
        const int m = atoi(argv[++argNum]);
        const int k = atoi(argv[++argNum]);
        const int nb = atoi(argv[++argNum]);
        const int nbLocal = atoi(argv[++argNum]);
        const bool printMatrices = atoi(argv[++argNum]);
#ifndef RELEASE
        if( rank == 0 )
        {
            cout << "==========================================\n"
                 << " In debug mode! Performance will be poor! \n"
                 << "==========================================" << endl;
        }
#endif
        const Grid g( comm, r, c );
        SetBlocksize( nb );
        SetLocalTrr2kBlocksize<double>( nbLocal );
        SetLocalTrr2kBlocksize<Complex<double> >( nbLocal );

        if( rank == 0 )
        {
            cout << "Will test Syr2k" << UpperOrLowerToChar(uplo) 
                                      << OrientationToChar(orientation) << endl;
        }

        if( rank == 0 )
        {
            cout << "---------------------\n"
                 << "Testing with doubles:\n"
                 << "---------------------" << endl;
        }
        TestSyr2k<double>
        ( printMatrices, uplo, orientation, 
          m, k, (double)3, (double)4, g );

        if( rank == 0 )
        {
            cout << "--------------------------------------\n"
                 << "Testing with double-precision complex:\n"
                 << "--------------------------------------" << endl;
        }
        TestSyr2k<Complex<double> >
        ( printMatrices, uplo, orientation, 
          m, k, Complex<double>(3), Complex<double>(4), g );
    }
    catch( exception& e )
    {
#ifndef RELEASE
        DumpCallStack();
#endif
        cerr << "Process " << rank << " caught error message:\n"
             << e.what() << endl;
    }
    Finalize();
    return 0;
}

