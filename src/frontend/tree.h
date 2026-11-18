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

/* Construction */

/* Copies the strings. Printable nodes require a name; NULL text denotes a
 * nonterminal, while empty text denotes a token with no displayed value. */
TreeNode *tree_new(const char *name, const char *text, int line);
/* NULL arguments are ignored. Otherwise append a detached subtree root
 * (no existing parent or next sibling), transferring ownership to parent. */
void tree_add_child(TreeNode *parent, TreeNode *child);

/* Queries */

/* Queries borrow nodes. Child lookups are direct, and indices are zero-based.
 */
const TreeNode *tree_child_at(const TreeNode *node, int index);
int tree_is(const TreeNode *node, const char *name);
const TreeNode *tree_find_child(const TreeNode *node, const char *name);

/* Traversal and cleanup */

/* Both traverse the node, its descendants, and its following siblings.
 * Pass the root to operate on a whole tree. NULL is a no-op. */
void tree_print(const TreeNode *node, int depth);
void tree_free(TreeNode *node);

#endif
