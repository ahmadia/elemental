Level 3
=======

The prototypes for the following routines can be found at          
`include/elemental/blas-like.hpp <../../../../include/elemental/blas-like.hpp>`_, while the
implementations are in `include/elemental/blas-like/level3/ <../../../../include/elemental/b
asic/level3/>`_.

Gemm
----
General matrix-matrix multiplication: updates
:math:`C := \alpha \mbox{op}_A(A) \mbox{op}_B(B) + \beta C`,
where :math:`\mbox{op}_A(M)` and :math:`\mbox{op}_B(M)` can each be chosen from 
:math:`M`, :math:`M^T`, and :math:`M^H`.

.. cpp:function:: void Gemm( Orientation orientationOfA, Orientation orientationOfB, T alpha, const Matrix<T>& A, const Matrix<T>& B, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Gemm( Orientation orientationOfA, Orientation orientationOfB, T alpha, const DistMatrix<T,MC,MR>& A, const DistMatrix<T,MC,MR>& B, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Hemm
----
Hermitian matrix-matrix multiplication: updates
:math:`C := \alpha A B + \beta C`, or 
:math:`C := \alpha B A + \beta C`, depending upon whether `side` is set to 
``LEFT`` or ``RIGHT``, respectively. In both of these types of updates, 
:math:`A` is implicitly Hermitian and only the triangle specified by `uplo` is 
accessed.

.. cpp:function:: void Hemm( LeftOrRight side, UpperOrLower uplo, T alpha, const Matrix<T>& A, const Matrix<T>& B, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Hemm( LeftOrRight side, UpperOrLower uplo, T alpha, const DistMatrix<T,MC,MR>& A, const DistMatrix<T,MC,MR>& B, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Her2k
-----
Hermitian rank-2K update: updates
:math:`C := \alpha (A B^H + B A^H) + \beta C`, or 
:math:`C := \alpha (A^H B + B^H A) + \beta C`, depending upon whether 
`orientation` is set to ``NORMAL`` or ``ADJOINT``, respectively. Only the 
triangle of :math:`C` specified by the `uplo` parameter is modified.

.. cpp:function:: void Her2k( UpperOrLower uplo, Orientation orientation, T alpha, const Matrix<T>& A, const Matrix<T>& B, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Her2k( UpperOrLower uplo, Orientation orientation, T alpha, const DistMatrix<T,MC,MR>& A, const DistMatrix<T,MC,MR>& B, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Please see ``SetLocalTrr2kBlocksize<T>( int blocksize )`` 
and ``int LocalTrr2kBlocksize<T>()`` in the 
*Tuning parameters* section for information on tuning the distributed 
``Her2k``.

Herk
----
Hermitian rank-K update: updates
:math:`C := \alpha A A^H + \beta C`, or 
:math:`C := \alpha A^H A + \beta C`, depending upon whether `orientation` is
set to ``NORMAL`` or ``ADJOINT``, respectively. Only the triangle of :math:`C` 
specified by the `uplo` parameter is modified.

.. cpp:function:: void Herk( UpperOrLower uplo, Orientation orientation, T alpha, const Matrix<T>& A, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Herk( UpperOrLower uplo, Orientation orientation, T alpha, const DistMatrix<T,MC,MR>& A, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Please see ``SetLocalTrrkBlocksize<T>( int blocksize )`` 
and ``int LocalTrrkBlocksize<T>()`` in the *Tuning parameters*
section for information on tuning the distributed ``Herk``.

Hetrmm
------
.. note:: 

   This routine directly corresponds with the LAPACK routines ?lauum, but it 
   only involves matrix-matrix multiplication, so it is lumped in with the 
   BLAS-like routines in Elemental.

Hermitian triangular matrix-matrix multiply: performs 
:math:`L := L^H L` or :math:`U := U U^H`, depending upon the choice of the 
`uplo` parameter. 

.. cpp:function:: void Hetrmm( UpperOrLower uplo, Matrix<T>& A )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Hetrmm( UpperOrLower uplo, DistMatrix<T,MC,MR>& A )

   The distributed implementation (templated over the datatype).

Symm
----
Symmetric matrix-matrix multiplication: updates
:math:`C := \alpha A B + \beta C`, or 
:math:`C := \alpha B A + \beta C`, depending upon whether `side` is set to 
``LEFT`` or ``RIGHT``, respectively. In both of these types of updates, 
:math:`A` is implicitly symmetric and only the triangle specified by `uplo` 
is accessed.

.. cpp:function:: void Symm( LeftOrRight side, UpperOrLower uplo, T alpha, const Matrix<T>& A, const Matrix<T>& B, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Symm( LeftOrRight side, UpperOrLower uplo, T alpha, const DistMatrix<T,MC,MR>& A, const DistMatrix<T,MC,MR>& B, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Syr2k
-----
Symmetric rank-2K update: updates
:math:`C := \alpha (A B^T + B A^T) + \beta C`, or 
:math:`C := \alpha (A^T B + B^T A) + \beta C`, depending upon whether 
`orientation` is set to ``NORMAL`` or ``TRANSPOSE``, respectively. Only the 
triangle of :math:`C` specified by the `uplo` parameter is modified.

.. cpp:function:: void Syr2k( UpperOrLower uplo, Orientation orientation, T alpha, const Matrix<T>& A, const Matrix<T>& B, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Syr2k( UpperOrLower uplo, Orientation orientation, T alpha, const DistMatrix<T,MC,MR>& A, const DistMatrix<T,MC,MR>& B, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Please see ``SetLocalTrr2kBlocksize<T>( int blocksize )`` 
and ``int LocalTrr2kBlocksize<T>()`` in the 
*Tuning parameters* section for information on tuning the distributed 
``Syr2k``.

Syrk
----
Symmetric rank-K update: updates
:math:`C := \alpha A A^T + \beta C`, or 
:math:`C := \alpha A^T A + \beta C`, depending upon whether `orientation` is
set to ``NORMAL`` or ``TRANSPOSE``, respectively. Only the triangle of :math:`C`
specified by the `uplo` parameter is modified.

.. cpp:function:: void Syrk( UpperOrLower uplo, Orientation orientation, T alpha, const Matrix<T>& A, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Syrk( UpperOrLower uplo, Orientation orientation, T alpha, const DistMatrix<T,MC,MR>& A, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Please see ``SetLocalTrrkBlocksize<T>( int blocksize )`` 
and ``int LocalTrrkBlocksize<T>()`` in the *Tuning parameters*
section for information on tuning the distributed ``Syrk``.

Trmm
----
Triangular matrix-matrix multiplication: performs
:math:`C := \alpha \mbox{op}(A) B`, or 
:math:`C := \alpha B \mbox{op}(A)`, depending upon whether `side` was chosen
to be ``LEFT`` or ``RIGHT``, respectively. Whether :math:`A` is treated as 
lower or upper triangular is determined by `uplo`, and :math:`\mbox{op}(A)` 
can be any of :math:`A`, :math:`A^T`, and :math:`A^H` (and `diag` determines
whether :math:`A` is treated as unit-diagonal or not).

.. cpp:function:: void Trmm( LeftOrRight side, UpperOrLower uplo, Orientation orientation, UnitOrNonUnit diag, T alpha, const Matrix<T>& A, Matrix<T>& B )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Trmm( LeftOrRight side, UpperOrLower uplo, Orientation orientation, UnitOrNonUnit diag, T alpha, const DistMatrix<T,MC,MR>& A, DistMatrix<T,MC,MR>& B )

   The distributed implementation (templated over the datatype).

Trr2k
-----
Triangular rank-2k update: performs 
:math:`E := \alpha ( \mbox{op}(A) \mbox{op}(B) + \mbox{op}(C) \mbox{op}(D) ) + \beta E`,
where only the triangle of `E` specified by `uplo` is modified, and 
:math:`\mbox{op}(X)` is determined by `orientationOfX`, for each 
:math:`X \in \left\{A,B,C,D\right\}`.

.. note::

   There is no corresponding BLAS routine, but it is a natural generalization
   of "symmetric" and "Hermitian" updates.

.. cpp:function:: void Trr2k( UpperOrLower uplo, Orientation orientationOfA, Orientation orientationOfB, Orientation orientationOfC, Orientation orientationOfD, T alpha, const Matrix<T>& A, const Matrix<T>& B, const Matrix<T>& C, const Matrix<T>& D, T beta, Matrix<T>& E )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Trr2k( UpperOrLower uplo, Orientation orientationOfA, Orientation orientationOfB, Orientation orientationOfC, Orientation orientationOfD, T alpha, const DistMatrix<T,MC,MR>& A, const DistMatrix<T,MC,MR>& B, const DistMatrix<T,MC,MR>& C, const DistMatrix<T,MC,MR>& D, T beta, DistMatrix<T,MC,MR>& E )

   The distributed implementation (templated over the datatype).

Trrk
----
Triangular rank-k update: performs 
:math:`C := \alpha \mbox{op}(A) \mbox{op}(B) + \beta C`, where only the 
triangle of `C` specified by `uplo` is modified, and :math:`\mbox{op}(A)` and 
:math:`\mbox{op}(B)` are determined by `orientationOfA` and `orientationOfB`, 
respectively.

.. note::

   There is no corresponding BLAS routine, but this type of update is frequently
   encountered, even in serial. For instance, the symmetric rank-k update 
   performed during an LDL factorization is symmetric but one of the 
   two update matrices is scaled by D.

.. cpp:function:: void Trrk( UpperOrLower uplo, Orientation orientationOfA, Orientation orientationOfB, T alpha, const Matrix<T>& A, const Matrix<T>& B, T beta, Matrix<T>& C )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Trrk( UpperOrLower uplo, Orientation orientationOfA, Orientation orientationOfB, T alpha, const DistMatrix<T,MC,MR>& A, const DistMatrix<T,MC,MR>& B, T beta, DistMatrix<T,MC,MR>& C )

   The distributed implementation (templated over the datatype).

Trsm
----
Triangular solve with multiple right-hand sides: performs
:math:`C := \alpha \mbox{op}(A)^{-1} B`, or 
:math:`C := \alpha B \mbox{op}(A)^{-1}`, depending upon whether `side` was 
chosen to be ``LEFT`` or ``RIGHT``, respectively. Whether :math:`A` is treated 
as lower or upper triangular is determined by `uplo`, and :math:`\mbox{op}(A)` 
can be any of :math:`A`, :math:`A^T`, and :math:`A^H` (and `diag` determines
whether :math:`A` is treated as unit-diagonal or not).

.. cpp:function:: void Trsm( LeftOrRight side, UpperOrLower uplo, Orientation orientation, UnitOrNonUnit diag, T alpha, const Matrix<T>& A, Matrix<T>& B )

   The serial implementation (templated over the datatype).

.. cpp:function:: void Trsm( LeftOrRight side, UpperOrLower uplo, Orientation orientation, UnitOrNonUnit diag, T alpha, const DistMatrix<T,MC,MR>& A, DistMatrix<T,MC,MR>& B )

   The distributed implementation (templated over the datatype).
