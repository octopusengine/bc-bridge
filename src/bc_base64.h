#ifndef _BC_BASE64_H
#define _BC_BASE64_H

#include "bc_common.h"

bool bc_base64_encode(char *output, size_t *output_length, uint8_t *input, size_t input_length);
bool bc_base64_decode(uint8_t *output, size_t *output_length, char *input, size_t input_length);

size_t bc_base64_calculate_encode_length(size_t length);
size_t bc_base64_calculate_decode_length(char *input, size_t length);

#endif /* _BC_BASE64_H */
