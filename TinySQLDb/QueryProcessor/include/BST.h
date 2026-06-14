#pragma once
#include <string>
#include "Records.h"

// BSTNode -> nodo del arbol binario de busqueda
// guarda el valor de la columna indexada y la posicion de esa fila en disco

// una columna esta indexada si tiene un indice creado, si uno hace CREATE INDEX idx ON Estudiante(ID)
// la columna ID estaria indexada


struct BSTNode {

    std::string key; // valor de la columna indexada
    long position; // posicion en bytes de la fila en el archivo .bin
    BSTNode* left; 
    BSTNode* right;

    // Constructor
    BSTNode(const std::string& key, long position)
    {
        this->key = key;
        this->position = position;
        this->left = nullptr;
        this->right = nullptr;
    }
};

// BST -> arbol binario de busqueda para indexar columnas de una tabla
// mapea valores de una columna a posiciones en disco
class BST {
public:

    // Constructor y destructor, recibe el tipo de columna de una vez para comparar int double...
    BST(ColumnType colType);
    ~BST();

    //metodos relacionados a los nodos
    bool insert(const std::string& key, long position);
    long search(const std::string& key);
    bool valueExists(const std::string& key);

private:

    BSTNode* root;
    ColumnType colType; // tipo de dato de la columna indexada

    // metodos privados
    int compare(const std::string& a, const std::string& b);
    BSTNode* insertNode(BSTNode* node, const std::string& key, long position, bool& inserted);
    long searchNode(BSTNode* node, const std::string& key);
    void destroyTree(BSTNode* node);
};