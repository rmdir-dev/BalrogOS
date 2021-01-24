#include "lib/rbt.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

rbt_node* rbt_create_node(rbt_node* parent, uint64_t key)
{
    //TODO first implement malloc!
    rbt_node* ret;// = malloc(sizeof(rbt_node));

    printf("!!!!! MALLOC REQUIRE !!!!!\n");
    while(1)
    {}

    ret->key = key;
    ret->color = RBT_RED;
    ret->children[RBT_LEFT] = NULL;
    ret->children[RBT_RIGHT] = NULL;
    ret->parent = parent;

    return ret;
}

rbt_node* rbt_search(rbt_node* root, const uint64_t key)
{
    rbt_node* current_node = root;

    while(current_node)
    {
        if(current_node->key == key)
        {
            return current_node;
        }

        current_node = current_node->children[key > current_node->key];
    }

    return NULL;
}

rbt_node* rbt_minimum(rbt_node* from)
{
    while(from->children[RBT_LEFT])
    {
        from = from->children[RBT_LEFT];
    }
    return from;
}

static inline void __rotate(rbt_tree* root, rbt_node* rotating_node, const uint8_t dir)
{
    rbt_node* pivot = rotating_node->children[!dir];

    rotating_node->children[!dir] = pivot->children[dir];
    if(rotating_node->children[!dir])
    {
        rotating_node->children[!dir]->parent = rotating_node;
    }

    pivot->parent = rotating_node->parent;
    if(!rotating_node->parent)
    {
        root->rb_root = pivot;
    } else if(rotating_node == rotating_node->parent->children[dir])
    {
        rotating_node->parent->children[dir] = pivot;
    } else 
    {
        rotating_node->parent->children[!dir] = pivot;
    }

    pivot->children[dir] = rotating_node;
    rotating_node->parent = pivot;
}

/**
 * @brief Fix the nodes order and color in function of Red Black tree rules
 * 
 * @param root root node
 * @param ptr the latest entry
 */
static void __insert_fixup(rbt_tree* root, rbt_node* ptr)
{
    rbt_node* parent_ptr = NULL;
    rbt_node* grand_parent_ptr = NULL;

    while((ptr != root->rb_root)
        && (ptr->color != RBT_BLACK)
        && (ptr->parent->color == RBT_RED))
    {
        parent_ptr = ptr->parent;
        grand_parent_ptr = parent_ptr->parent;
        uint8_t dir = RBT_GET_DIR(parent_ptr);

        rbt_node* uncle_ptr = grand_parent_ptr->children[!dir];

        if(uncle_ptr && uncle_ptr->color == RBT_RED)
        {
            parent_ptr->color = RBT_BLACK;
            uncle_ptr->color = RBT_BLACK;
            grand_parent_ptr->color = RBT_RED;
            ptr = grand_parent_ptr;
        } else 
        {
            if(ptr == parent_ptr->children[!dir])
            {
                __rotate(root, parent_ptr, dir);
                ptr = parent_ptr;
                parent_ptr = ptr->parent;
            }

            __rotate(root, ptr->parent->parent, !dir);
            uint8_t col = parent_ptr->color;
            parent_ptr->color = grand_parent_ptr->color;
            grand_parent_ptr->color = col;
            ptr = parent_ptr;
        }
    }

    root->rb_root->color = RBT_BLACK;
}

/**
 * @brief binary search tree insert
 * 
 * @param root root node
 * @param key key to add to the tree
 */
void rbt_insert(rbt_tree* root, uint64_t key)
{
    if(!root->rb_root)
    {
        root->rb_root = rbt_create_node(0, key);
        root->rb_root->color = RBT_BLACK;
        return;
    }

    rbt_node* parent_branch = NULL;
    rbt_node* current_branch = root->rb_root;
    uint8_t dir = 0;

    while(current_branch)
    {
        parent_branch = current_branch;

        if(current_branch->key == key)
        {
            return;
        }

        dir = ( key > current_branch->key );
        current_branch = current_branch->children[dir];
    }

    current_branch = rbt_create_node(parent_branch, key);

    if(parent_branch)
    {
        parent_branch->children[dir] = current_branch;
    }

    __insert_fixup(root, current_branch);
}

static __always_inline void __delete_fixup(rbt_tree* root, rbt_node* entry)
{

}

static __always_inline void rbt_transplant(rbt_tree* root, rbt_node* left, rbt_node* right)
{
    
}

void rbt_delete(rbt_tree* root, rbt_node* to_delete)
{
    __delete_fixup(root, to_delete);
}

void rbt_delete_key(rbt_tree* root, uint64_t key)
{
    rbt_delete(root, rbt_search(root->rb_root, key));
}

void rbt_print(rbt_node* root)
{
    if(root != NULL)
    {
        rb_print(root->children[RBT_LEFT]);
        printf("%lu\n", root->key);
        rb_print(root->children[RBT_RIGHT]);
    }
}