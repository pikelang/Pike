#ifdef PIKE_NEW_BLOCK_ALLOC
void TREE_CLASSIFY(relocate_simple)(void*, void*, ptrdiff_t);
static cb_node_t TREE_CLASSIFY(node_alloc)();
static void TREE_CLASSIFY(node_free)(cb_node_t);

# define CB_NODE_ALLOC(tree)    ((cb_node_t)ba_lalloc(&tree->allocator))
# define CB_NODE_FREE(tree, p)  ba_lfree(&tree->allocator, p)
#else
# define CB_NODE_ALLOC(tree)    ((cb_node_t)xalloc(sizeof(cb_node)))
# define CB_NODE_FREE(tree, p)    free(p)
#endif
