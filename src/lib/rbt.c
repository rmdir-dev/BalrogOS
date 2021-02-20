#include "lib/rbt.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "BalrogOS/Memory/kheap.h"

#define RBT_GET_DIR(var)    (var == var->parent->children[RBT_RIGHT])
#define RBT_IS_RED(var)     (var != NULL && var->color)

rbt_node* rbt_create_node(rbt_node* parent, uint64_t key)
{
    rbt_node* ret = vmalloc(sizeof(rbt_node));

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
static inline void __insert_fixup(rbt_tree* root, rbt_node* ptr)
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

rbt_node* rbt_insert(rbt_tree* root, uint64_t key)
{
    if(!root->rb_root)
    {
        root->rb_root = rbt_create_node(0, key);
        root->rb_root->color = RBT_BLACK;
        return root->rb_root;
    }

    rbt_node* parent_branch = NULL;
    rbt_node* current_branch = root->rb_root;
    uint8_t dir = 0;

    while(current_branch)
    {
        parent_branch = current_branch;

        if(current_branch->key == key)
        {
            return current_branch;
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

    return current_branch;
}

static inline void __delete_fixup(rbt_tree* root, rbt_node* unbalanced_entry, uint8_t dir)
{
    while(unbalanced_entry)
    {
        rbt_node* unbalanced_sibling = unbalanced_entry->children[!dir];

        if(RBT_IS_RED(unbalanced_sibling))
        {
            /*
                Case 1 : Parent is BLACK and sibling is RED
            */
            __rotate(root, unbalanced_entry, dir);
            unbalanced_entry->color = RBT_RED;
            unbalanced_sibling->color = RBT_BLACK;
        } else if(RBT_IS_RED(unbalanced_sibling->children[!dir]))
        {
            /*
                Case 2 :
            */
            unbalanced_sibling->children[!dir]->color = RBT_BLACK;
            unbalanced_sibling->color = unbalanced_entry->color;
            unbalanced_entry->color = RBT_BLACK;
            __rotate(root, unbalanced_entry, dir);
            return;
        } else if(RBT_IS_RED(unbalanced_sibling->children[dir]))
        {
            /*
                Case 3 :
            */
            unbalanced_sibling->children[dir]->color = RBT_BLACK;
            unbalanced_sibling->color = RBT_RED;

            __rotate(root, unbalanced_sibling, !dir);
        } else if(unbalanced_entry->color)
        {
            /*
                Case 4 : 
            */
            unbalanced_entry->color = RBT_BLACK;
            unbalanced_sibling->color = RBT_RED;
            return;
        } else 
        {
            /*
                Case 5 :
            */
            unbalanced_sibling->color = RBT_RED;
            if(unbalanced_entry->parent)
            {
                dir = RBT_GET_DIR(unbalanced_entry);
            }

            unbalanced_entry = unbalanced_entry->parent;
        }
    }
}

void rbt_delete(rbt_tree* root, rbt_node* to_delete)
{

    if(to_delete != NULL)
    {
        rbt_node* unbalanced_parent = NULL;

        uint8_t dir = RBT_LEFT;
        /*
            If the node has two children
        */
        if(to_delete->children[RBT_LEFT]  && to_delete->children[RBT_RIGHT])
        {
            /*
                search for the successor.
                we will search the left most value of the right child.
                The left most value of the right child will :
                    - greater than left child
                    - smaller than right child

                So it will replace this node perfectly.
            */
            rbt_node* successor = to_delete->children[RBT_RIGHT];
            while (successor->children[RBT_LEFT])
            {
                successor = successor->children[RBT_LEFT];
            }
            
            /*
                Once we found the successor we replace the value of 
                the node we want to delete with it's value.
                Without changing it's color.
            */

            to_delete->key = successor->key;
            to_delete->value = successor->value;

            /*
                we then replace the node to delete by the successor.
                So we will now free the successor instead now that it's 
                value is saved into the original node.
            */
            to_delete = successor;
        }

        /*
            Now the node will either have one or zero child.

            We will determin if the node has one child left.

            to_delete->children[RBT_LEFT] == NULL :
                - return 0 if RBT_LEFT is not NULL
                - return 1 if RBT_LEFT is NULL

            If RBT_RIGHT is also null then promoted will be a pointer to NULL
        */
        rbt_node* promoted = to_delete->children[to_delete->children[RBT_LEFT] == NULL];

        /*
            If to_delete is the root node we reassign the tree root 
            to promoted.
        */
        if(to_delete->parent == NULL)
        {
            root->rb_root = promoted;
        } else 
        {
            /*
                else we replace to_delete by promoted.
            */
            dir = RBT_GET_DIR(to_delete);
            to_delete->parent->children[dir] = promoted;
        }

        /*
            We then replace promoted parent if promoted is not NULL
        */
        if(promoted)
        {
            promoted->parent = to_delete->parent;
        }

        /*
        */
        if(to_delete->color != RBT_IS_RED(promoted))
        {
            if(!to_delete->color)
            {
                promoted->color ^= RBT_RED;
            }
            vmfree(to_delete);

            return;
        } else 
        {
            /*
            */
            unbalanced_parent = to_delete->parent;
            vmfree(to_delete);

            if(unbalanced_parent)
            {
                unbalanced_parent->children[dir] = NULL;
            }
        }

        __delete_fixup(root, unbalanced_parent, dir);
    }
}

void rbt_delete_key(rbt_tree* root, uint64_t key)
{
    rbt_delete(root, rbt_search(root->rb_root, key));
}


void rbt_print(rbt_node* root)
{
    if(root != NULL)
    {
        rbt_print(root->children[RBT_LEFT]);
        printf("%lu\n", root->key);
        rbt_print(root->children[RBT_RIGHT]);
    }
}