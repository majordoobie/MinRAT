#ifndef BSLE_GALINDEZ_INCLUDE_SERVER_CRYPTO_H_
#define BSLE_GALINDEZ_INCLUDE_SERVER_CRYPTO_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus
// HEADER GUARD
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <utils.h>

typedef struct
{
    size_t      size;
    uint8_t *   array;
} hash_t;

/*!
 * @brief Function takes a hash_t object and compares it against a hexadecimal
 * string that represents the p_hash to ensure they are the same.
 *
 * @param p_hash Pointer to a p_hash object to compare against
 * @param p_input Hexadecimal representation of the p_hash to compare
 * @param length Length of the hexadecimal representation
 * @return True if hashes match else false
 */
bool hash_pass_match(hash_t * p_hash, const char * p_input, size_t length);

/*!
 * @brief Hash the given byte array into a sha256 hash and return a hash_t
 * structure containing the byte array and its size
 *
 * @param p_byte_array Pointer to the bytearray to hash
 * @param length Length of the bytearray to hash
 * @return hash_t object if successful or a NULL
 */
hash_t * hash_byte_array(uint8_t * p_byte_array, size_t length);

/*!
 * @brief Free a hash_t object
 *
 * @param pp_hash Pointer to the pp_hash object to free
 */
void hash_destroy(hash_t ** pp_hash);


// HEADER GUARD
#ifdef __cplusplus
}
#endif // END __cplusplus
#endif //BSLE_GALINDEZ_INCLUDE_SERVER_CRYPTO_H_
