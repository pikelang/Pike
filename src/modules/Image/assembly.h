void image_mult_buffer_mmx_x86asm(void *source,int npixels_div_4,
                            int rgbr,int gbrg,int brgb);

void image_mult_buffers_mmx_x86asm(void *dest_s1, char *s2, int npixels_div_4);

void image_add_buffers_mmx_x86asm( char *s1, char *s2, int npixels_div_8 );

void image_add_buffer_mmx_x86asm( char *s1,
                                  int npixels_div_4,
                                  int rgbr,int gbrg,int brgb );

void image_sub_buffer_mmx_x86asm( char *s1,
                                  int npixels_div_4,
                                  int rgbr,int gbrg,int brgb );
