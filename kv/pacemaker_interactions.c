#include "pacemaker_interactions.h"

static int kv_read_file(const char *path, char *buf, int buf_len)
{
	struct file *f;
	loff_t pos;
	int ret;

	f = filp_open(path, 0, 0400);
	if (IS_ERR(f)) {
		ret = PTR_ERR(f);
		goto out;
	}
	pos = 0;
	ret = kernel_read(f, buf, buf_len, &pos);
	if (ret >= 0)
		buf[min(ret, buf_len - 1)] = '\0';
	filp_close(f, NULL);
out:
	return ret;
}

static int kv_dlm_filldir(struct dir_context *arg, const char *name_arg,
			    int name_len, loff_t curr_pos, u64 inode,
			    unsigned int dtype)
{
	char *p, *q, name[64];
	struct kv_dlm_readdir_context *ctx = container_of(arg, typeof(*ctx), ctx);
	char **entries = &ctx->entries;

	int i;

	snprintf(name, sizeof(name), "%.*s", name_len, name_arg);
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || !*entries)
		goto out;
	for (p = *entries; *p; p += strlen(p) + 1)
		;
	i = p - *entries;
	q = *entries;
	*entries = krealloc(q, i + strlen(name) + 2, GFP_KERNEL);
	if (!*entries) {
		kfree(q);
		goto out;
	}
	strcpy(*entries + i, name);
	i += strlen(name);
	(*entries)[i + 1] = '\0';

out:
	return *entries ? 0 : -ENOMEM;
}

int get_cluster_nodes_info(int* this_node_id, char*** node_list)
{
    static const char comms_dir[] = "/sys/kernel/config/dlm/cluster/comms";
    char *p, *entries = kzalloc(1, GFP_KERNEL);
    int ret = 0;
    struct file *comms;
	uint32_t *new;
	int num_nodes = 0;
	int i;

	comms = filp_open(comms_dir, 0, 0400);
	if (IS_ERR(comms)) {
		ret = PTR_ERR(comms);
		goto out;
	}


	struct kv_dlm_readdir_context ctx = {
		.ctx = {
			.actor = kv_dlm_filldir,
		},
		.entries = entries,
	};
	ret = iterate_dir(comms, &ctx.ctx);
	entries = ctx.entries;

	filp_close(comms, NULL);
	ret = -ENOMEM;
	if (!entries)
		goto out;

	for (p = entries; *p; p += strlen(p) + 1)
	{
		num_nodes++;
		pr_info("dlm : entries : %s\n", p);
	}

	*node_list = kcalloc(num_nodes, sizeof(char *), GFP_KERNEL);
	i = 0;

	for (p = entries; *p; p += strlen(p) + 1)
	{
		(*node_list)[i] = kcalloc(strlen(p), sizeof(char), GFP_KERNEL);
		strcpy((*node_list)[i], p);
	}

out:
	kfree(entries);
	return ret;
}


