#pragma once
#include <stdint.h>

#define RBT_BLACK   0
#define RBT_RED     1

#define RBT_RIGHT   1
#define RBT_LEFT    0

#define RBT_GET_DIR(var) (var == var->parent->children[RBT_RIGHT])

typedef struct rbt_node_st
{
    uint64_t key;
    void* value;
    uint8_t color;
    struct rbt_node_st* parent;
    struct rbt_node_st* children[2];
} __attribute__((packed)) rbt_node;

typedef struct rbt_container_st
{
    rbt_node* rb_root;
    rbt_node* rb_leftmost;
} rbt_tree;

/**
 * @brief create a new node.
 * 
 * @param parent the parent node
 * @param key the key value
 * @return rbt_node* 
 */
rbt_node* rbt_create_node(rbt_node* parent, uint64_t key);

/**
 * @brief search a key into a tree.
 * 
 * @param root the tree root
 * @param key the key to search into the tree
 * @return rbt_node* 
 */
rbt_node* rbt_search(rbt_tree* root, const uint64_t key);

/**
 * @brief return the minimum value into the RBT.
 * 
 * @param root the node from which check the minimum
 * @return rbt_node* the node with the smallest value
 */
rbt_node* rbt_minimum(rbt_tree* root);

/**
 * @brief binary search tree insert
 * 
 * @param root root node
 * @param key key to add to the tree
 */
rbt_node* rbt_insert(rbt_tree* root, uint64_t key);

/**
 * @brief delete a node from the RBT root
 * 
 * @param root the root of the tree containing that node
 * @param to_delete the node to delete
 */
void rbt_delete(rbt_tree* root, rbt_node* to_delete);

/**
 * @brief delete a key from the RBT 
 * 
 * @param root the rbt root
 * @param key the key to delete
 */
void rbt_delete_key(rbt_tree* root, uint64_t key);

/**
 * @brief print the rbt from one of it's node
 * 
 * @param root the root node to print from.
 */
void rbt_print(rbt_node* root);

/**
 * @brief Delete all element in the RBT
 * 
 * @param root 
 */
void rbt_clear_tree(rbt_tree* root);