;  depends on nasm

 global image_mult_buffer_mmx_x86asm
 global image_mult_buffers_mmx_x86asm

 global image_add_buffers_mmx_x86asm
 global image_add_buffer_mmx_x86asm

 global image_sub_buffer_mmx_x86asm

 global image_get_cpuid

;  do not use the function from mmxlib, since that might be
;  unavailable even when mmx is available.	
;
;  void image_get_cpuid(int oper,int *cpuid1,int *cpuid2,int *cpuid3,int *a )
;
image_get_cpuid:
	enter 0,0
	pushf
	pop  eax
	mov  ecx, eax
	xor  eax, 200000h
	push eax
	popf
	pushf
	pop  eax
	xor  ecx, eax
	test ecx, 200000h
	jnz .ok

	xor eax,eax
	mov [ebp+12], eax	;  cpuid not supported
	mov [ebp+16], eax
	mov [ebp+20], eax

	leave
	ret

.ok:	
	push ebx
 	mov eax, [ebp+8]
	cpuid
	push ebx
	mov ebx, [ebp+24]
	mov [ebx], eax
	pop ebx

	mov eax, [ebp+12]
	mov [eax], ebx

	mov eax, [ebp+16]
	mov [eax], edx	

	mov eax, [ebp+20]
	mov [eax], ecx

	pop ebx
	leave
	ret


; Add two images
;
; void image_add_buffers_mmx_x86asm( char *d,
;		                     char *s1, char *s2, 
;                                    int npixels_mult_3_div_8 )
;
image_add_buffers_mmx_x86asm:
	enter 0,0
	push ebx
	mov ebx, [ebp+8]
	mov eax, [ebp+12]
	mov edx, [ebp+16]
	mov ecx, [ebp+20]

.loop:
	movq	mm0, [edx]
	add	edx, 8
	paddusb mm0, [eax]
	add	eax, 8
	movq	[ebx],mm0
	add	ebx, 8
	loopnz .loop,ecx
		
	emms
	pop ebx
	leave
	ret


	
	
; Multiply two images

; Add a fixed rgb value to an image
; 
; void image_mult_buffers_mmx_x86asm( char *d, char *s1, char *s2, 
;				      int npixels_div_4 )
;
image_mult_buffers_mmx_x86asm:
	enter 0,0
	push ebx
	mov ebx, [ebp+8]
	mov eax, [ebp+12]
	mov edx, [ebp+16]
	mov ecx, [ebp+20]

	pxor    mm4,mm4

.loop:
	movd	  mm0, [eax]
	add	  eax, 4

	punpcklbw mm0,mm4
	movd      mm1, [edx]
	add	  edx, 4

	punpcklbw mm1,mm4

	pmullw	  mm0,mm1
	psrlw	  mm0, 8
	packuswb  mm0,mm0

	movd	  [ebx],mm0
	add	  ebx, 4

	loopnz .loop,ecx
		
	emms
	pop ebx
	leave
	ret


; Subtract a RGB-value value to an image
;
; void image_sub_buffer_mmx_x86asm( char *d, char *source, int npixels,
;	                             int rgbr, int gbrg, int brgb )
;
; edx				dest          ebp+8
; eax				source        ebp+12
; mm0				sourcedata    [eax]
; ecx				numpixels     ebp+16
; mm1				mult1 <rgbr>  ebp+20
; mm2				mult2 <gbrg>  ebp+24
; mm3				mult3 <brgb>  ebp+28
; mm4				null
;
; This funciton can be changed to do twice the amount of work per
; instruction, but it's all memory bound anyway, so there is no
; significant performance gain.
;
image_sub_buffer_mmx_x86asm:
	enter   0,0

	mov edx, [ebp+8]
	mov eax, [ebp+12]

	mov ecx, [ebp+16]

	movd	mm1,[ebp+20]	; rgb r
	movd	mm2,[ebp+24]	; gb rg
	movd	mm3,[ebp+28]	; b rgb

;	r g b r g b r g b r g b r g b r g  ...
;	0       4       8       12      16
;
;  int is 0:	 r b g r   
;	  4:	 g r b g  
; 	  8:	 b g r b
.loop:
 	movd	  mm0,[eax]
	add	  eax,4
	psubusb	  mm0,mm1
 	movd	  [edx],mm0
	add	  edx,4


 	movd	  mm0,[eax]
	add	  eax,4
	psubusb	  mm0,mm2
 	movd	  [edx],mm0
	add	  edx,4

 	movd	  mm0,[eax]
	add	  eax,4
	psubusb	  mm0,mm3
 	movd	  [edx],mm0
	add	  edx,4

	loopnz .loop,ecx
	
	emms
	leave
	ret

; Add a RGB-value value to an image
;
; void image_add_buffer_mmx_x86asm( char *d, char *source, int npixels,
;	                             int rgbr, int gbrg, int brgb )
;
; edx				dest          ebp+8
; eax				source        ebp+12
; mm0				sourcedata    [eax]
; ecx				numpixels     ebp+16
; mm1				mult1 <rgbr>  ebp+20
; mm2				mult2 <gbrg>  ebp+24
; mm3				mult3 <brgb>  ebp+28
; mm4				null
;
; This funciton can be changed to do twice the amount of work per
; instruction, but it's all memory bound anyway, so there is no
; significant performance gain.
;
image_add_buffer_mmx_x86asm:
	enter   0,0

	mov edx, [ebp+8]
	mov eax, [ebp+12]

	mov ecx, [ebp+16]

	movd	mm1,[ebp+20]	; rgb r
	movd	mm2,[ebp+24]	; gb rg
	movd	mm3,[ebp+28]	; b rgb


;	r g b r g b r g b r g b r g b r g  ...
;	0       4       8       12      16
;
;  int is 0:	 r b g r   
;	  4:	 g r b g  
; 	  8:	 b g r b
.loop:
 	movd	  mm0,[eax]	
	add	  eax,4		
	paddusb	  mm0,mm1	
 	movd	  [edx],mm0	
	add	  edx,4		

 	movd	  mm0,[eax]	
	add	  eax,4		
	paddusb	  mm0,mm2
 	movd	  [edx],mm0	
	add	  edx,4		

 	movd	  mm0,[eax]
	add	  eax,4		
	paddusb	  mm0,mm3
 	movd	  [edx],mm0
	add	  edx,4

	loopnz .loop,ecx
	
	emms
	leave
	ret
	
; Multiply an image with a fixed rgb value
;
; void image_mult_buffer_mmx_x86asm( char *d, char *source, int npixels, 
;	                             int rgbr, int gbrg, int brgb )
;
; eax				address       ebp+8
; mm0				sourcedata    [eax]
; ecx				numpixels     ebp+12
; mm1				mult1 <rgbr>  ebp+16
; mm2				mult2 <gbrg>  ebp+20
; mm3				mult3 <brgb>  ebp+24
; mm4				null
image_mult_buffer_mmx_x86asm:
	enter   0,0

	mov edx, [ebp+8]
	mov eax, [ebp+12]
	mov ecx, [ebp+16]

	pxor    mm4,mm4

	movd	mm1,[ebp+20]	; rgb r
	punpcklbw mm1, mm4

	movd	mm2,[ebp+24]	; gb rg
	punpcklbw mm2, mm4

	movd	mm3,[ebp+28]	; b rgb
	punpcklbw mm3, mm4



;	r g b r g b r g b r g b r g b r g  ...
;	0       4       8       12      16
;
;  int is 0:	 r b g r   
;	  4:	 g r b g  
; 	  8:	 b g r b
.loop:
 	movd	  mm0,[eax]	
	add	  eax,4		
	punpcklbw mm0,mm4	
	pmullw	  mm0,mm1	
	psrlw	  mm0,8		
	packuswb  mm0,mm0	
 	movd	  [edx],mm0	
	add	  edx,4		


 	movd	  mm0,[eax]	
	add	  eax,4		
	punpcklbw mm0,mm4	
	pmullw	  mm0,mm2	
	psrlw	  mm0,8		
	packuswb  mm0,mm0	
 	movd	  [edx],mm0	
	add	  edx,4		

 	movd	  mm0,[eax]
	add	  eax,4		
	punpcklbw mm0,mm4
	pmullw	  mm0,mm3
	psrlw	  mm0,8
	packuswb  mm0,mm0
 	movd	  [edx],mm0
	add	  edx,4

	loopnz .loop,ecx
	
	emms
	leave
	ret
