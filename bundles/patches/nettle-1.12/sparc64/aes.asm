! -*- mode: asm; asm-comment-char: ?!; -*-  
! nettle, low-level cryptographics library
! 
! Copyright (C) 2002 Niels Möller
!  
! The nettle library is free software; you can redistribute it and/or modify
! it under the terms of the GNU Lesser General Public License as published by
! the Free Software Foundation; either version 2.1 of the License, or (at your
! option) any later version.
! 
! The nettle library is distributed in the hope that it will be useful, but
! WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
! or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
! License for more details.
! 
! You should have received a copy of the GNU Lesser General Public License
! along with the nettle library; see the file COPYING.LIB.  If not, write to
! the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
! MA 02111-1307, USA.

! FIXME: For improved ultra sparc performance, we should avoid ALU
! instructions that use the result of an immediately preceeding ALU
! instruction. It is also a good idea to have a greater distance than
! one instruction between a load and use of its value, as that reduces
! the penalty for cache misses. Such instruction sequences are marked
! with !U comments.

! NOTE: Some of the %g registers are reserved for operating system etc
! (see gcc/config/sparc.h). The only %g registers that seems safe to
! use are %g1-%g3.
	
	! Used registers:	%l0,1,2,3,4,5,6,7
	!			%i0,1,2,3,4 (%i6=%fp, %i7 = return)
	!			%o0,1,2,3,4 (%o6=%sp)
	!			
	
	.file	"aes.asm"

! Arguments
define(ctx, %i0)
define(T, %i1)
define(length, %i2)
define(dst, %i3)
define(src, %i4)

! Loop invariants
define(wtxt, %l0)
define(tmp, %l1)
define(diff, %l2)
define(nrounds, %l3)

! Further loop invariants
define(T0, %l4)
define(T1, %l5)
define(T2, %l6)
define(T3, %l7)
	
! Teporaries
define(t0, %o0)
define(t1, %o1)
define(t2, %o2)

! Loop variables
define(round, %o3)
define(key, %o4)

C IDX1 cointains the permutation values * 4 + 2
define(IDX1, <T + AES_SIDX1 >)
C IDX3 cointains the permutation values * 4
define(IDX3, <T + AES_SIDX3 >)


C AES_LOAD(i)
C Get one word of input, XOR with first subkey, store in wtxt
define(<AES_LOAD>, <
	ldub	[src+$1], t0
	ldub	[src+$1+1], t1
	ldub	[src+$1+2], t2
	sll	t1, 8, t1
	
	or	t0, t1, t0	! U
	ldub	[src+$1+3], t1
	sll	t2, 16, t2
	or	t0, t2, t0
	
	sll	t1, 24, t1
	! Get subkey
	ld	[ctx + $1], t2
	or	t0, t1, t0
	xor	t0, t2, t0
	
	st	t0, [wtxt+$1]>)dnl

C AES_ROUND(i)
C Compute one word in the round function. 
C Input in wtxt, output stored in tmp + i.
C
C The comments mark which j in T->table[j][ Bj(wtxt[IDXi(i)]) ]
C the instruction is a part of. 
define(<AES_ROUND>, <
	ld	[IDX1+$1], t1		! 1
	ldub	[wtxt+$1+3], t0		! 0
	ldub	[wtxt+t1], t1		! 1
	sll	t0, 2, t0		! 0
	
	ld	[T0+t0], t0		! 0
	sll	t1, 2, t1		! 1
	ld	[T1+t1], t1		! 1 !U
	ld	[IDX3+$1], t2		! 3
	
	xor	t0, t1, t0		! 0, 1
	! IDX2(j) = j XOR 2
	ldub	[wtxt+eval($1 ^ 8)+1], t1	! 2
	ldub	[wtxt+t2], t2		! 3
	sll	t1, 2, t1		! 2
	
	ld	[T2+t1], t1		! 2	!U
	sll	t2, 2, t2		! 3
	ld	[T3+t2], t2		! 3	!U
	xor	t0, t1, t0		! 0, 1, 2
	
	! Fetch roundkey
	ld	[key + $1], t1
	xor	t0, t2, t0		! 0, 1, 2, 3
	xor	t0, t1, t0		!U
	st	t0, [tmp + $1]>)dnl

C AES_FINAL_ROUND(i)
C Compute one word in the final round function. 
C Input in wtxt, output converted to an octet string and stored at dst. 
C
C The comments mark which j in T->table[j][ Bj(wtxt[IDXi(i)]) ]
C the instruction is a part of. 
define(<AES_FINAL_ROUND>, <
	ld	[IDX1+$1], t1		! 1
	ldub	[wtxt+$1+3], t0		! 0
	ldub	[wtxt+t1], t1		! 1
	ldub	[T+t0], t0		! 0
	
	ldub	[T+t1], t1		! 1
	ld	[IDX3 + $1], t2		! 3
	sll	t1, 8, t1		! 1
	or	t0, t1, t0		! 0, 1 !U
	
	! IDX2(j) = j XOR 2
	ldub	[wtxt+eval($1 ^ 8)+1], t1	! 2
	ldub	[wtxt+t2], t2		! 3
	ldub	[T+t1], t1		! 2
	ldub	[T+t2], t2		! 3
	
	sll	t1, 16, t1		! 2
	or	t0, t1, t0		! 0, 1, 2 !U
	sll	t2, 24, t2		! 3
	ld	[key + $1], t1
	
	or	t0, t2, t0		! 0, 1, 2, 3
	xor	t0, t1, t0		!U
	srl	t0, 24, t1		!U
	stb	t1, [dst+$1+3]		!U
	
	srl	t0, 16, t1
	stb	t1, [dst+$1+2]		!U
	srl	t0, 8, t1
	stb	t1, [dst+$1+1]		!U
	
	stb	t0, [dst+$1]>)dnl
	
C The stack frame looks like (sparc64 aka sparcv9)
C
C %fp -   8: OS-dependent link field (8 bytes)
C %fp -  16: OS-dependent link field (8 bytes)
C %fp -  32: tmp, uint32_t[4] (16 bytes)
C %fp -  48: wtxt, uint32_t[4] (16 bytes)
C %fp - 224: OS register save area. (22*8 == 176 bytes)
C
C /grubba 2005-06-22
define(<FRAME_SIZE>, 224)
define(<BIAS>, 2047)

	.section	".text"
	.align 16
	.global _nettle_aes_crypt
	.type	_nettle_aes_crypt,#function
	.proc	020
	
_nettle_aes_crypt:
	save	%sp, -FRAME_SIZE, %sp
	cmp	length, 0
	be	.Lend
	! wtxt
	add	%fp, BIAS-32, wtxt
	
	add	%fp, BIAS-48, tmp
	ld	[ctx + AES_NROUNDS], nrounds
	! Compute xor, so that we can swap efficiently.
	xor	wtxt, tmp, diff
	! The loop variable will be multiplied by 16.
	! More loop invariants
	add	T, AES_TABLE0, T0
	
	add	T, AES_TABLE1, T1
	add	T, AES_TABLE2, T2
	add	T, AES_TABLE3, T3
	nop
	
.Lblock_loop:
	C  Read src, and add initial subkey
	AES_LOAD(0)	! i = 0
	AES_LOAD(4)	! i = 1
	AES_LOAD(8)	! i = 2
	AES_LOAD(12)	! i = 3
	add	src, 16, src

	sub	nrounds, 1, round
	add	ctx, 16, key
	nop
.Lround_loop:

	AES_ROUND(0)	! i = 0
	AES_ROUND(4)	! i = 1
	AES_ROUND(8)	! i = 2
	AES_ROUND(12)	! i = 3
			
	! switch roles for tmp and wtxt
	xor	wtxt, diff, wtxt
	xor	tmp, diff, tmp
	subcc	round, 1, round
	bne	.Lround_loop

	add	key, 16, key

	C Final round, and storage of the output

	AES_FINAL_ROUND(0)	! i = 0
	AES_FINAL_ROUND(4)	! i = 1
	AES_FINAL_ROUND(8)	! i = 2
	AES_FINAL_ROUND(12)	! i = 3
		
	addcc	length, -16, length
	bne	.Lblock_loop
	add	dst, 16, dst

.Lend:
	ret
	restore
.Leord:
	.size	_nettle_aes_crypt,.Leord-_nettle_aes_crypt

	! Benchmarks on my slow sparcstation:	
	! Original C code	
	! aes128 (ECB encrypt): 14.36s, 0.696MB/s
	! aes128 (ECB decrypt): 17.19s, 0.582MB/s
	! aes128 (CBC encrypt): 16.08s, 0.622MB/s
	! aes128 ((CBC decrypt)): 18.79s, 0.532MB/s
	! 
	! aes192 (ECB encrypt): 16.85s, 0.593MB/s
	! aes192 (ECB decrypt): 19.64s, 0.509MB/s
	! aes192 (CBC encrypt): 18.43s, 0.543MB/s
	! aes192 (CBC decrypt): 20.76s, 0.482MB/s
	! 
	! aes256 (ECB encrypt): 19.12s, 0.523MB/s
	! aes256 (ECB decrypt): 22.57s, 0.443MB/s
	! aes256 (CBC encrypt): 20.92s, 0.478MB/s
	! aes256 (CBC decrypt): 23.22s, 0.431MB/s

	! After unrolling key_addition32, and getting rid of
	! some sll x, 2, x, encryption speed is 0.760 MB/s.

	! Next, the C code was optimized to use larger tables and
	! no rotates. New timings:
	! aes128 (ECB encrypt): 13.10s, 0.763MB/s
	! aes128 (ECB decrypt): 11.51s, 0.869MB/s
	! aes128 (CBC encrypt): 15.15s, 0.660MB/s
	! aes128 (CBC decrypt): 13.10s, 0.763MB/s
	! 
	! aes192 (ECB encrypt): 15.68s, 0.638MB/s
	! aes192 (ECB decrypt): 13.59s, 0.736MB/s
	! aes192 (CBC encrypt): 17.65s, 0.567MB/s
	! aes192 (CBC decrypt): 15.31s, 0.653MB/s
	! 
	! aes256 (ECB encrypt): 17.95s, 0.557MB/s
	! aes256 (ECB decrypt): 15.90s, 0.629MB/s
	! aes256 (CBC encrypt): 20.16s, 0.496MB/s
	! aes256 (CBC decrypt): 17.47s, 0.572MB/s

	! After optimization using pre-shifted indices
	! (AES_SIDX[1-3]): 
	! aes128 (ECB encrypt): 12.46s, 0.803MB/s
	! aes128 (ECB decrypt): 10.74s, 0.931MB/s
	! aes128 (CBC encrypt): 17.74s, 0.564MB/s
	! aes128 (CBC decrypt): 12.43s, 0.805MB/s
	! 
	! aes192 (ECB encrypt): 14.59s, 0.685MB/s
	! aes192 (ECB decrypt): 12.76s, 0.784MB/s
	! aes192 (CBC encrypt): 19.97s, 0.501MB/s
	! aes192 (CBC decrypt): 14.46s, 0.692MB/s
	! 
	! aes256 (ECB encrypt): 17.00s, 0.588MB/s
	! aes256 (ECB decrypt): 14.81s, 0.675MB/s
	! aes256 (CBC encrypt): 22.65s, 0.442MB/s
	! aes256 (CBC decrypt): 16.46s, 0.608MB/s

	! After implementing double buffering
	! aes128 (ECB encrypt): 12.59s, 0.794MB/s
	! aes128 (ECB decrypt): 10.56s, 0.947MB/s
	! aes128 (CBC encrypt): 17.91s, 0.558MB/s
	! aes128 (CBC decrypt): 12.30s, 0.813MB/s
	! 
	! aes192 (ECB encrypt): 15.03s, 0.665MB/s
	! aes192 (ECB decrypt): 12.56s, 0.796MB/s
	! aes192 (CBC encrypt): 20.30s, 0.493MB/s
	! aes192 (CBC decrypt): 14.26s, 0.701MB/s
	! 
	! aes256 (ECB encrypt): 17.30s, 0.578MB/s
	! aes256 (ECB decrypt): 14.51s, 0.689MB/s
	! aes256 (CBC encrypt): 22.75s, 0.440MB/s
	! aes256 (CBC decrypt): 16.35s, 0.612MB/s
	
	! After reordering aes-encrypt.c and aes-decypt.c
	! (the order probably causes strange cache-effects):
	! aes128 (ECB encrypt): 9.21s, 1.086MB/s
	! aes128 (ECB decrypt): 11.13s, 0.898MB/s
	! aes128 (CBC encrypt): 14.12s, 0.708MB/s
	! aes128 (CBC decrypt): 13.77s, 0.726MB/s
	! 
	! aes192 (ECB encrypt): 10.86s, 0.921MB/s
	! aes192 (ECB decrypt): 13.17s, 0.759MB/s
	! aes192 (CBC encrypt): 15.74s, 0.635MB/s
	! aes192 (CBC decrypt): 15.91s, 0.629MB/s
	! 
	! aes256 (ECB encrypt): 12.71s, 0.787MB/s
	! aes256 (ECB decrypt): 15.38s, 0.650MB/s
	! aes256 (CBC encrypt): 17.49s, 0.572MB/s
	! aes256 (CBC decrypt): 17.87s, 0.560MB/s

	! After further optimizations of the initial and final loops,
	! source_loop and final_loop. 
	! aes128 (ECB encrypt): 8.07s, 1.239MB/s
	! aes128 (ECB decrypt): 9.48s, 1.055MB/s
	! aes128 (CBC encrypt): 12.76s, 0.784MB/s
	! aes128 (CBC decrypt): 12.15s, 0.823MB/s
	! 
	! aes192 (ECB encrypt): 9.43s, 1.060MB/s
	! aes192 (ECB decrypt): 11.20s, 0.893MB/s
	! aes192 (CBC encrypt): 14.19s, 0.705MB/s
	! aes192 (CBC decrypt): 13.97s, 0.716MB/s
	! 
	! aes256 (ECB encrypt): 10.81s, 0.925MB/s
	! aes256 (ECB decrypt): 12.92s, 0.774MB/s
	! aes256 (CBC encrypt): 15.59s, 0.641MB/s
	! aes256 (CBC decrypt): 15.76s, 0.635MB/s
	
	! After unrolling loops, and other optimizations suggested by
	! Marcus: 
	! aes128 (ECB encrypt): 6.40s, 1.562MB/s
	! aes128 (ECB decrypt): 8.17s, 1.224MB/s
	! aes128 (CBC encrypt): 13.11s, 0.763MB/s
	! aes128 (CBC decrypt): 10.05s, 0.995MB/s
	! 
	! aes192 (ECB encrypt): 7.43s, 1.346MB/s
	! aes192 (ECB decrypt): 9.51s, 1.052MB/s
	! aes192 (CBC encrypt): 14.09s, 0.710MB/s
	! aes192 (CBC decrypt): 11.58s, 0.864MB/s
	! 
	! aes256 (ECB encrypt): 8.57s, 1.167MB/s
	! aes256 (ECB decrypt): 11.13s, 0.898MB/s
	! aes256 (CBC encrypt): 15.30s, 0.654MB/s
	! aes256 (CBC decrypt): 12.93s, 0.773MB/s
