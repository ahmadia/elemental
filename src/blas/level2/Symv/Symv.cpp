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
#include "elemental/blas_internal.hpp"
using namespace std;
using namespace elemental;

template<typename T>
void
elemental::blas::Symv
( Shape shape,
  T alpha, const DistMatrix<T,MC,MR>& A,
           const DistMatrix<T,MC,MR>& x,
  T beta,        DistMatrix<T,MC,MR>& y )
{
#ifndef RELEASE
    PushCallStack("blas::Symv");
    if( A.GetGrid() != x.GetGrid() || x.GetGrid() != y.GetGrid() )
        throw "{A,x,y} must be distributed over the same grid.";
    if( A.Height() != A.Width() )
        throw "A must be square.";
    if( ( x.Width() != 1 && x.Height() != 1 ) ||
        ( y.Width() != 1 && y.Height() != 1 )   )
    {
        throw "x and y are assumed to be vectors.";
    }
    const int xLength = ( x.Width()==1 ? x.Height() : x.Width() );
    const int yLength = ( y.Width()==1 ? y.Height() : y.Width() );
    if( A.Height() != xLength || A.Height() != yLength )
    {
        ostringstream msg;
        msg << "Nonconformal Symv: " << endl
            << "  A ~ " << A.Height() << " x " << A.Width() << endl
            << "  x ~ " << x.Height() << " x " << x.Width() << endl
            << "  y ~ " << y.Height() << " x " << y.Width() << endl;
        const string& s = msg.str();
        throw s.c_str();
    }
#endif
    const Grid& grid = A.GetGrid();

    if( x.Width() == 1 && y.Width() == 1 )
    {
        // Temporary distributions
        DistMatrix<T,MC,Star> x_MC_Star(grid);
        DistMatrix<T,MR,Star> x_MR_Star(grid);
        DistMatrix<T,MC,Star> z_MC_Star(grid);
        DistMatrix<T,MR,Star> z_MR_Star(grid);
        DistMatrix<T,MR,MC  > z_MR_MC(grid);
        DistMatrix<T,MC,MR  > z(grid);

        // Begin the algoritm
        blas::Scal( beta, y );
        x_MC_Star.ConformWith( A );
        x_MR_Star.ConformWith( A );
        z_MC_Star.AlignWith( A );
        z_MR_Star.AlignWith( A );
        z_MC_Star.ResizeTo( y.Height(), 1 );
        z_MR_Star.ResizeTo( y.Height(), 1 );
        z_MC_Star.SetToZero();
        z_MR_Star.SetToZero();
        z.AlignWith( y );
        //--------------------------------------------------------------------//
        x_MC_Star = x;
        x_MR_Star = x_MC_Star;
        blas::internal::SymvColAccumulate
        ( shape, alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );

        z_MR_MC.ReduceScatterFrom( z_MR_Star );
        z = z_MR_MC;
        z.ReduceScatterUpdate( (T)1, z_MC_Star );
        blas::Axpy( (T)1, z, y );
        //--------------------------------------------------------------------//
        x_MC_Star.FreeConstraints();
        x_MR_Star.FreeConstraints();
        z_MC_Star.FreeConstraints();
        z_MR_Star.FreeConstraints();
        z.FreeConstraints();
    }
    else if( x.Width() == 1 )
    {
        // Temporary distributions
        DistMatrix<T,MC,Star> x_MC_Star(grid);
        DistMatrix<T,MR,Star> x_MR_Star(grid);
        DistMatrix<T,MC,Star> z_MC_Star(grid);
        DistMatrix<T,MR,Star> z_MR_Star(grid);
        DistMatrix<T,MC,MR  > z(grid);
        DistMatrix<T,MR,MC  > z_MR_MC(grid);
        DistMatrix<T,MC,MR  > zTrans(grid);

        // Begin the algoritm
        blas::Scal( beta, y );
        x_MC_Star.ConformWith( A );
        x_MR_Star.ConformWith( A );
        z_MC_Star.AlignWith( A );
        z_MR_Star.AlignWith( A );
        z_MC_Star.ResizeTo( y.Width(), 1 );
        z_MR_Star.ResizeTo( y.Width(), 1 );
        z_MC_Star.SetToZero();
        z_MR_Star.SetToZero();
        z.AlignWith( y );
        z_MR_MC.AlignWith( y );
        //--------------------------------------------------------------------//
        x_MC_Star = x;
        x_MR_Star = x_MC_Star;
        blas::internal::SymvColAccumulate
        ( shape, alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );

        z.ReduceScatterFrom( z_MC_Star );
        z_MR_MC = z;
        z_MR_MC.ReduceScatterUpdate( (T)1, z_MR_Star );
        blas::Trans( z_MR_MC, zTrans );
        blas::Axpy( (T)1, zTrans, y );
        //--------------------------------------------------------------------//
        x_MC_Star.FreeConstraints();
        x_MR_Star.FreeConstraints();
        z_MC_Star.FreeConstraints();
        z_MR_Star.FreeConstraints();
        z.FreeConstraints();
        z_MR_MC.FreeConstraints();
    }
    else if( y.Width() == 1 )
    {
        // Temporary distributions
        DistMatrix<T,Star,MC> x_Star_MC(grid);
        DistMatrix<T,Star,MR> x_Star_MR(grid);
        DistMatrix<T,Star,MC> z_Star_MC(grid);
        DistMatrix<T,Star,MR> z_Star_MR(grid);
        DistMatrix<T,MC,  MR> z(grid);
        DistMatrix<T,MC,  MR> zTrans(grid);
        DistMatrix<T,MR,  MC> z_MR_MC(grid);

        // Begin the algoritm
        blas::Scal( beta, y );
        x_Star_MC.ConformWith( A );
        x_Star_MR.ConformWith( A );
        z_Star_MC.AlignWith( A );
        z_Star_MR.AlignWith( A );
        z_Star_MC.ResizeTo( 1, y.Height() );
        z_Star_MR.ResizeTo( 1, y.Height() );
        z_Star_MC.SetToZero();
        z_Star_MR.SetToZero();
        z.AlignWith( y );
        z_MR_MC.AlignWith( y );
        //--------------------------------------------------------------------//
        x_Star_MR = x;
        x_Star_MC = x_Star_MR;
        blas::internal::SymvRowAccumulate
        ( shape, alpha, A, x_Star_MC, x_Star_MR, z_Star_MC, z_Star_MR );

        z.ReduceScatterFrom( z_Star_MR );
        z_MR_MC = z;
        z_MR_MC.ReduceScatterUpdate( (T)1, z_Star_MC );
        blas::Trans( z_MR_MC, zTrans );
        blas::Axpy( (T)1, zTrans, y );
        //--------------------------------------------------------------------//
        x_Star_MC.FreeConstraints();
        x_Star_MR.FreeConstraints();
        z_Star_MC.FreeConstraints();
        z_Star_MR.FreeConstraints();
        z.FreeConstraints();
        z_MR_MC.FreeConstraints();
    }
    else
    {
        // Temporary distributions
        DistMatrix<T,Star,MC> x_Star_MC(grid);
        DistMatrix<T,Star,MR> x_Star_MR(grid);
        DistMatrix<T,Star,MC> z_Star_MC(grid);
        DistMatrix<T,Star,MR> z_Star_MR(grid);
        DistMatrix<T,MC,  MR> z(grid);
        DistMatrix<T,MR,  MC> z_MR_MC(grid);

        // Begin the algoritm
        blas::Scal( beta, y );
        x_Star_MC.ConformWith( A );
        x_Star_MR.ConformWith( A );
        z_Star_MC.AlignWith( A );
        z_Star_MR.AlignWith( A );
        z_Star_MC.ResizeTo( 1, y.Width() );
        z_Star_MR.ResizeTo( 1, y.Width() );
        z_Star_MC.SetToZero();
        z_Star_MR.SetToZero();
        z.AlignWith( y );
        z_MR_MC.AlignWith( y );
        //--------------------------------------------------------------------//
        x_Star_MR = x;
        x_Star_MC = x_Star_MR;
        blas::internal::SymvRowAccumulate
        ( shape, alpha, A, x_Star_MC, x_Star_MR, z_Star_MC, z_Star_MR );

        z_MR_MC.ReduceScatterFrom( z_Star_MC );
        z = z_MR_MC;
        z.ReduceScatterUpdate( (T)1, z_Star_MR );
        blas::Axpy( (T)1, z, y );
        //--------------------------------------------------------------------//
        x_Star_MC.FreeConstraints();
        x_Star_MR.FreeConstraints();
        z_Star_MC.FreeConstraints();
        z_Star_MR.FreeConstraints();
        z.FreeConstraints();
        z_MR_MC.FreeConstraints();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::blas::internal::SymvColAccumulate
( Shape shape,
  T alpha, const DistMatrix<T,MC,MR  >& A,
           const DistMatrix<T,MC,Star>& x_MC_Star,
           const DistMatrix<T,MR,Star>& x_MR_Star,
                 DistMatrix<T,MC,Star>& z_MC_Star,
                 DistMatrix<T,MR,Star>& z_MR_Star )
{
#ifndef RELEASE
    PushCallStack("blas::internal::SymvColAccumulate");
#endif
    if( shape == Lower )
    {
        blas::internal::SymvColAccumulateL
        ( alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );
    }
    else
    {
        blas::internal::SymvColAccumulateU
        ( alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::blas::internal::SymvRowAccumulate
( Shape shape,
  T alpha, const DistMatrix<T,MC,  MR>& A,
           const DistMatrix<T,Star,MC>& x_Star_MC,
           const DistMatrix<T,Star,MR>& x_Star_MR,
                 DistMatrix<T,Star,MC>& z_Star_MC,
                 DistMatrix<T,Star,MR>& z_Star_MR )
{
#ifndef RELEASE
    PushCallStack("blas::internal::SymvRowAccumulate");
#endif
    if( shape == Lower )
    {
        blas::internal::SymvRowAccumulateL
        ( alpha, A, x_Star_MC, x_Star_MR, z_Star_MC, z_Star_MR );
    }
    else
    {
        blas::internal::SymvRowAccumulateU
        ( alpha, A, x_Star_MC, x_Star_MR, z_Star_MC, z_Star_MR );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template void elemental::blas::Symv
( Shape shape,
  float alpha, const DistMatrix<float,MC,MR>& A,
               const DistMatrix<float,MC,MR>& x,
  float beta,        DistMatrix<float,MC,MR>& y );

template void elemental::blas::internal::SymvColAccumulate
( Shape shape,
  float alpha, const DistMatrix<float,MC,MR  >& A,
               const DistMatrix<float,MC,Star>& x_MC_Star,
               const DistMatrix<float,MR,Star>& x_MR_Star,
                     DistMatrix<float,MC,Star>& z_MC_Star,
                     DistMatrix<float,MR,Star>& z_MR_Star );

template void elemental::blas::internal::SymvRowAccumulate
( Shape shape,
  float alpha, const DistMatrix<float,MC,  MR>& A,
               const DistMatrix<float,Star,MC>& x_Star_MC,
               const DistMatrix<float,Star,MR>& x_Star_MR,
                     DistMatrix<float,Star,MC>& z_Star_MC,
                     DistMatrix<float,Star,MR>& z_Star_MR );

template void elemental::blas::Symv
( Shape shape,
  double alpha, const DistMatrix<double,MC,MR>& A,
                const DistMatrix<double,MC,MR>& x,
  double beta,        DistMatrix<double,MC,MR>& y );

template void elemental::blas::internal::SymvColAccumulate
( Shape shape,
  double alpha, const DistMatrix<double,MC,MR  >& A,
                const DistMatrix<double,MC,Star>& x_MC_Star,
                const DistMatrix<double,MR,Star>& x_MR_Star,
                      DistMatrix<double,MC,Star>& z_MC_Star,
                      DistMatrix<double,MR,Star>& z_MR_Star );

template void elemental::blas::internal::SymvRowAccumulate
( Shape shape,
  double alpha, const DistMatrix<double,MC,  MR>& A,
                const DistMatrix<double,Star,MC>& x_Star_MC,
                const DistMatrix<double,Star,MR>& x_Star_MR,
                      DistMatrix<double,Star,MC>& z_Star_MC,
                      DistMatrix<double,Star,MR>& z_Star_MR );

#ifndef WITHOUT_COMPLEX
template void elemental::blas::Symv
( Shape shape,
  scomplex alpha, const DistMatrix<scomplex,MC,MR>& A,
                  const DistMatrix<scomplex,MC,MR>& x,
  scomplex beta,        DistMatrix<scomplex,MC,MR>& y );

template void elemental::blas::internal::SymvColAccumulate
( Shape shape,
  scomplex alpha, 
  const DistMatrix<scomplex,MC,MR  >& A,
  const DistMatrix<scomplex,MC,Star>& x_MC_Star,
  const DistMatrix<scomplex,MR,Star>& x_MR_Star,
        DistMatrix<scomplex,MC,Star>& z_MC_Star,
        DistMatrix<scomplex,MR,Star>& z_MR_Star );

template void elemental::blas::internal::SymvRowAccumulate
( Shape shape,
  scomplex alpha, 
  const DistMatrix<scomplex,MC,  MR>& A,
  const DistMatrix<scomplex,Star,MC>& x_Star_MC,
  const DistMatrix<scomplex,Star,MR>& x_Star_MR,
        DistMatrix<scomplex,Star,MC>& z_Star_MC,
        DistMatrix<scomplex,Star,MR>& z_Star_MR );

template void elemental::blas::Symv
( Shape shape,
  dcomplex alpha, const DistMatrix<dcomplex,MC,MR>& A,
                  const DistMatrix<dcomplex,MC,MR>& x,
  dcomplex beta,        DistMatrix<dcomplex,MC,MR>& y );

template void elemental::blas::internal::SymvColAccumulate
( Shape shape,
  dcomplex alpha, 
  const DistMatrix<dcomplex,MC,MR  >& A,
  const DistMatrix<dcomplex,MC,Star>& x_MC_Star,
  const DistMatrix<dcomplex,MR,Star>& x_MR_Star,
        DistMatrix<dcomplex,MC,Star>& z_MC_Star,
        DistMatrix<dcomplex,MR,Star>& z_MR_Star );

template void elemental::blas::internal::SymvRowAccumulate
( Shape shape,
  dcomplex alpha, 
  const DistMatrix<dcomplex,MC,  MR>& A,
  const DistMatrix<dcomplex,Star,MC>& x_Star_MC,
  const DistMatrix<dcomplex,Star,MR>& x_Star_MR,
        DistMatrix<dcomplex,Star,MC>& z_Star_MC,
        DistMatrix<dcomplex,Star,MR>& z_Star_MR );
#endif
