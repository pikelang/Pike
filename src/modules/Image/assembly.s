;  depends on nasm

 global image_mult_buffer_mmx_x86asm
 global image_mult_buffers_mmx_x86asm
 global image_add_buffers_mmx_x86asm
 global image_add_buffer_mmx_x86asm
 global image_sub_buffer_mmx_x86asm

; Add two images
;
; void image_add_buffer_mmx_x86asm( char *s1, char *s2, int npixels_div_8 )
image_add_buffers_mmx_x86asm:
	enter 0,0
	mov eax, [ebp+8]
	mov edx, [ebp+12]
	mov ecx, [ebp+16]

.loop:
	movq	mm0, [edx]
	add	edx, 8
	paddusb mm0, [eax]
	movq	[eax],mm0
	add	eax, 8
	loopnz .loop,ecx
		
	emms
	leave
	ret


	
	
; Multiply two images

; Add a fixed rgb value to an image
; 
; void image_mult_buffers_mmx_x86asm( char *s1, char *s2, int npixels_div_4 )
image_mult_buffers_mmx_x86asm:
	enter 0,0
	mov eax, [ebp+8]
	mov edx, [ebp+12]
	mov ecx, [ebp+16]

	pxor    mm4,mm4

.loop:
	movd	  mm0, [eax]
	movd      mm1, [edx]
	punpcklbw mm0,mm4
	add	  edx, 4
	punpcklbw mm1,mm4
	pmullw	  mm0,mm1
	psrlw	  mm0, 8
	packuswb  mm0,mm0
 	movd	  [eax],mm0	
	add	  eax, 4
	loopnz .loop,ecx
		
	emms
	leave
	ret


; Subtract a RGB-value value to an image
;
; void image_sub_buffer_mmx_x86asm( char *source, int npixels,
;	                             int rgbr, int gbrg, int brgb )
;
; eax				address       ebp+8
; mm0				sourcedata    [eax]
; ecx				numpixels     ebp+12
; mm1				mult1 <rgbr>  ebp+16
; mm2				mult2 <gbrg>  ebp+20
; mm3				mult3 <brgb>  ebp+24
; mm4				null
image_sub_buffer_mmx_x86asm:
	enter   0,0

	mov eax, [ebp+8]

	mov ecx, [ebp+12]

	movd	mm1,[ebp+16]	; rgb r
	movd	mm2,[ebp+20]	; gb rg
	movd	mm3,[ebp+24]	; b rgb

;	r g b r g b r g b r g b r g b r g  ...
;	0       4       8       12      16
;
;  int is 0:	 r b g r   
;	  4:	 g r b g  
; 	  8:	 b g r b
.loop:
 	movd	  mm0,[eax]	
	psubusb	  mm0,mm1	
 	movd	  [eax],mm0	
	add	  eax,4		


 	movd	  mm0,[eax]	
	psubusb	  mm0,mm2
 	movd	  [eax],mm0	
	add	  eax,4		

 	movd	  mm0,[eax]
	psubusb	  mm0,mm3
 	movd	  [eax],mm0
	add	  eax,4

	loopnz .loop,ecx
	
	emms
	leave
	ret

; Add a RGB-value value to an image
;
; void image_add_buffer_mmx_x86asm( char *source, int npixels,
;	                             int rgbr, int gbrg, int brgb )
;
; eax				address       ebp+8
; mm0				sourcedata    [eax]
; ecx				numpixels     ebp+12
; mm1				mult1 <rgbr>  ebp+16
; mm2				mult2 <gbrg>  ebp+20
; mm3				mult3 <brgb>  ebp+24
; mm4				null
image_add_buffer_mmx_x86asm:
	enter   0,0

	mov eax, [ebp+8]

	mov ecx, [ebp+12]

	movd	mm1,[ebp+16]	; rgb r
	movd	mm2,[ebp+20]	; gb rg
	movd	mm3,[ebp+24]	; b rgb


;	r g b r g b r g b r g b r g b r g  ...
;	0       4       8       12      16
;
;  int is 0:	 r b g r   
;	  4:	 g r b g  
; 	  8:	 b g r b
.loop:
 	movd	  mm0,[eax]	
	paddusb	  mm0,mm1	
 	movd	  [eax],mm0	
	add	  eax,4		

 	movd	  mm0,[eax]	
	paddusb	  mm0,mm2
 	movd	  [eax],mm0	
	add	  eax,4		

 	movd	  mm0,[eax]
	paddusb	  mm0,mm3
 	movd	  [eax],mm0
	add	  eax,4

	loopnz .loop,ecx
	
	emms
	leave
	ret
	
; Multiply an image with a fixed rgb value
;
; void image_mult_buffer_mmx_x86asm( char *source, int npixels, 
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

	mov eax, [ebp+8]

	mov ecx, [ebp+12]

	pxor    mm4,mm4

	movd	mm1,[ebp+16]	; rgb r
	punpcklbw mm1, mm4

	movd	mm2,[ebp+20]	; gb rg
	punpcklbw mm2, mm4

	movd	mm3,[ebp+24]	; b rgb
	punpcklbw mm3, mm4



;	r g b r g b r g b r g b r g b r g  ...
;	0       4       8       12      16
;
;  int is 0:	 r b g r   
;	  4:	 g r b g  
; 	  8:	 b g r b
.loop:
 	movd	  mm0,[eax]	
	punpcklbw mm0,mm4	
	pmullw	  mm0,mm1	
	psrlw	  mm0,8		
	packuswb  mm0,mm0	
 	movd	  [eax],mm0	
	add	  eax,4		


 	movd	  mm0,[eax]	
	punpcklbw mm0,mm4	
	pmullw	  mm0,mm2	
	psrlw	  mm0,8		
	packuswb  mm0,mm0	
 	movd	  [eax],mm0	
	add	  eax,4		

 	movd	  mm0,[eax]
	punpcklbw mm0,mm4
	pmullw	  mm0,mm3
	psrlw	  mm0,8
	packuswb  mm0,mm0
 	movd	  [eax],mm0
	add	  eax,4

	loopnz .loop,ecx
	
	emms
	leave
	ret
