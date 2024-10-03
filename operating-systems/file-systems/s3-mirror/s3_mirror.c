#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>

#include "s3_mirror.h"

static struct options {
	int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

static void *s3m_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
    printf("s3m_init\n");
	fuse_get_context();

    s3m_init_api();

    return BB_DATA;
}

void s3m_free_object_list(s3m_object_list_t *list) {
    int i;

    for (i = 0; i < list->num_dirs; i++) {
        free(list->dirs[i]);
    }

    for (i = 0; i < list->num_objs; i++) {
        free(list->objs[i].key);
    }

    free(list->objs);
    free(list);
}

void s3m_set_prefix(char *prefix, const char *path) {
    if (strcmp(path, "/") == 0) {
        prefix[0] = '\0';
        return;
    }

    if (path[0] == '/') {
        strcpy(prefix, path + 1);
    } else {
        strcpy(prefix, path);
    }
}

void s3m_set_dir_prefix(char *prefix, const char *path) {
    s3m_set_prefix(prefix, path);    

    strcat(prefix, "/");
}

void s3m_set_fbuf(char *fbuf, char *prefix, char *key) {
    memset(fbuf, 0, sizeof(char) * MAX_KEY_LEN);

    if (strncmp(key, prefix, strlen(prefix)) == 0) {
        strncpy(fbuf, key + strlen(prefix), sizeof(fbuf) - 1);
    } else {
        strncpy(fbuf, key, sizeof(fbuf) - 1);
    }

    fbuf[sizeof(fbuf) - 1] = '\0';

    int len = strlen(fbuf);

    if (fbuf[len - 1] == '/')
        fbuf[len - 1] = '\0';
}

static int s3m_getattr(const char *path, struct stat *stbuf,
		       struct fuse_file_info *fi) {
    printf("s3m_getattr: %s\n", path);
    int i, res = 0;
    char prefix[MAX_KEY_LEN];

    memset(stbuf, 0, sizeof(struct stat));

    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_atime = time(NULL);
    stbuf->st_mtime = time(NULL);

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    
        return res;
    }

    s3m_set_prefix(prefix, path);

    s3m_object_list_t *list = s3m_list_objects(BB_DATA->bucket, prefix);

    if (list == NULL) {
        printf("list NULL\n");
        return -ENOENT;
    }

    for (i = 0; i < list->num_objs; i++) {
        int len = strlen(list->objs[i].key);

        if (list->objs[i].key[len - 1] == '/')
            continue;

        if (strcmp(path + 1, list->objs[i].key) == 0) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = list->objs[i].size;
            stbuf->st_mtime = list->objs[i].last_modified;        

            s3m_free_object_list(list);

            return res;
        }
    }

    for (i = 0; i < list->num_dirs; i++) {
        int len = strlen(list->dirs[i]);

        if (list->dirs[i][len - 1] == '/')
            list->dirs[i][len - 1] = '\0';

        if (strcmp(path + 1, list->dirs[i]) == 0) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;

            s3m_free_object_list(list);

            return res;
        }
    }

    return -ENOENT;
}

static int s3m_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags) {
    int i;
    char prefix[MAX_KEY_LEN], fbuf[MAX_KEY_LEN];

    printf("s3m_readdir: %s\n", path);

    filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

    s3m_set_dir_prefix(prefix, path);

    s3m_object_list_t *list = s3m_list_objects(BB_DATA->bucket, prefix);

    for (i = 0; i < list->num_dirs; i++) {
        s3m_set_fbuf(fbuf, prefix, list->dirs[i]);
        
        if (strlen(fbuf))
            filler(buf, fbuf, NULL, 0, FUSE_FILL_DIR_PLUS);
    }

    for (i = 0; i < list->num_objs; i++) {
        s3m_set_fbuf(fbuf, prefix, list->objs[i].key);
        
        if (strlen(fbuf))
            filler(buf, fbuf, NULL, 0, FUSE_FILL_DIR_PLUS);
    }

    s3m_free_object_list(list);

    return 0;
}

static int s3m_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi) {

    const char *content = s3m_read_object(BB_DATA->bucket, path);

    memcpy(buf, content + offset, size);

    return strlen(content) - offset; 
}

void s3m_destroy(void *ptr) {
    printf("s3m_destroy\n");
    free(ptr);
    s3m_shutdown_api();
}

static const struct fuse_operations s3m_oper = {  
  .init    = s3m_init,
  .getattr = s3m_getattr,
  .readdir = s3m_readdir,
  .read    = s3m_read,
  .destroy = s3m_destroy
};

static void show_help(const char *progname)
{
	printf("usage: %s [options] <bucket> <mountpoint>\n\n", progname);
}

int main(int argc, char *argv[]) {
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
    {
        show_help(argv[0]);
        return 1;
    }

    struct s3m_state *state;
    state = malloc(sizeof(struct s3m_state));
    
    state->bucket = argv[argc - 2];
    
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

    if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}

    int ret = fuse_main(args.argc, args.argv, &s3m_oper, state);
    fuse_opt_free_args(&args);

    printf("OK\n");

    return ret;
}