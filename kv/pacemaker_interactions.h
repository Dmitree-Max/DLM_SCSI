#include <linux/fs.h>
#include <linux/slab.h>


int get_cluster_nodes_info(int* this_node_id, char*** node_list);

struct kv_dlm_readdir_context {
	struct dir_context ctx;
	char *entries;
};
