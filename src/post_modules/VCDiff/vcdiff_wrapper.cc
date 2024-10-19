#ifdef HAVE_VCDIFF

#include <string>
#include <string.h>
#include <stdlib.h>
#include <exception>
#include "vcdiff_wrapper.h"

#include <google/vcencoder.h>
#include <google/vcdecoder.h>

void* vcdiff_init_dictionary (const char* dictionary, size_t len)
{
  open_vcdiff::HashedDictionary *dict;
  try {
    dict =  new open_vcdiff::HashedDictionary (dictionary, len);
  } catch (std::exception& e) {
    return NULL;
  }

  if (!dict->Init()) {
    delete dict;
    dict = NULL;
  }
  return dict;
}

void* vcdiff_init_encoder (const void* hashed_dict)
{
  open_vcdiff::VCDiffStreamingEncoder *vcencoder;
  vcencoder =
    new open_vcdiff
    ::VCDiffStreamingEncoder ((const open_vcdiff::HashedDictionary*)hashed_dict,
			      false,
			      false);
  return vcencoder;
}

void* vcdiff_init_decoder (const char* dictionary, size_t len)
{
  open_vcdiff::VCDiffStreamingDecoder *vcdecoder;
  vcdecoder = new open_vcdiff::VCDiffStreamingDecoder();

  vcdecoder->SetAllowVcdTarget (false);
  vcdecoder->StartDecoding (dictionary, len);

  return vcdecoder;
}

int vcdiff_encode_chunk (const void* vcencoder,
			 const char* in_buf,
			 int in_buf_len,
			 char** out_buf,
			 int* out_buf_len,
			 int* encoder_state)
{
  open_vcdiff::VCDiffStreamingEncoder *vcenc =
    (open_vcdiff::VCDiffStreamingEncoder*) vcencoder;

  std::string output_string;
  int res = 1;

  if (*encoder_state == 1) {
    res &= vcenc->StartEncoding (&output_string);
    *encoder_state = 2;
  }

  if (*encoder_state == 2) {
    if (in_buf != NULL) {
      res &= vcenc->EncodeChunk (in_buf, in_buf_len, &output_string);
    } else {
      res &= vcenc->FinishEncoding (&output_string);
      *encoder_state = 3;
    }
  }

  *out_buf_len = output_string.length();
  *out_buf = (char *)malloc (*out_buf_len);
  memcpy (*out_buf, output_string.data(), *out_buf_len);
  return res;
}

int vcdiff_decode_chunk (const void* vcdecoder,
			 const char* in_buf,
			 int in_buf_len,
			 char** out_buf,
			 int* out_buf_len,
			 int* decoder_state)
{
  open_vcdiff::VCDiffStreamingDecoder *vcdec =
    (open_vcdiff::VCDiffStreamingDecoder*) vcdecoder;

  std::string output_string;
  int res = 1;

  if (*decoder_state == 2) {
    if (in_buf != NULL) {
      res &= vcdec->DecodeChunk (in_buf, in_buf_len, &output_string);
    } else {
      res &= vcdec->FinishDecoding ();
      *decoder_state = 3;
    }
  }

  *out_buf_len = output_string.length();
  *out_buf = (char *)malloc (*out_buf_len);
  memcpy (*out_buf, output_string.data(), *out_buf_len);
  return res;
}

void vcdiff_free_dictionary (const void* hashed_dict)
{
  delete (const open_vcdiff::HashedDictionary*) hashed_dict;
}

void vcdiff_free_encoder (const void* vcencoder)
{
  delete (const open_vcdiff::VCDiffStreamingEncoder*) vcencoder;
}

void vcdiff_free_decoder (const void* vcdecoder)
{
  delete (const open_vcdiff::VCDiffStreamingDecoder*) vcdecoder;
}

int vcdiff_decoder_set_max_window_size(const void *vcdecoder,
                                       size_t new_size)
{
  open_vcdiff::VCDiffStreamingDecoder *vcdec =
    (open_vcdiff::VCDiffStreamingDecoder*) vcdecoder;

  return vcdec->SetMaximumTargetWindowSize(new_size);
}

int vcdiff_decoder_set_max_file_size(const void *vcdecoder,
                                     size_t new_size)
{
  open_vcdiff::VCDiffStreamingDecoder *vcdec =
    (open_vcdiff::VCDiffStreamingDecoder*) vcdecoder;

  return vcdec->SetMaximumTargetFileSize(new_size);
}

#endif /* HAVE_VCDIFF */
