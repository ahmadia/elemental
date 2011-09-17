/*
   Copyright (c) 2009-2011, Jack Poulson
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

// Left Lower (Conjugate)Transpose (Non)Unit Trsm
//   X := tril(L)^-T,
//   X := tril(L)^-H,
//   X := trilu(L)^-T, or
//   X := trilu(L)^-H

// width(X) >> p
template<typename F>
inline void
elemental::basic::internal::TrsmLLTLarge
( Orientation orientation, 
  Diagonal diagonal,
  F alpha, 
  const DistMatrix<F,MC,MR>& L,
        DistMatrix<F,MC,MR>& X,
  bool checkIfSingular )
{
#ifndef RELEASE
    PushCallStack("basic::internal::TrsmLLTLarge");
    if( L.Grid() != X.Grid() )
        throw std::logic_error
        ("L and X must be distributed over the same grid");
    if( orientation == NORMAL )
        throw std::logic_error("TrsmLLT expects a (Conjugate)Transpose option");
    if( L.Height() != L.Width() || L.Height() != X.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal TrsmLLT: \n"
            << "  L ~ " << L.Height() << " x " << L.Width() << "\n"
            << "  X ~ " << X.Height() << " x " << X.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    const Grid& g = L.Grid();

    // Matrix views
    DistMatrix<F,MC,MR> 
        LTL(g), LTR(g),  L00(g), L01(g), L02(g),
        LBL(g), LBR(g),  L10(g), L11(g), L12(g),
                         L20(g), L21(g), L22(g);

    DistMatrix<F,MC,MR> XT(g),  X0(g),
                        XB(g),  X1(g),
                                X2(g);

    // Temporary distributions
    DistMatrix<F,STAR,MC  > L10_STAR_MC(g);
    DistMatrix<F,STAR,STAR> L11_STAR_STAR(g);
    DistMatrix<F,STAR,MR  > X1_STAR_MR(g);
    DistMatrix<F,STAR,VR  > X1_STAR_VR(g);

    // Start the algorithm
    basic::Scal( alpha, X );
    LockedPartitionUpDiagonal
    ( L, LTL, LTR,
         LBL, LBR, 0 );
    PartitionUp
    ( X, XT,
         XB, 0 );
    while( XT.Height() > 0 )
    {
        LockedRepartitionUpDiagonal
        ( LTL, /**/ LTR,  L00, L01, /**/ L02,
               /**/       L10, L11, /**/ L12,
         /*************/ /******************/
          LBL, /**/ LBR,  L20, L21, /**/ L22 );

        RepartitionUp
        ( XT,  X0,
               X1,
         /**/ /**/
          XB,  X2 ); 

        L10_STAR_MC.AlignWith( X0 );
        X1_STAR_MR.AlignWith( X0 );
        //--------------------------------------------------------------------//
        L11_STAR_STAR = L11; // L11[* ,* ] <- L11[MC,MR]
        X1_STAR_VR    = X1;  // X1[* ,VR] <- X1[MC,MR]

        // X1[* ,VR] := L11^-[T/H][* ,* ] X1[* ,VR]
        basic::internal::LocalTrsm
        ( LEFT, LOWER, orientation, diagonal, (F)1, L11_STAR_STAR, X1_STAR_VR,
          checkIfSingular );

        X1_STAR_MR  = X1_STAR_VR; // X1[* ,MR] <- X1[* ,VR]
        X1          = X1_STAR_MR; // X1[MC,MR] <- X1[* ,MR]
        L10_STAR_MC = L10;        // L10[* ,MC] <- L10[MC,MR]

        // X0[MC,MR] -= (L10[* ,MC])^(T/H) X1[* ,MR]
        //            = L10^[T/H][MC,* ] X1[* ,MR]
        basic::internal::LocalGemm
        ( orientation, NORMAL, (F)-1, L10_STAR_MC, X1_STAR_MR, (F)1, X0 );
        //--------------------------------------------------------------------//
        L10_STAR_MC.FreeAlignments();
        X1_STAR_MR.FreeAlignments();

        SlideLockedPartitionUpDiagonal
        ( LTL, /**/ LTR,  L00, /**/ L01, L02,
         /*************/ /******************/
               /**/       L10, /**/ L11, L12,
          LBL, /**/ LBR,  L20, /**/ L21, L22 );

        SlidePartitionUp
        ( XT,  X0,
         /**/ /**/
               X1,
          XB,  X2 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

// width(X) ~= p
template<typename F>
inline void
elemental::basic::internal::TrsmLLTMedium
( Orientation orientation, 
  Diagonal diagonal,
  F alpha, 
  const DistMatrix<F,MC,MR>& L,
        DistMatrix<F,MC,MR>& X,
  bool checkIfSingular )
{
#ifndef RELEASE
    PushCallStack("basic::internal::TrsmLLTMedium");
    if( L.Grid() != X.Grid() )
        throw std::logic_error
        ("L and X must be distributed over the same grid");
    if( orientation == NORMAL )
        throw std::logic_error("TrsmLLT expects a (Conjugate)Transpose option");
    if( L.Height() != L.Width() || L.Height() != X.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal TrsmLLT: \n"
            << "  L ~ " << L.Height() << " x " << L.Width() << "\n"
            << "  X ~ " << X.Height() << " x " << X.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    const Grid& g = L.Grid();

    // Matrix views
    DistMatrix<F,MC,MR> 
        LTL(g), LTR(g),  L00(g), L01(g), L02(g),
        LBL(g), LBR(g),  L10(g), L11(g), L12(g),
                         L20(g), L21(g), L22(g);

    DistMatrix<F,MC,MR> XT(g),  X0(g),
                        XB(g),  X1(g),
                                X2(g);

    // Temporary distributions
    DistMatrix<F,STAR,MC  > L10_STAR_MC(g);
    DistMatrix<F,STAR,STAR> L11_STAR_STAR(g);
    DistMatrix<F,MR,  STAR> X1AdjOrTrans_MR_STAR(g);

    // Start the algorithm
    basic::Scal( alpha, X );
    LockedPartitionUpDiagonal
    ( L, LTL, LTR,
         LBL, LBR, 0 );
    PartitionUp
    ( X, XT,
         XB, 0 );
    while( XT.Height() > 0 )
    {
        LockedRepartitionUpDiagonal
        ( LTL, /**/ LTR,  L00, L01, /**/ L02,
               /**/       L10, L11, /**/ L12,
         /*************/ /******************/
          LBL, /**/ LBR,  L20, L21, /**/ L22 );

        RepartitionUp
        ( XT,  X0,
               X1,
         /**/ /**/
          XB,  X2 ); 

        L10_STAR_MC.AlignWith( X0 );
        X1AdjOrTrans_MR_STAR.AlignWith( X0 );
        //--------------------------------------------------------------------//
        L11_STAR_STAR = L11; // L11[* ,* ] <- L11[MC,MR]
        // X1[* ,MR] <- X1[MC,MR]
        if( orientation == TRANSPOSE )
            X1AdjOrTrans_MR_STAR.TransposeFrom( X1 );
        else
            X1AdjOrTrans_MR_STAR.AdjointFrom( X1 );

        // X1[* ,MR] := L11^-[T/H][* ,* ] X1[* ,MR]
        // X1^[T/H][MR,* ] := X1^[T/H][MR,* ] L11^-1[* ,* ]
        basic::internal::LocalTrsm
        ( RIGHT, LOWER, NORMAL, diagonal, 
          (F)1, L11_STAR_STAR, X1AdjOrTrans_MR_STAR, checkIfSingular );

        if( orientation == TRANSPOSE )
            X1.TransposeFrom( X1AdjOrTrans_MR_STAR );
        else
            X1.AdjointFrom( X1AdjOrTrans_MR_STAR );
        L10_STAR_MC = L10; // L10[* ,MC] <- L10[MC,MR]

        // X0[MC,MR] -= (L10[* ,MC])^[T/H] X1[* ,MR]
        //            = L10^[T/H][MC,* ] X1[* ,MR]
        basic::internal::LocalGemm
        ( orientation, orientation, 
          (F)-1, L10_STAR_MC, X1AdjOrTrans_MR_STAR, (F)1, X0 );
        //--------------------------------------------------------------------//
        L10_STAR_MC.FreeAlignments();
        X1AdjOrTrans_MR_STAR.FreeAlignments();

        SlideLockedPartitionUpDiagonal
        ( LTL, /**/ LTR,  L00, /**/ L01, L02,
         /*************/ /******************/
               /**/       L10, /**/ L11, L12,
          LBL, /**/ LBR,  L20, /**/ L21, L22 );

        SlidePartitionUp
        ( XT,  X0,
         /**/ /**/
               X1,
          XB,  X2 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

// width(X) << p
template<typename F>
inline void
elemental::basic::internal::TrsmLLTSmall
( Orientation orientation, 
  Diagonal diagonal,
  F alpha, 
  const DistMatrix<F,STAR,VR  >& L,
        DistMatrix<F,VR,  STAR>& X,
  bool checkIfSingular )
{
#ifndef RELEASE
    PushCallStack("basic::internal::TrsmLLTSmall");
    if( L.Grid() != X.Grid() )
        throw std::logic_error
        ("L and X must be distributed over the same grid");
    if( orientation == NORMAL )
        throw std::logic_error("TrsmLLT expects a (Conjugate)Transpose option");
    if( L.Height() != L.Width() || L.Height() != X.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal TrsmLLT: \n"
            << "  L ~ " << L.Height() << " x " << L.Width() << "\n"
            << "  X ~ " << X.Height() << " x " << X.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( L.RowAlignment() != X.ColAlignment() )
        throw std::logic_error("L and X must be aligned");
#endif
    const Grid& g = L.Grid();

    // Matrix views
    DistMatrix<F,STAR,VR> 
        LTL(g), LTR(g),  L00(g), L01(g), L02(g),
        LBL(g), LBR(g),  L10(g), L11(g), L12(g),
                         L20(g), L21(g), L22(g);

    DistMatrix<F,VR,STAR> XT(g),  X0(g),
                          XB(g),  X1(g),
                                  X2(g);

    // Temporary distributions
    DistMatrix<F,STAR,STAR> L11_STAR_STAR(g);
    DistMatrix<F,STAR,STAR> X1_STAR_STAR(g);

    // Start the algorithm
    basic::Scal( alpha, X );
    LockedPartitionUpDiagonal
    ( L, LTL, LTR,
         LBL, LBR, 0 );
    PartitionUp
    ( X, XT,
         XB, 0 );
    while( XT.Height() > 0 )
    {
        LockedRepartitionUpDiagonal
        ( LTL, /**/ LTR,  L00, L01, /**/ L02,
               /**/       L10, L11, /**/ L12,
         /*************/ /******************/
          LBL, /**/ LBR,  L20, L21, /**/ L22 );

        RepartitionUp
        ( XT,  X0,
               X1,
         /**/ /**/
          XB,  X2 ); 

        //--------------------------------------------------------------------//
        L11_STAR_STAR = L11; // L11[* ,* ] <- L11[* ,VR]
        X1_STAR_STAR = X1;   // X1[* ,* ] <- X1[VR,* ]

        // X1[* ,* ] := L11^-[T/H][* ,* ] X1[* ,* ]
        basic::internal::LocalTrsm
        ( LEFT, LOWER, orientation, diagonal,
          (F)1, L11_STAR_STAR, X1_STAR_STAR, checkIfSingular );

        X1 = X1_STAR_STAR;

        // X0[VR,* ] -= L10[* ,VR]^(T/H) X1[* ,* ]
        basic::internal::LocalGemm
        ( orientation, NORMAL,
          (F)-1, L10, X1_STAR_STAR, (F)1, X0 );
        //--------------------------------------------------------------------//

        SlideLockedPartitionUpDiagonal
        ( LTL, /**/ LTR,  L00, /**/ L01, L02,
         /*************/ /******************/
               /**/       L10, /**/ L11, L12,
          LBL, /**/ LBR,  L20, /**/ L21, L22 );

        SlidePartitionUp
        ( XT,  X0,
         /**/ /**/
               X1,
          XB,  X2 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}