#ifdef HAVE_VCDIFF

#ifndef VCDIFF_WRAPPER_H
#define VCDIFF_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

  void *vcdiff_init_dictionary (const char* dictionary, size_t len);
  void *vcdiff_init_encoder (const void* hashed_dict);
  void *vcdiff_init_decoder (const char* dictionary, size_t len);

  int vcdiff_encode_chunk (const void* vcencoder,
			   const char* in_buf,
			   int in_buf_len,
			   char** out_buf,
			   int* out_buf_len,
			   int* encoder_state);

  int vcdiff_decode_chunk (const void* vcdecoder,
			   const char* in_buf,
			   int in_buf_len,
			   char** out_buf,
			   int* out_buf_len,
			   int* decoder_state);

  void vcdiff_free_dictionary (const void* hashed_dict);
  void vcdiff_free_encoder (const void* vcencoder);
  void vcdiff_free_decoder (const void* vcdecoder);

  int vcdiff_decoder_set_max_window_size(const void *vcdecoder,
                                         size_t new_size);
  int vcdiff_decoder_set_max_file_size(const void *vcdecoder,
                                       size_t new_size);

#ifdef __cplusplus
}
#endif

#endif

#endif
