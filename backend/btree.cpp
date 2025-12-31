#include "btree.h"

BTree::BTree(int order) : order_(order), root_(nullptr) {
    root_ = std::make_shared<BTreeNode>(true);
}

BTree::~BTree() {
}

void BTree::insert(uint32_t key, int value) {
    if (root_->keys.size() == (size_t)(order_ - 1)) {
        // Root is full, need to split
        auto newRoot = std::make_shared<BTreeNode>(false);
        newRoot->children.push_back(root_);
        splitChild(newRoot, 0);
        root_ = newRoot;
    }
    
    insertNonFull(root_, key, value);
}

int BTree::search(uint32_t key) {
    return searchNode(root_, key);
}

void BTree::remove(uint32_t key) {
    // Simplified remove - not fully implemented for prototype
    // In production, implement full B-Tree deletion
}

void BTree::insertNonFull(std::shared_ptr<BTreeNode> node, uint32_t key, int value) {
    int i = (int)node->keys.size() - 1;
    
    if (node->isLeaf) {
        // Insert into leaf node
        node->keys.push_back(0);
        node->values.push_back(0);
        
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
    } else {
        // Find child to insert into
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        
        if (node->children[i]->keys.size() == (size_t)(order_ - 1)) {
            splitChild(node, i);
            
            if (key > node->keys[i]) {
                i++;
            }
        }
        
        insertNonFull(node->children[i], key, value);
    }
}

void BTree::splitChild(std::shared_ptr<BTreeNode> parent, int index) {
    auto fullChild = parent->children[index];
    auto newChild = std::make_shared<BTreeNode>(fullChild->isLeaf);
    
    int mid = order_ / 2;
    
    // Move half of keys to new node
    newChild->keys.assign(fullChild->keys.begin() + mid + 1, fullChild->keys.end());
    newChild->values.assign(fullChild->values.begin() + mid + 1, fullChild->values.end());
    
    if (!fullChild->isLeaf) {
        newChild->children.assign(fullChild->children.begin() + mid + 1, fullChild->children.end());
        fullChild->children.resize(mid + 1);
    }
    
    // Move middle key up to parent
    parent->keys.insert(parent->keys.begin() + index, fullChild->keys[mid]);
    parent->values.insert(parent->values.begin() + index, fullChild->values[mid]);
    parent->children.insert(parent->children.begin() + index + 1, newChild);
    
    // Truncate original child
    fullChild->keys.resize(mid);
    fullChild->values.resize(mid);
}

int BTree::searchNode(std::shared_ptr<BTreeNode> node, uint32_t key) {
    if (!node) {
        return -1;
    }
    
    int i = 0;
    while (i < (int)node->keys.size() && key > node->keys[i]) {
        i++;
    }
    
    if (i < (int)node->keys.size() && key == node->keys[i]) {
        return node->values[i];
    }
    
    if (node->isLeaf) {
        return -1;
    }
    
    return searchNode(node->children[i], key);
}