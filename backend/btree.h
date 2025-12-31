#ifndef BTREE_H
#define BTREE_H

#include <vector>
#include <memory>
#include <cstdint>

// B-Tree node
struct BTreeNode {
    bool isLeaf;
    std::vector<uint32_t> keys;
    std::vector<int> values;
    std::vector<std::shared_ptr<BTreeNode>> children;
    
    BTreeNode(bool leaf = true) : isLeaf(leaf) {}
};

// B-Tree implementation for indexing
class BTree {
public:
    BTree(int order = 5);
    ~BTree();
    
    void insert(uint32_t key, int value);
    int search(uint32_t key);
    void remove(uint32_t key);
    
private:
    std::shared_ptr<BTreeNode> root_;
    int order_;  // Maximum number of keys per node
    
    void insertNonFull(std::shared_ptr<BTreeNode> node, uint32_t key, int value);
    void splitChild(std::shared_ptr<BTreeNode> parent, int index);
    int searchNode(std::shared_ptr<BTreeNode> node, uint32_t key);
};

#endif