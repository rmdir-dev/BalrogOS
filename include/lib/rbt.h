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
 * @brief 
 * 
 * @param parent 
 * @param key 
 * @return rbt_node* 
 */
rbt_node* rbt_create_node(rbt_node* parent, uint64_t key);

/**
 * @brief 
 * 
 * @param root 
 * @param key 
 * @return rbt_node* 
 */
rbt_node* rbt_search(rbt_node* root, const uint64_t key);

/**
 * @brief 
 * 
 * @param from 
 * @return rbt_node* 
 */
rbt_node* rbt_minimum(rbt_node* from);

/**
 * @brief binary search tree insert
 * 
 * @param root root node
 * @param key key to add to the tree
 */
rbt_node* rbt_insert(rbt_tree* root, uint64_t key);

/**
 * @brief 
 * 
 * @param root 
 * @param to_delete 
 */
void rbt_delete(rbt_tree* root, rbt_node* to_delete);

/**
 * @brief 
 * 
 * @param root 
 * @param key 
 */
void rbt_delete_key(rbt_tree* root, uint64_t key);

/**
 * @brief 
 * 
 * @param root 
 */
void rbt_print(rbt_node* root);