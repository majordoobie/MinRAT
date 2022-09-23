#include <server_init.h>

static const char * DB_DIR          = ".cape";
static const char * DB_NAME         = ".cape/.cape.db";
static const char * DB_HASH         = ".cape/.cape.hash";
static const char * DEFAULT_USER    = "admin";
static const char * DEFAULT_HASH    = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";
static const uint32_t MAGIC_BYTES   = 0xFFAAFABA;

static verified_path_t * init_db_dir(char * p_home_dir);
static verified_path_t * init_db_file(const char * p_home_dir);
static verified_path_t * update_db_hash(const char * p_home_dir, verified_path_t * p_db_file);

int8_t hash_init_db(char * p_home_dir, size_t dir_length)
{
    // Check if the ${HOME_DIR}/.cape directory exists; If not, create it
    verified_path_t * p_db_dir = f_path_resolve(p_home_dir, DB_DIR);
    if (NULL == p_db_dir)
    {
        debug_print("[!] Database dir %s/%s does not exist. Attempting to create"
                    " it.\n", p_home_dir, DB_DIR);
        p_db_dir = init_db_dir(p_home_dir);
        if (NULL == p_db_dir)
        {
            goto ret_null;
        }
    }
    // Once verified/created we no longer need the object
    f_destroy_path(&p_db_dir);

    // With the ${HOME_DIR}/.cape directory in existence, read the .cape.hash
    // and the .cape.db
    verified_path_t * p_hash_file = f_path_resolve(p_home_dir, DB_HASH);
    verified_path_t * p_db_file = f_path_resolve(p_home_dir, DB_NAME);
    if ((NULL == p_hash_file) && (NULL == p_db_file))
    {
        debug_print("%s\n", "[!] The database files do not exist, attempting to "
                    "create the defaults");
        // Attempt to init the db_file
        p_db_file = init_db_file(p_home_dir);
        if (NULL == p_db_file)
        {
            goto ret_null;
        }
        p_hash_file = update_db_hash(p_home_dir, p_db_file);
        if (NULL == p_hash_file)
        {
            goto cleanup_db;
        }
    }

    file_content_t * p_db_contents = f_read_file(p_db_file);
    if (NULL == p_db_contents)
    {
        goto cleanup_hash;
    }
    file_content_t * p_hash_contents = f_read_file(p_hash_file);
    if (NULL == p_hash_contents)
    {
        goto cleanup_db_content;
    }

    f_destroy_path(&p_hash_file);
    f_destroy_path(&p_db_file);

    //TODO you need to read the buff dude
    if (hash_hash_t_match(p_db_contents->p_hash, p_hash_contents->p_hash))
    {
        printf("THEY MATCH\n");
    }
    else
    {
        printf("THey do not match\n");
    }

    f_destroy_content(&p_db_contents);
    f_destroy_content(&p_hash_contents);


    return 0;

cleanup_db_content:
    f_destroy_content(&p_db_contents);
cleanup_hash:
    f_destroy_path(&p_hash_file);
cleanup_db:
    f_destroy_path(&p_db_file);
ret_null:
    return -1;
}

/*!
 * @brief Update the .cape.hash file with the hash that represents the
 * .cape.db file. This is used to ensure that the file has not been corrupted
 *
 * @param p_home_dir Pointer to the servers home directory
 * @param p_db_file verified_path_t object to the .cape.db file
 * @return verified_path_t object representing the .cape.hash or NULL if error
 */
static verified_path_t * update_db_hash(const char * p_home_dir, verified_path_t * p_db_file)
{
    FILE * h_db_file = f_open_file(p_db_file, "r");
    if (NULL == h_db_file)
    {
        fprintf(stderr, "[!] Could not open the database file "
                        "in %s/%s you may have to create it yourself\n",
                DB_DIR, DB_NAME);
        goto ret_null;
    }

    // Get the files size to allocate a byte array
    fseek(h_db_file, 0L, SEEK_END);
    long int file_size = ftell(h_db_file);
    if (-1 == file_size)
    {
        perror("ftell");
        goto cleanup_file;
    }
    fseek(h_db_file, 0L, SEEK_SET);

    // Create the memory to read the files contents
    uint8_t * p_byte_array = (uint8_t *)calloc((unsigned long)file_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_byte_array))
    {
        goto cleanup_file;
    }

    // Read the contents of the file so that we can hash it
    size_t bytes_read = fread(p_byte_array, sizeof(uint8_t),(unsigned long)file_size, h_db_file);
    fclose(h_db_file);
    if (bytes_read != (unsigned long)file_size)
    {
        fprintf(stderr, "[!] Could not properly read the file\n");
        goto cleanup_array;
    }

    // Call the hashing function to get a hash structure to save the hash
    // string to the hash file
    hash_t * p_hash = hash_byte_array(p_byte_array, bytes_read);
    if (NULL == p_hash)
    {
        fprintf(stderr, "[!] Could not hash the file\n");
        goto cleanup_array;
    }

    // Get a verified path for the hash file to write the hash data to
    verified_path_t * p_hash_file = f_valid_resolve(p_home_dir, DB_HASH);
    if (NULL == p_hash_file)
    {
        fprintf(stderr, "[!] Could not create the database file "
                        "in %s/%s you may have to create it yourself\n",
                DB_DIR, DB_NAME);
        goto cleanup_hash;
    }

    // Open a handle the hash file to write the contents to
    FILE * h_hash_file = f_open_file(p_hash_file, "w");
    if (NULL == h_hash_file) // Message printed already
    {
        goto cleanup_hash_file;
    }

    fwrite(&MAGIC_BYTES, sizeof(uint32_t), 1, h_hash_file);
    fwrite("\n", sizeof(char), 1, h_hash_file);
    fwrite(p_hash->array, sizeof(uint8_t), p_hash->size, h_hash_file);

    // clean up
    fclose(h_hash_file);
    free(p_byte_array);
    hash_destroy(&p_hash);
    debug_print("%s\n", "[+] .cape.hash file updated with new .cape.db hash");

    return p_hash_file;

cleanup_hash_file:
    f_destroy_path(&p_hash_file);
cleanup_hash:
    hash_destroy(&p_hash);
cleanup_array:
    free(p_byte_array);
cleanup_file:
    fclose(h_db_file);
ret_null:
    return NULL;
}

/*!
 * @brief Initialized the db file with the default admin user
 *
 * @param p_home_dir Pointer to the home directory of the server
 * @return If successful, a verified_path_t object is returned otherwise NULL
 */
static verified_path_t * init_db_file(const char * p_home_dir)
{
    // Attempt to resolve the path to the db file; This should never fail at
    // this point, but you can never be too sure
    verified_path_t * p_db = f_valid_resolve(p_home_dir, DB_NAME);
    if (NULL == p_db)
    {
        fprintf(stderr, "[!] Could not create the database file "
                        "in %s/%s you may have to create it yourself\n",
                p_home_dir, DB_NAME);
        goto ret_null;
    }

    // Create the p_buffer used to write to the file.
    // *NOTE* the + 4 is to account for the format and NULL ":", ":", "\n", "\0"
    size_t array_size = sizeof(MAGIC_BYTES) + strlen(DEFAULT_USER) + sizeof(uint8_t) + strlen(DEFAULT_HASH) + 4;
    uint8_t * p_buffer = (uint8_t *)calloc(array_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_buffer))
    {
        goto cleanup_path;
    }

    // Populate the p_buffer with the default data
    memcpy(p_buffer, &MAGIC_BYTES, 4);
    int write_bytes = sprintf((char *)(p_buffer + 4), "%s:%hhd:%s\n", DEFAULT_USER, (uint8_t){ADMIN}, DEFAULT_HASH);
    //*NOTE* The - 4 is to exclude the magic bytes and the + 1 is to
    // include the "\0". sprintf does not consider this a byte written
    if ((array_size - 4) != (write_bytes + 1))
    {
        fprintf(stderr, "[!] Unable to properly write to db_file\n");
        goto cleanup_array;
    }

    file_op_t status = f_write_file(p_db, p_buffer, array_size);
    if (FILE_OP_FAILURE == status)
    {
        goto cleanup_array;
    }
    debug_print("%s\n", "[+] Successfully created the .cape.db file with "
                        "defaults");

    free(p_buffer);
    p_buffer = NULL;
    return p_db;

cleanup_array:
    free(p_buffer);
    p_buffer = NULL;
cleanup_path:
    f_destroy_path(&p_db);
ret_null:
    return NULL;
}

/*!
 * @brief Create the database directory to save the server information
 *
 * @param p_home_dir Pointer to the home directory of the server
 * @return verified_path_t object if successful, otherwise NULL
 */
static verified_path_t * init_db_dir(char * p_home_dir)
{
    verified_path_t * p_db_dir = f_valid_resolve(p_home_dir, DB_DIR);
    if (NULL == p_db_dir)
    {
        goto ret_null;
    }

    file_op_t status = f_create_dir(p_db_dir);
    if (FILE_OP_FAILURE == status)
    {
        fprintf(stderr, "[!] Could not create the database "
                        "directory in %s. You may have to perform this "
                        "operation yourself\n", DB_DIR);
        goto cleanup;
    }

    debug_print("%s\n", "[+] Successfully created the database directory");
    return p_db_dir;

cleanup:
    f_destroy_path(&p_db_dir);
ret_null:
    return NULL;
}