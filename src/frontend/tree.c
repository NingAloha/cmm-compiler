#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Storage helpers */

static char *copy_string(const char *source) {
  if (source == NULL) {
    return NULL;
  }

  size_t size = strlen(source) + 1;
  char *copy = malloc(size);

  if (copy != NULL) {
    memcpy(copy, source, size);
  }

  return copy;
}

/* Construction */

TreeNode *tree_new(const char *name, const char *text, int line) {
  TreeNode *node = malloc(sizeof(*node));

  if (node == NULL) {
    return NULL;
  }

  node->name = copy_string(name);
  node->text = copy_string(text);
  node->line = line;
  node->first_child = NULL;
  node->last_child = NULL;
  node->next_sibling = NULL;

  return node;
}

void tree_add_child(TreeNode *parent, TreeNode *child) {
  if (parent == NULL || child == NULL) {
    return;
  }

  if (parent->first_child == NULL) {
    parent->first_child = child;
  } else {
    parent->last_child->next_sibling = child;
  }

  parent->last_child = child;
}

/* Traversal and cleanup */

static void print_indentation(int depth) {
  for (int level = 0; level < depth; level++) {
    printf("  ");
  }
}

static void print_node(const TreeNode *node) {
  if (strcmp(node->name, "RELOP") == 0) {
    printf("RELOP\n");
    return;
  }

  if (node->text != NULL && node->text[0] != '\0') {
    printf("%s: %s\n", node->name, node->text);
  } else if (node->text != NULL) {
    printf("%s\n", node->name);
  } else if (node->line > 0) {
    printf("%s (%d)\n", node->name, node->line);
  } else {
    printf("%s\n", node->name);
  }
}

void tree_print(const TreeNode *node, int depth) {
  if (node == NULL) {
    return;
  }

  print_indentation(depth);
  print_node(node);

  tree_print(node->first_child, depth + 1);
  tree_print(node->next_sibling, depth);
}

void tree_free(TreeNode *node) {
  if (node == NULL) {
    return;
  }

  tree_free(node->first_child);
  tree_free(node->next_sibling);

  free(node->name);
  free(node->text);
  free(node);
}
