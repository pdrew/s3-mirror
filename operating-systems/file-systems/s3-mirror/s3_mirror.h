#ifndef S3_MIRROR_H 
#define S3_MIRROR_H 

#define MAX_KEY_LEN 1024
#define BB_DATA ((struct s3m_state *) fuse_get_context()->private_data)

#ifdef __cplusplus
extern "C" {
#endif
    struct s3m_state {
        char *bucket;
    };

    typedef struct s3m_object_t {
        long size;
        time_t last_modified;
        char *key;
    } s3m_object_t;

    typedef struct s3m_object_list_t {
        int num_objs;
        int num_dirs;
        s3m_object_t *objs;
        char **dirs;
    } s3m_object_list_t;

    void s3m_init_api();
    void s3m_shutdown_api();
    s3m_object_list_t *s3m_list_objects(const char *bucket, const char *prefix); 
    const char *s3m_read_object(const char *bucket, const char *key);
#ifdef __cplusplus
}
#endif

#endif