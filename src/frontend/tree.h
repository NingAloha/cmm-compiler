#ifndef TREE_H
#define TREE_H

typedef struct TreeNode {
    char *name;
    char *text;
    int line;
    struct TreeNode *first_child;
    struct TreeNode *last_child;
    struct TreeNode *next_sibling;
} TreeNode;

TreeNode *tree_new(const char *name, const char *text, int line);
void tree_add_child(TreeNode *parent, TreeNode *child);
void tree_print(const TreeNode *node, int depth);
void tree_free(TreeNode *node);

#endif
