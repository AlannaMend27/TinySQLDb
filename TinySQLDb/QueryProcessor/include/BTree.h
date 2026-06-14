#pragma once
#include <string>
#include "Records.h"

// orden del arbol B cada nodo tiene maximo ORDER-1 claves y ORDER hijos
const int BTREE_ORDER = 3;

struct BTreeNode
{

    std::string keys[BTREE_ORDER - 1];
    long positions[BTREE_ORDER - 1];
    BTreeNode* children[BTREE_ORDER];
    int keyCount;
    bool isLeaf;

    // Constructor
    BTreeNode(){

        this->keyCount = 0;
        this->isLeaf = true;

        for (int i = 0; i < BTREE_ORDER; i++){
            this->children[i] = nullptr;
        }
    }
};

class BTree {
public:

    // Destructor y Constructor 
    BTree(ColumnType colType);
    ~BTree();

    bool insert(const std::string& key, long position);
    long search(const std::string& key);
    bool valueExists(const std::string& key);

private:

    BTreeNode* root; 
    ColumnType colType; 

    //Métodos privados
    int compare(const std::string& a, const std::string& b);
    long searchNode(BTreeNode* node, const std::string& key);
    void insertNonFull(BTreeNode* node, const std::string& key, long position);
    void splitChild(BTreeNode* parent, int childIndex);
    void destroyTree(BTreeNode* node);
};