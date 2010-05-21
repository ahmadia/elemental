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
elemental::blas::Hemv
( Shape shape,
  T alpha, const DistMatrix<T,MC,MR>& A,
           const DistMatrix<T,MC,MR>& x,
  T beta,        DistMatrix<T,MC,MR>& y )
{
#ifndef RELEASE
    PushCallStack("blas::Hemv");
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
        msg << "Nonconformal Hemv: " << endl
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
        z_MC_Star.ResizeTo( y.Height(), y.Width() );
        z_MR_Star.ResizeTo( y.Height(), y.Width() );
        z_MC_Star.SetToZero();
        z_MR_Star.SetToZero();
        z.AlignWith( y );
        //--------------------------------------------------------------------//
        x_MC_Star = x;
        x_MR_Star = x_MC_Star;
        blas::internal::HemvColAccumulate
        ( shape, alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );

        y.ReduceScatterUpdate( (T)1, z_MC_Star );
        z_MR_MC.ReduceScatterFrom( z_MR_Star );
        z = z_MR_MC;
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
        DistMatrix<T,MR,MC  > z_MR_MC(grid);
        DistMatrix<T,MC,MR  > z(grid);
        DistMatrix<T,MC,MR  > zTrans(grid);

        // Begin the algoritm
        blas::Scal( beta, y );
        x_MC_Star.ConformWith( A );
        x_MR_Star.ConformWith( A );
        z_MC_Star.AlignWith( A );
        z_MR_Star.AlignWith( A );
        z_MC_Star.ResizeTo( y.Height(), y.Width() );
        z_MR_Star.ResizeTo( y.Height(), y.Width() );
        z_MC_Star.SetToZero();
        z_MR_Star.SetToZero();
        z.AlignWith( y );
        zTrans.AlignWith( y );
        //--------------------------------------------------------------------//
        x_MC_Star = x;
        x_MR_Star = x_MC_Star;
        blas::internal::HemvColAccumulate
        ( shape, alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );

        z_MR_MC.ReduceScatterFrom( z_MR_Star );
        z = z_MR_MC;
        z.ReduceScatterUpdate( (T)1, z_MC_Star );
        blas::Trans( z, zTrans );
        blas::Axpy( (T)1, zTrans, y );
        //--------------------------------------------------------------------//
        x_MC_Star.FreeConstraints();
        x_MR_Star.FreeConstraints();
        z_MC_Star.FreeConstraints();
        z_MR_Star.FreeConstraints();
        z.FreeConstraints();
        zTrans.FreeConstraints();
    }
    else if( y.Width() == 1 )
    {
        // Temporary distributions
        DistMatrix<T,Star,MC  > x_Star_MC(grid);
        DistMatrix<T,Star,MR  > x_Star_MR(grid);
        DistMatrix<T,MC,  Star> z_MC_Star(grid);
        DistMatrix<T,MR,  Star> z_MR_Star(grid);
        DistMatrix<T,MR,  MC  > z_MR_MC(grid);
        DistMatrix<T,MC,  MR  > z(grid);

        // Begin the algoritm
        blas::Scal( beta, y );
        x_Star_MC.ConformWith( A );
        x_Star_MR.ConformWith( A );
        z_MC_Star.AlignWith( A );
        z_MR_Star.AlignWith( A );
        z_MC_Star.ResizeTo( y.Height(), y.Width() );
        z_MR_Star.ResizeTo( y.Height(), y.Width() );
        z_MC_Star.SetToZero();
        z_MR_Star.SetToZero();
        z.AlignWith( y );
        //--------------------------------------------------------------------//
        x_Star_MR = x;
        x_Star_MC = x_Star_MR;
        blas::internal::HemvRowAccumulate
        ( shape, alpha, A, x_Star_MC, x_Star_MR, z_MC_Star, z_MR_Star );

        y.ReduceScatterUpdate( (T)1, z_MC_Star );
        z_MR_MC.ReduceScatterFrom( z_MR_Star );
        z = z_MR_MC;
        blas::Axpy( (T)1, z, y );
        //--------------------------------------------------------------------//
        x_Star_MC.FreeConstraints();
        x_Star_MR.FreeConstraints();
        z_MC_Star.FreeConstraints();
        z_MR_Star.FreeConstraints();
        z.FreeConstraints();
    }
    else
    {
        // Temporary distributions
        DistMatrix<T,Star,MC  > x_Star_MC(grid);
        DistMatrix<T,Star,MR  > x_Star_MR(grid);
        DistMatrix<T,MC,  Star> z_MC_Star(grid);
        DistMatrix<T,MR,  Star> z_MR_Star(grid);
        DistMatrix<T,MR,  MC  > z_MR_MC(grid);
        DistMatrix<T,MC,  MR  > z(grid);
        DistMatrix<T,MC,  MR  > zTrans(grid);

        // Begin the algoritm
        blas::Scal( beta, y );
        x_Star_MC.ConformWith( A );
        x_Star_MR.ConformWith( A );
        z_MC_Star.AlignWith( A );
        z_MR_Star.AlignWith( A );
        z_MC_Star.ResizeTo( y.Height(), y.Width() );
        z_MR_Star.ResizeTo( y.Height(), y.Width() );
        z_MC_Star.SetToZero();
        z_MR_Star.SetToZero();
        z.AlignWith( y );
        zTrans.AlignWith( y );
        //--------------------------------------------------------------------//
        x_Star_MR = x;
        x_Star_MC = x_Star_MR;
        blas::internal::HemvRowAccumulate
        ( shape, alpha, A, x_Star_MC, x_Star_MR, z_MC_Star, z_MR_Star );

        z_MR_MC.ReduceScatterFrom( z_MR_Star );
        z = z_MR_MC;
        z.ReduceScatterUpdate( (T)1, z_MC_Star );
        blas::Trans( z, zTrans );
        blas::Axpy( (T)1, zTrans, y );
        //--------------------------------------------------------------------//
        x_Star_MC.FreeConstraints();
        x_Star_MR.FreeConstraints();
        z_MC_Star.FreeConstraints();
        z_MR_Star.FreeConstraints();
        z.FreeConstraints();
        zTrans.FreeConstraints();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::blas::internal::HemvColAccumulate
( Shape shape,
  T alpha, const DistMatrix<T,MC,MR  >& A,
           const DistMatrix<T,MC,Star>& x_MC_Star,
           const DistMatrix<T,MR,Star>& x_MR_Star,
                 DistMatrix<T,MC,Star>& z_MC_Star,
                 DistMatrix<T,MR,Star>& z_MR_Star )
{
#ifndef RELEASE
    PushCallStack("blas::internal::HemvColAccumulate");
#endif
    if( shape == Lower )
    {
        blas::internal::HemvColAccumulateL
        ( alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );
    }
    else
    {
        blas::internal::HemvColAccumulateU
        ( alpha, A, x_MC_Star, x_MR_Star, z_MC_Star, z_MR_Star );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::blas::internal::HemvRowAccumulate
( Shape shape,
  T alpha, const DistMatrix<T,MC,  MR  >& A,
           const DistMatrix<T,Star,MC  >& x_Star_MC,
           const DistMatrix<T,Star,MR  >& x_Star_MR,
                 DistMatrix<T,MC,  Star>& z_MC_Star,
                 DistMatrix<T,MR,  Star>& z_MR_Star )
{
#ifndef RELEASE
    PushCallStack("blas::internal::HemvRowAccumulate");
#endif
    if( shape == Lower )
    {
        blas::internal::HemvRowAccumulateL
        ( alpha, A, x_Star_MC, x_Star_MR, z_MC_Star, z_MR_Star );
    }
    else
    {
        blas::internal::HemvRowAccumulateU
        ( alpha, A, x_Star_MC, x_Star_MR, z_MC_Star, z_MR_Star );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template void elemental::blas::Hemv
( Shape shape,
  float alpha, const DistMatrix<float,MC,MR>& A,
               const DistMatrix<float,MC,MR>& x,
  float beta,        DistMatrix<float,MC,MR>& y );

template void elemental::blas::Hemv
( Shape shape,
  double alpha, const DistMatrix<double,MC,MR>& A,
                const DistMatrix<double,MC,MR>& x,
  double beta,        DistMatrix<double,MC,MR>& y );

#ifndef WITHOUT_COMPLEX
template void elemental::blas::Hemv
( Shape shape,
  scomplex alpha, const DistMatrix<scomplex,MC,MR>& A,
                  const DistMatrix<scomplex,MC,MR>& x,
  scomplex beta,        DistMatrix<scomplex,MC,MR>& y );

template void elemental::blas::Hemv
( Shape shape,
  dcomplex alpha, const DistMatrix<dcomplex,MC,MR>& A,
                  const DistMatrix<dcomplex,MC,MR>& x,
  dcomplex beta,        DistMatrix<dcomplex,MC,MR>& y );
#endif
