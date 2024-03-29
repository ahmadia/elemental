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

template<typename R> 
inline void
internal::HermitianTridiagUSquare( DistMatrix<R,MC,MR>& A )
{
#ifndef RELEASE
    PushCallStack("internal::HermitianTridiagUSquare");
    if( A.Height() != A.Width() )
        throw std::logic_error("A must be square");
#endif
    const Grid& g = A.Grid();

    if( g.InGrid() )
    {
        // Matrix views 
        DistMatrix<R,MC,MR> 
            ATL(g), ATR(g),  A00(g), A01(g), A02(g), 
            ABL(g), ABR(g),  A10(g), A11(g), A12(g),
                             A20(g), A21(g), A22(g);

        // Temporary distributions
        DistMatrix<R,MC,  MR  > WPan(g);
        DistMatrix<R,STAR,STAR> A11_STAR_STAR(g);
        DistMatrix<R,MC,  STAR> APan_MC_STAR(g),  A01_MC_STAR(g),
                                                  A11_MC_STAR(g);
        DistMatrix<R,MR,  STAR> APan_MR_STAR(g),  A01_MR_STAR(g),
                                                  A11_MR_STAR(g);
        DistMatrix<R,MC,  STAR> WPan_MC_STAR(g),  W01_MC_STAR(g),
                                                  W11_MC_STAR(g);
        DistMatrix<R,MR,  STAR> WPan_MR_STAR(g),  W01_MR_STAR(g),
                                                  W11_MR_STAR(g);

        PartitionUpDiagonal
        ( A, ATL, ATR,
             ABL, ABR, 0 );
        while( ABR.Height() < A.Height() )
        {
            RepartitionUpDiagonal
            ( ATL, /**/ ATR,  A00, A01, /**/ A02,
                   /**/       A10, A11, /**/ A12,
             /*************/ /******************/
              ABL, /**/ ABR,  A20, A21, /**/ A22 );

            if( A00.Height() > 0 )
            {
                WPan.AlignWith( A01 );
                APan_MC_STAR.AlignWith( A00 );
                WPan_MC_STAR.AlignWith( A00 );
                APan_MR_STAR.AlignWith( A00 );
                WPan_MR_STAR.AlignWith( A00 );
                //------------------------------------------------------------//
                WPan.ResizeTo( ATL.Height(), A11.Width() );
                APan_MC_STAR.ResizeTo( ATL.Height(), A11.Width() );
                WPan_MC_STAR.ResizeTo( ATL.Height(), A11.Width() );
                APan_MR_STAR.ResizeTo( ATL.Height(), A11.Width() );
                WPan_MR_STAR.ResizeTo( ATL.Height(), A11.Width() );

                internal::HermitianPanelTridiagUSquare
                ( ATL, WPan, 
                  APan_MC_STAR, APan_MR_STAR, WPan_MC_STAR, WPan_MR_STAR );
                
                PartitionUp
                ( APan_MC_STAR, A01_MC_STAR,
                                A11_MC_STAR, A11.Height() );
                PartitionUp
                ( APan_MR_STAR, A01_MR_STAR,
                                A11_MR_STAR, A11.Height() );
                PartitionUp
                ( WPan_MC_STAR, W01_MC_STAR,
                                W11_MC_STAR, A11.Height() );
                PartitionUp
                ( WPan_MR_STAR, W01_MR_STAR,
                                W11_MR_STAR, A11.Height() );

                internal::LocalTrr2k
                ( UPPER, TRANSPOSE, TRANSPOSE,
                  (R)-1, A01_MC_STAR, W01_MR_STAR,
                         W01_MC_STAR, A01_MR_STAR,
                  (R)1,  A00 );
                //------------------------------------------------------------//
                WPan_MR_STAR.FreeAlignments();
                APan_MR_STAR.FreeAlignments();
                WPan_MC_STAR.FreeAlignments();
                APan_MC_STAR.FreeAlignments();
                WPan.FreeAlignments();
            }
            else
            {
                A11_STAR_STAR = A11;
                HermitianTridiag( UPPER, A11_STAR_STAR.LocalMatrix() );
                A11 = A11_STAR_STAR;
            }

            SlidePartitionUpDiagonal
            ( ATL, /**/ ATR,  A00, /**/ A01, A02,
             /*************/ /******************/
                   /**/       A10, /**/ A11, A12,
              ABL, /**/ ABR,  A20, /**/ A21, A22 );
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R> 
inline void
internal::HermitianTridiagUSquare
( DistMatrix<Complex<R>,MC,  MR  >& A,
  DistMatrix<Complex<R>,STAR,STAR>& t )
{
#ifndef RELEASE
    PushCallStack("internal::HermitianTridiagUSquare");
    if( A.Grid() != t.Grid() )
        throw std::logic_error("{A,t} must be distributed over the same grid");
#endif
    const Grid& g = A.Grid();
#ifndef RELEASE
    if( A.Height() != A.Width() )
        throw std::logic_error("A must be square");
    if( t.Viewing() )
        throw std::logic_error("t must not be a view");
#endif
    typedef Complex<R> C;

    DistMatrix<C,MD,STAR> tDiag(g);
    tDiag.AlignWithDiagonal( A, 1 );
    tDiag.ResizeTo( A.Height()-1, 1 );

    if( g.InGrid() )
    {
        // Matrix views 
        DistMatrix<C,MC,MR> 
            ATL(g), ATR(g),  A00(g), A01(g), A02(g), 
            ABL(g), ABR(g),  A10(g), A11(g), A12(g),
                             A20(g), A21(g), A22(g);
        DistMatrix<C,MD,STAR> tT(g),  t0(g), 
                              tB(g),  t1(g),
                                      t2(g);

        // Temporary distributions
        DistMatrix<C,MC,  MR  > WPan(g);
        DistMatrix<C,STAR,STAR> t1_STAR_STAR(g);
        DistMatrix<C,STAR,STAR> A11_STAR_STAR(g);
        DistMatrix<C,MC,  STAR> APan_MC_STAR(g),  A01_MC_STAR(g),
                                                  A11_MC_STAR(g);
        DistMatrix<C,MR,  STAR> APan_MR_STAR(g),  A01_MR_STAR(g),
                                                  A11_MR_STAR(g);
        DistMatrix<C,MC,  STAR> WPan_MC_STAR(g),  W01_MC_STAR(g),
                                                  W11_MC_STAR(g);
        DistMatrix<C,MR,  STAR> WPan_MR_STAR(g),  W01_MR_STAR(g),
                                                  W11_MR_STAR(g);

        PartitionUpDiagonal
        ( A, ATL, ATR,
             ABL, ABR, 0 );
        PartitionUp
        ( tDiag, tT,
                 tB, 0 );
        while( ABR.Height() < A.Height() )
        {
            RepartitionUpDiagonal
            ( ATL, /**/ ATR,  A00, A01, /**/ A02,
                   /**/       A10, A11, /**/ A12,
             /*************/ /******************/
              ABL, /**/ ABR,  A20, A21, /**/ A22 );

            RepartitionUp
            ( tT,  t0,
                   t1,
             /**/ /**/
              tB,  t2 );
            
            if( A00.Height() > 0 )
            {
                WPan.AlignWith( A01 );
                APan_MC_STAR.AlignWith( A00 );
                WPan_MC_STAR.AlignWith( A00 );
                APan_MR_STAR.AlignWith( A00 );
                WPan_MR_STAR.AlignWith( A00 );
                //------------------------------------------------------------//
                WPan.ResizeTo( ATL.Height(), A11.Width() );
                APan_MC_STAR.ResizeTo( ATL.Height(), A11.Width() );
                WPan_MC_STAR.ResizeTo( ATL.Height(), A11.Width() );
                APan_MR_STAR.ResizeTo( ATL.Height(), A11.Width() );
                WPan_MR_STAR.ResizeTo( ATL.Height(), A11.Width() );

                internal::HermitianPanelTridiagUSquare
                ( ATL, WPan, t1,
                  APan_MC_STAR, APan_MR_STAR, WPan_MC_STAR, WPan_MR_STAR );

                PartitionUp
                ( APan_MC_STAR, A01_MC_STAR,
                                A11_MC_STAR, A11.Height() );
                PartitionUp
                ( APan_MR_STAR, A01_MR_STAR,
                                A11_MR_STAR, A11.Height() );
                PartitionUp
                ( WPan_MC_STAR, W01_MC_STAR,
                                W11_MC_STAR, A11.Height() );
                PartitionUp
                ( WPan_MR_STAR, W01_MR_STAR,
                                W11_MR_STAR, A11.Height() );

                internal::LocalTrr2k
                ( UPPER, ADJOINT, ADJOINT,
                  (C)-1, A01_MC_STAR, W01_MR_STAR,
                         W01_MC_STAR, A01_MR_STAR,
                  (C)1,  A00 );
                //------------------------------------------------------------//
                WPan_MR_STAR.FreeAlignments();
                APan_MR_STAR.FreeAlignments();
                WPan_MC_STAR.FreeAlignments();
                APan_MC_STAR.FreeAlignments();
                WPan.FreeAlignments();
            }
            else
            {
                A11_STAR_STAR = A11;
                t1_STAR_STAR.ResizeTo( t1.Height(), 1 );

                HermitianTridiag
                ( UPPER, A11_STAR_STAR.LocalMatrix(), 
                  t1_STAR_STAR.LocalMatrix() );

                A11 = A11_STAR_STAR;
                t1 = t1_STAR_STAR;
            }

            SlidePartitionUp
            ( tT,  t0,
             /**/ /**/
                   t1,
              tB,  t2 );

            SlidePartitionUpDiagonal
            ( ATL, /**/ ATR,  A00, /**/ A01, A02,
             /*************/ /******************/
                   /**/       A10, /**/ A11, A12,
              ABL, /**/ ABR,  A20, /**/ A21, A22 );
        }
    }
    // Redistribute from matrix-diagonal form to fully replicated
    t = tDiag;
#ifndef RELEASE
    PopCallStack();
#endif
}

} // namespace elem
