/* d == s is valid for all of these functions. */

void image_mult_buffer_mmx_x86asm(void *dest,void *source,int npixels_div_4,
                                  int rgbr, int gbrg, int brgb);

void image_mult_buffers_mmx_x86asm(void *dest, void *s1, void *s2, 
                                   int npixels_div_4);

void image_add_buffers_mmx_x86asm( void *d,void *s1, void *s2, 
                                   int npixels_div_8 );

void image_add_buffer_mmx_x86asm( void *d, void *s,
                                  int npixels_div_4,
                                  int rgbr, int gbrg, int brgb );

void image_sub_buffer_mmx_x86asm( void *d, void *s,
                                  int npixels_div_4,
                                  int rgbr, int gbrg, int brgb );

void image_get_cpuid( int oper, void *cpuid1, void *cpuid2, void *cpuid3, void *d );
