#ifdef PIKE_NEW_BLOCK_ALLOC
void TREE_CLASSIFY(relocate_simple)(void*, void*, ptrdiff_t);
static cb_node_t TREE_CLASSIFY(node_alloc)();
static void TREE_CLASSIFY(node_free)(cb_node_t);

cmod_DEFINE_EVAL(CB_NODE_ALLOC, TREE_CLASSIFY(node_alloc))
cmod_DEFINE_EVAL(CB_NODE_FREE, TREE_CLASSIFY(node_free))
#else
# define CB_NODE_ALLOC()    ((cb_node_t)xalloc(sizeof(cb_node)))
# define CB_NODE_FREE(p)    free(p)
#endif
