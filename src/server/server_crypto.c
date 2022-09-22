#include <server_crypto.h>

DEBUG_STATIC void print_b_array(hash_t * p_hash);
DEBUG_STATIC hash_t * hex_char_to_byte_array(const char * p_hash_str, size_t hash_size);

//static const char * DB_DIR = ".cape";
//static const char * DB_NAME = ".cape/.cape.db";
//static const char * DB_HASH = ".cape/.cape.hash";
//static const char * DEFAULT_USER = "admin";
//static const char * DEFAULT_HASH = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";

void hash_init_db(char * p_dir_path, size_t dir_length)
{
    FILE * h_hash_file = f_open_file(0, "r");
    if (NULL == h_hash_file)
    {
    }

    if (NULL != h_hash_file)
    {
        fclose(h_hash_file);
    }
}

/*!
 * @brief Function takes a hash_t object and compares it against a hexadecimal
 * string that represents the p_hash to ensure they are the same.
 *
 * @param p_hash Pointer to a p_hash object to compare against
 * @param p_input Hexadecimal representation of the p_hash to compare
 * @param length Length of the hexadecimal representation
 * @return True if hashes match else false
 */
bool hash_pass_match(hash_t * p_hash, const char * p_input, size_t length)
{
    if ((NULL == p_hash) || (NULL == p_input))
    {
        return false;
    }

    hash_t * p_pw_hash = hex_char_to_byte_array(p_input, length);
    if (NULL == p_pw_hash)
    {
        return false;
    }

    for (size_t i = 0; i < p_pw_hash->size; i++)
    {
        if (p_pw_hash->array[i] != p_hash->array[i])
        {
            hash_destroy(& p_pw_hash);
            return false;
        }
    }
    hash_destroy(&p_pw_hash);
    return true;
}

/*!
 * @brief Hash the given byte array into a sha256 hash and return a hash_t
 * structure containing the byte array and its size
 *
 * @param p_input Pointer to the bytearray to hash
 * @param length Length of the bytearray to hash
 * @return hash_t object if successful or a NULL
 */
hash_t * hash_pass(const unsigned char * p_input, size_t length)
{
    if (NULL == p_input)
    {
        return NULL;
    }

    unsigned char md[SHA256_DIGEST_LENGTH];
    unsigned char * p_hash_array = SHA256((const unsigned char *)p_input, length, (unsigned char *)&md);
    if (NULL == p_hash_array)
    {
        fprintf(stderr, "[!] Unknown issue with SHA256\n");
        return NULL;
    }

    uint8_t * p_byte_array = (uint8_t *)calloc(SHA256_DIGEST_LENGTH, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_byte_array))
    {
        return NULL;
    }

    // The SHA256 p_hash_array is not a malloc pointer
    memcpy(p_byte_array, p_hash_array, SHA256_DIGEST_LENGTH);

    hash_t * p_hash = (hash_t *)malloc(sizeof(hash_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_hash))
    {
        goto cleanup;
    }
    *p_hash = (hash_t){
        .array = p_byte_array,
        .size = SHA256_DIGEST_LENGTH
    };
    return p_hash;

cleanup:
    free(p_byte_array);
    return NULL;
}

/*!
 * @brief Free a hash_t object
 *
 * @param pp_hash Pointer to the pp_hash object to free
 */
void hash_destroy(hash_t ** pp_hash)
{
    if (NULL == pp_hash)
    {
        return;
    }
    hash_t * p_hash = *pp_hash;
    if (NULL == p_hash)
    {
        return;
    }

    if (NULL != p_hash->array)
    {
        free(p_hash->array);
    }
    *p_hash = (hash_t){
        .array = NULL,
        .size = 0
    };
    free(p_hash);
    *pp_hash = NULL;
}


DEBUG_STATIC void print_b_array(hash_t * p_hash)
{
    if (NULL == p_hash)
    {
        return;
    }
    for (size_t i = 0; i < p_hash->size; i++)
    {
        printf("%02x", p_hash->array[i]);
    }
    printf("\n");
}

/*!
 * @brief Translate a hexadecimal hash back into a byte array. Every two
 * characters of a hexadecimal array represents one byte. The function
 * reads two characters at a time and converts it to the byte representation.
 * The result is a byte array.
 *
 * @param p_hash_str Pointer to the hexadecimal string
 * @param hash_size Size of the hexadecimal string
 * @return hash_t object if successful otherwise NULL
 */
DEBUG_STATIC hash_t * hex_char_to_byte_array(const char * p_hash_str, size_t hash_size)
{
    if ((NULL == p_hash_str) || (0 == hash_size))
    {
        return NULL;
    }

    // The hash string represents a byte using two characters, so the byte
    // array must be half the size of the hash string
    if (0 != (hash_size % 2))
    {
        fprintf(stderr, "[!] The hash string provided contains an "
                        "odd number of hexadecimal characters.\n");
        return NULL;
    }

    size_t array_size = (size_t)(hash_size / 2);

    uint8_t * p_byte_array = (uint8_t *)calloc(array_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_byte_array))
    {
        return NULL;
    }

    int result;
    size_t str_idx  = 0;
    size_t barr_idx = 0;
    while (str_idx < hash_size)
    {
        result = sscanf(&p_hash_str[str_idx], "%2hhx", &p_byte_array[barr_idx]);

        // The scan API returns the number of bytes read. To check if a valid
        // read occurred a comparison of 1 byte is made.
        if (1 == result)
        {
            str_idx  += 2;
            barr_idx += 1;
        }
        // EOF is returned when either the end of the string is reached or
        // when an error occurred. Since we are reading with bounds, we should
        // only get EOF when an error occurs
        else if (EOF == result)
        {
            perror("sscanf");
            goto cleanup;
        }
    }

    hash_t * p_hash = (hash_t *)malloc(sizeof(hash_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_hash))
    {
        goto cleanup;
    }
    *p_hash = (hash_t){
        .array  = p_byte_array,
        .size   = array_size
    };
    return p_hash;

cleanup:
    free(p_byte_array);
    return NULL;
}




