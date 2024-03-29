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

// This routine has only partially been optimized. The ReduceScatter operations
// need to be (conjugate-)transposed in order to play nice with cache.
template<typename F> 
inline void
internal::HegstRUVar2( DistMatrix<F,MC,MR>& A, const DistMatrix<F,MC,MR>& U )
{
#ifndef RELEASE
    PushCallStack("internal::HegstRUVar2");
    if( A.Height() != A.Width() )
        throw std::logic_error("A must be square");
    if( U.Height() != U.Width() )
        throw std::logic_error("Triangular matrices must be square");
    if( A.Height() != U.Height() )
        throw std::logic_error("A and U must be the same size");
#endif
    const Grid& g = A.Grid();

    // Matrix views
    DistMatrix<F,MC,MR>
        ATL(g), ATR(g),  A00(g), A01(g), A02(g),
        ABL(g), ABR(g),  A10(g), A11(g), A12(g),
                         A20(g), A21(g), A22(g);

    DistMatrix<F,MC,MR>
        UTL(g), UTR(g),  U00(g), U01(g), U02(g),
        UBL(g), UBR(g),  U10(g), U11(g), U12(g),
                         U20(g), U21(g), U22(g);

    // Temporary distributions
    DistMatrix<F,MC,  STAR> A01_MC_STAR(g);
    DistMatrix<F,VC,  STAR> A01_VC_STAR(g);
    DistMatrix<F,STAR,STAR> A11_STAR_STAR(g);
    DistMatrix<F,STAR,VR  > A12_STAR_VR(g);
    DistMatrix<F,MC,  STAR> F01_MC_STAR(g);
    DistMatrix<F,MC,  STAR> U01_MC_STAR(g);
    DistMatrix<F,VR,  STAR> U01_VR_STAR(g);
    DistMatrix<F,STAR,MR  > U01Adj_STAR_MR(g);
    DistMatrix<F,STAR,STAR> U11_STAR_STAR(g);
    DistMatrix<F,MC,  MR  > X11(g);
    DistMatrix<F,STAR,MR  > X11_STAR_MR(g);
    DistMatrix<F,MR,  STAR> X12Adj_MR_STAR(g);
    DistMatrix<F,MR,  MC  > X12Adj_MR_MC(g);
    DistMatrix<F,MC,  MR  > Y01(g);
    DistMatrix<F,MR,  MC  > Y01_MR_MC(g);
    DistMatrix<F,MR,  STAR> Y01_MR_STAR(g);

    Matrix<F> X12Local;

    PartitionDownDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    LockedPartitionDownDiagonal
    ( U, UTL, UTR,
         UBL, UBR, 0 );
    while( ATL.Height() < A.Height() )
    {
        RepartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, /**/ A01, A02,
         /*************/ /******************/
               /**/       A10, /**/ A11, A12,
          ABL, /**/ ABR,  A20, /**/ A21, A22 );

        LockedRepartitionDownDiagonal
        ( UTL, /**/ UTR,  U00, /**/ U01, U02,
         /*************/ /******************/
               /**/       U10, /**/ U11, U12,
          UBL, /**/ UBR,  U20, /**/ U21, U22 );

        A01_MC_STAR.AlignWith( U01 );
        Y01.AlignWith( A01 );
        Y01_MR_STAR.AlignWith( A00 );
        U01_MC_STAR.AlignWith( A00 );
        U01_VR_STAR.AlignWith( A00 );
        U01Adj_STAR_MR.AlignWith( A00 );
        X11_STAR_MR.AlignWith( U01 );
        X11.AlignWith( A11 );
        X12Adj_MR_STAR.AlignWith( A02 );
        X12Adj_MR_MC.AlignWith( A12 );
        F01_MC_STAR.AlignWith( A00 );
        //--------------------------------------------------------------------//
        // Y01 := A00 U01
        U01_MC_STAR = U01;
        U01_VR_STAR = U01_MC_STAR;
        U01Adj_STAR_MR.AdjointFrom( U01_VR_STAR );
        Y01_MR_STAR.ResizeTo( A01.Height(), A01.Width() ); 
        F01_MC_STAR.ResizeTo( A01.Height(), A01.Width() );
        Zero( Y01_MR_STAR );
        Zero( F01_MC_STAR );
        internal::LocalSymmetricAccumulateLU
        ( ADJOINT, 
          (F)1, A00, U01_MC_STAR, U01Adj_STAR_MR, F01_MC_STAR, Y01_MR_STAR );
        Y01_MR_MC.SumScatterFrom( Y01_MR_STAR );
        Y01 = Y01_MR_MC;
        Y01.SumScatterUpdate( (F)1, F01_MC_STAR );

        // X11 := U01' A01
        X11_STAR_MR.ResizeTo( A11.Height(), A11.Width() );
        internal::LocalGemm
        ( ADJOINT, NORMAL,
          (F)1, U01_MC_STAR, A01, (F)0, X11_STAR_MR );

        // A01 := A01 - Y01
        Axpy( (F)-1, Y01, A01 );
        A01_MC_STAR = A01;
        
        // A11 := A11 - triu(X11 + A01' U01) = A11 - (U01 A01 + A01' U01)
        internal::LocalGemm
        ( ADJOINT, NORMAL,
          (F)1, A01_MC_STAR, U01, (F)1, X11_STAR_MR );
        X11.SumScatterFrom( X11_STAR_MR );
        MakeTrapezoidal( LEFT, UPPER, 0, X11 );
        Axpy( (F)-1, X11, A11 );

        // A01 := A01 inv(U11)
        U11_STAR_STAR = U11;
        A01_VC_STAR = A01_MC_STAR;
        internal::LocalTrsm
        ( RIGHT, UPPER, NORMAL, NON_UNIT, (F)1, U11_STAR_STAR, A01_VC_STAR );
        A01 = A01_VC_STAR;

        // A11 := inv(U11)' A11 inv(U11)
        A11_STAR_STAR = A11;
        internal::LocalHegst( RIGHT, UPPER, A11_STAR_STAR, U11_STAR_STAR );
        A11 = A11_STAR_STAR;

        // A12 := A12 - A02' U01
        X12Adj_MR_STAR.ResizeTo( A12.Width(), A12.Height() );
        internal::LocalGemm
        ( ADJOINT, NORMAL,
          (F)1, A02, U01_MC_STAR, (F)0, X12Adj_MR_STAR );
        X12Adj_MR_MC.SumScatterFrom( X12Adj_MR_STAR );
        Adjoint( X12Adj_MR_MC.LockedLocalMatrix(), X12Local );
        Axpy( (F)-1, X12Local, A12.LocalMatrix() );

        // A12 := inv(U11)' A12
        A12_STAR_VR = A12;
        internal::LocalTrsm
        ( LEFT, UPPER, ADJOINT, NON_UNIT,
          (F)1, U11_STAR_STAR, A12_STAR_VR );
        A12 = A12_STAR_VR;
        //--------------------------------------------------------------------//
        A01_MC_STAR.FreeAlignments();
        F01_MC_STAR.FreeAlignments();
        U01_MC_STAR.FreeAlignments();
        U01_VR_STAR.FreeAlignments();
        U01Adj_STAR_MR.FreeAlignments();
        X11.FreeAlignments();
        X11_STAR_MR.FreeAlignments();
        X12Adj_MR_STAR.FreeAlignments();
        X12Adj_MR_MC.FreeAlignments();
        Y01.FreeAlignments();
        Y01_MR_STAR.FreeAlignments();

        SlidePartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, A01, /**/ A02,
               /**/       A10, A11, /**/ A12,
         /*************/ /******************/
          ABL, /**/ ABR,  A20, A21, /**/ A22 );

        SlideLockedPartitionDownDiagonal
        ( UTL, /**/ UTR,  U00, U01, /**/ U02,
               /**/       U10, U11, /**/ U12,
         /*************/ /******************/
          UBL, /**/ UBR,  U20, U21, /**/ U22 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

} // namespace elem
