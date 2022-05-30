//
//  Beachline.cpp
//  FortuneAlgo
//
//  Created by Dmytro Kotsur on 06/05/2018.
//  Copyright © 2018 Dmytro Kotsur. All rights reserved.
//

#include "Node.hpp"

namespace beachline {

    
    BLNode::BLNode(const std::pair<int,int>& _indices,
                   BLNodePtr _left,
                   BLNodePtr _right,
                   BLNodePtr _parent,
                   int _height) : indices(_indices), left(_left), right(_right),
                                  parent(_parent), height(_height),
                                  next(nullptr), prev(nullptr)
				  {
				  }

    /**
     Connect as a list
     */
    void connect(BLNodePtr prev, BLNodePtr next) {
        prev->next = next;
        next->prev = prev;
    }


    /**
     Check if the node is a root node
     */
    bool is_root(BLNodePtr node) {
        return node->parent == nullptr;
    }


    /**
     Get height of the node
     */
    int get_height(BLNodePtr node) {
        if (node == nullptr) return 0;
        return node->height;
    }


    /**
     Update height of the node
     */
    void update_height(BLNodePtr node) {
        if (node == nullptr)
            return;
        node->height = std::max(get_height(node->left), get_height(node->right)) + 1;
    }


    /**
     Get balance of the node (difference between the height of left and right subtrees)
     */
    int get_balance(BLNodePtr node) {
        return get_height(node->left) - get_height(node->right);
    }


    /**
     Performs rotation of a tree around `node` such that it goes to the left subtree
     */
    BLNodePtr rotate_left(BLNodePtr node) {
        
        if (node == nullptr)
            return nullptr;
        
        if (node->right == nullptr)
            return node;
        
        // get right node, which becomes a new root node
        BLNodePtr rnode = node->right;
        
        // establish connections with a root node if threre is one
        if (!is_root(node)) {
            if (node->parent->left == node) {
                node->parent->left = rnode;
            } else {
                node->parent->right = rnode;
            }
        }
        rnode->parent = node->parent;
        
        // connect right subtree of the left child as a left subtree of `node`
        node->right = rnode->left;
        if (rnode->left != nullptr) {
            rnode->left->parent = node;
        }
        
        // connect `node` as a right child of it's child
        rnode->left = node;
        node->parent = rnode;
        
        // update height attribute
        update_height(node);
        update_height(rnode);
        update_height(rnode->parent);
        
        return rnode;
    }


    /**
     Performs rotation of a tree around `node` such that it goes to the right subtree
     */
    BLNodePtr rotate_right(BLNodePtr node) {
        
        if (node == nullptr)
            return nullptr;
        
        if (node->left == nullptr)
            return node;
        
        // left node becomes root node of subtree
        BLNodePtr lnode = node->left;
        
        // establish connections with a root node if threre is one
        if (!is_root(node)) {
            if (node->parent->left == node) {
                node->parent->left = lnode;
            } else {
                node->parent->right = lnode;
            }
        }
        lnode->parent = node->parent;
        
        // connect right subtree of the left child as a left subtree of `node`
        node->left = lnode->right;
        if (lnode->right != nullptr) {
            lnode->right->parent = node;
        }
        
        // connect `node` as a right child of it's child
        lnode->right = node;
        node->parent = lnode;
        
        // update height attribute
        update_height(node);
        update_height(lnode);
        update_height(lnode->parent);
        
        return lnode;
    }

    /**
     Remove a disappearing arc related to a circle event.
     The function rebalances the tree and returns the pointer to a new root node.
     */
    BLNodePtr remove(BLNodePtr leaf) {
        
        // General idea behind this code:
        // This function removes the leaf and it's parent corresponding to one breakpoint.
        // It moves up in a tree and rebalaces it. If function encounters second breakpoint,
        // it replaces this breakpoint with a new one. This is possible because when the circle
        // event appears, two breakpoints coincide and thus they should be represented by one.
        
        if (leaf == nullptr)
            return nullptr;
        
        BLNodePtr parent = leaf->parent, grandparent = parent->parent;
        std::pair<int,int> bp1(leaf->prev->get_id(), leaf->get_id());
        std::pair<int,int> bp2(leaf->get_id(), leaf->next->get_id());
        std::pair<int,int> other_bp;
        
        assert(leaf->next != nullptr);
        assert(leaf->prev != nullptr);
        assert(parent != nullptr);
        assert(grandparent != nullptr);
        
        assert(parent->has_indices(bp1) || parent->has_indices(bp2));
        
        if (parent->has_indices(bp1)) {
            other_bp = bp2;
        } else if (parent->has_indices(bp2)) {
            other_bp = bp1;
        }
        
        BLNodePtr other_subtree;
        if (parent->left == leaf)
            other_subtree = parent->right;
        else
            other_subtree = parent->left;
        
        other_subtree->parent = grandparent;
        if (grandparent->left == parent) {
            grandparent->left = other_subtree;
        } else {
            grandparent->right = other_subtree;
        }
        
        BLNodePtr new_root = grandparent;
        // Go up and rebalance the whole tree
        while (grandparent != nullptr) {
            if (grandparent->has_indices(other_bp))
                grandparent->indices = std::make_pair(leaf->prev->get_id(), leaf->next->get_id());
            // update height of a node
            update_height(grandparent);
            // calculate balance factor of a node
            int balance = get_balance(grandparent);
            if (balance > 1) { // left subtree is higher than right subtree by more than 1
                if (grandparent->left != nullptr && !grandparent->left->is_leaf() && get_balance(grandparent->left) < 0) {
                    grandparent->left = rotate_left(grandparent->left);
                }
                grandparent = rotate_right(grandparent);
            } else if (balance < -1) { // right subtree is lower than left subtree by more than 1
                if (grandparent->right != nullptr && !grandparent->right->is_leaf() && get_balance(grandparent->right) > 0) {
                    grandparent->right = rotate_right(grandparent->right);
                }
                grandparent = rotate_left(grandparent);
            }
            
            //_validate(grandparent);
            
            new_root = grandparent;
            grandparent = grandparent->parent;
        }
        
        // Connect previous with next leaf
        connect(leaf->prev, leaf->next);
        
        //_check_balance(new_root);
        
        return new_root;
    }

    bool _validate(BLNodePtr node) {
        
        if (node == nullptr)
            return true;
        
        if (node->is_leaf()) {
            if (node->left != nullptr || node->right != nullptr) {
                std::cout << "LEAF NOT A LEAF: " << node->indices.first << ", " << node->indices.second << std::endl;
                return false;
            }
        } else {
            if (node->left == nullptr || node->right == nullptr) {
                std::cout << " BP WITHOUT LEAF: " << node->indices.first << ", " << node->indices.second << std::endl;
                return false;
            }
        }
        return true;
    }

    bool _check_balance(BLNodePtr node) {
        if (node == nullptr) return true;
        if (_check_balance(node->left) && _check_balance(node->right)) {
            if (fabs(get_balance(node)) > 1) {
                
                std::cout << "+unbalanced (" << node->indices.first << ", " << node->indices.second << ")" << std::endl;
                
                return false;
            }
        }
        return true;
    }


    /**
     Print tree
     */
    void print_tree(BLNodePtr root, int width) {
        
        if (root == nullptr)
            return;
        
        std::vector<std::vector<BLNodePtr>> layers(root->height);
        
        layers[0].push_back(root);
        int size = 2;
        for (int i = 1; i < root->height; ++i) {
            layers[i].resize(size);
            for (int j = 0; j < layers[i-1].size(); ++j) {
                if (layers[i-1][j] != nullptr) {
                    layers[i][2*j] = layers[i-1][j]->left;
                    layers[i][2*j+1] = layers[i-1][j]->right;
                }
            }
            size *= 2;
        }
        
        size /= 2;
        for (int i = 0; i < root->height; ++i) {
            for (size_t j = 0; j < layers[i].size(); ++j) {
                if (layers[i][j] != nullptr)
                    std::cout << std::setw(width * size) << "<" << layers[i][j]->indices.first << ", " << layers[i][j]->indices.second << ">";
                else
                    std::cout << std::setw(width * size) << "      ";
            }
            std::cout << std::endl;
            size /= 2;
        }
    }

}


