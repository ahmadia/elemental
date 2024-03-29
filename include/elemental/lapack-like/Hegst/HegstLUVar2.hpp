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

template<typename F> 
inline void
internal::HegstLUVar2( DistMatrix<F,MC,MR>& A, const DistMatrix<F,MC,MR>& U )
{
#ifndef RELEASE
    PushCallStack("internal::HegstLUVar2");
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
    DistMatrix<F,VC,  STAR> A01_VC_STAR(g);
    DistMatrix<F,STAR,STAR> A11_STAR_STAR(g);
    DistMatrix<F,STAR,VR  > A12_STAR_VR(g);
    DistMatrix<F,STAR,STAR> U11_STAR_STAR(g);
    DistMatrix<F,STAR,MC  > U12_STAR_MC(g);
    DistMatrix<F,STAR,VR  > U12_STAR_VR(g);
    DistMatrix<F,MR,  STAR> U12Adj_MR_STAR(g);
    DistMatrix<F,VC,  STAR> U12Adj_VC_STAR(g);
    DistMatrix<F,MC,  STAR> X01_MC_STAR(g);
    DistMatrix<F,STAR,STAR> X11_STAR_STAR(g);
    DistMatrix<F,MC,  MR  > Y12(g);
    DistMatrix<F,MC,  MR  > Z12Adj(g);
    DistMatrix<F,MR,  MC  > Z12Adj_MR_MC(g);
    DistMatrix<F,MC,  STAR> Z12Adj_MC_STAR(g);
    DistMatrix<F,MR,  STAR> Z12Adj_MR_STAR(g);

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

        A12_STAR_VR.AlignWith( A12 );
        U12_STAR_MC.AlignWith( A22 );
        U12_STAR_VR.AlignWith( A12 );
        U12Adj_MR_STAR.AlignWith( A22 );
        U12Adj_VC_STAR.AlignWith( A22 );
        X01_MC_STAR.AlignWith( A01 );
        Y12.AlignWith( A12 );
        Z12Adj.AlignWith( A12 );
        Z12Adj_MR_MC.AlignWith( A12 );
        Z12Adj_MC_STAR.AlignWith( A22 );
        Z12Adj_MR_STAR.AlignWith( A22 );
        //--------------------------------------------------------------------//
        // A01 := A01 U11'
        U11_STAR_STAR = U11;
        A01_VC_STAR = A01;
        internal::LocalTrmm
        ( RIGHT, UPPER, ADJOINT, NON_UNIT, (F)1, U11_STAR_STAR, A01_VC_STAR );
        A01 = A01_VC_STAR;

        // A01 := A01 + A02 U12'
        U12Adj_MR_STAR.AdjointFrom( U12 );
        X01_MC_STAR.ResizeTo( A01.Height(), A01.Width() );
        internal::LocalGemm
        ( NORMAL, NORMAL,
          (F)1, A02, U12Adj_MR_STAR, (F)0, X01_MC_STAR );
        A01.SumScatterUpdate( (F)1, X01_MC_STAR );

        // Y12 := U12 A22
        U12Adj_VC_STAR = U12Adj_MR_STAR;
        U12_STAR_MC.AdjointFrom( U12Adj_VC_STAR );
        Z12Adj_MC_STAR.ResizeTo( A12.Width(), A12.Height() );
        Z12Adj_MR_STAR.ResizeTo( A12.Width(), A12.Height() );
        Zero( Z12Adj_MC_STAR );
        Zero( Z12Adj_MR_STAR );
        internal::LocalSymmetricAccumulateRU
        ( ADJOINT, 
          (F)1, A22, U12_STAR_MC, U12Adj_MR_STAR, 
          Z12Adj_MC_STAR, Z12Adj_MR_STAR );
        Z12Adj.SumScatterFrom( Z12Adj_MC_STAR );
        Z12Adj_MR_MC = Z12Adj;
        Z12Adj_MR_MC.SumScatterUpdate( (F)1, Z12Adj_MR_STAR );
        Y12.ResizeTo( A12.Height(), A12.Width() );
        Adjoint( Z12Adj_MR_MC.LockedLocalMatrix(), Y12.LocalMatrix() );

        // A12 := U11 A12
        A12_STAR_VR = A12;
        U11_STAR_STAR = U11;
        internal::LocalTrmm
        ( LEFT, UPPER, NORMAL, NON_UNIT, (F)1, U11_STAR_STAR, A12_STAR_VR );
        A12 = A12_STAR_VR;

        // A12 := A12 + 1/2 Y12
        Axpy( (F)0.5, Y12, A12 );

        // A11 := U11 A11 U11'
        A11_STAR_STAR = A11;
        internal::LocalHegst( LEFT, UPPER, A11_STAR_STAR, U11_STAR_STAR );
        A11 = A11_STAR_STAR;

        // A11 := A11 + (A12 U12' + U12 A12')
        A12_STAR_VR = A12;
        U12_STAR_VR = U12;
        X11_STAR_STAR.ResizeTo( A11.Height(), A11.Width() );
        Her2k
        ( UPPER, NORMAL,
          (F)1, A12_STAR_VR.LocalMatrix(), U12_STAR_VR.LocalMatrix(),
          (F)0, X11_STAR_STAR.LocalMatrix() );
        A11.SumScatterUpdate( (F)1, X11_STAR_STAR );

        // A12 := A12 + 1/2 Y12
        Axpy( (F)0.5, Y12, A12 );
        //--------------------------------------------------------------------//
        A12_STAR_VR.FreeAlignments();
        U12_STAR_MC.FreeAlignments();
        U12_STAR_VR.FreeAlignments();
        U12Adj_MR_STAR.FreeAlignments();
        U12Adj_VC_STAR.FreeAlignments();
        X01_MC_STAR.FreeAlignments();
        Y12.FreeAlignments();
        Z12Adj.FreeAlignments();
        Z12Adj_MR_MC.FreeAlignments(); 
        Z12Adj_MC_STAR.FreeAlignments();
        Z12Adj_MR_STAR.FreeAlignments();

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
