#define _CRT_SECURE_NO_WARNINGS
#include "BST.h"
#include <cstdlib>

// Constructor, recibe el tipo para comparar de una
BST::BST(ColumnType colType)
{
    this->root = nullptr;
    this->colType = colType;
}

// Destructor
BST::~BST()
{
    this->destroyTree(this->root);
}

// Inserta un valor y su posicion en disco, false si ya existe
bool BST::insert(const std::string& key, long position)
{
    bool inserted = false;
    this->root = this->insertNode(this->root, key, position, inserted);
    return inserted;
}

// Busca un valor y retorna su posicion en disco, -1 si no existe
long BST::search(const std::string& key)
{
    return this->searchNode(this->root, key);
}

// Verifica si un valor ya existe en el arbol
bool BST::valueExists(const std::string& key)
{
    return this->searchNode(this->root, key) != -1;
}

// Limpia todos los nodos del arbol dejandolo vacio
void BST::clear()
{
    this->destroyTree(this->root);
    this->root = nullptr;
}


// retorna -1 si a < b, 0 si a == b, 1 si a > b
int BST::compare(const std::string& a, const std::string& b)
{
    // si el valor es numerico, comparamos como numeros
    if (colType == TYPE_INTEGER || colType == TYPE_DOUBLE)
    {
        //pasar de string a numero
        double numA = std::stod(a);
        double numB = std::stod(b);

        // comparacion
        if (numA < numB) {
            return -1;
        }
        if (numA > numB) {
            return  1;
        }
        return 0;
    }

    // para VARCHAR y DATETIME comparar como strings
    if (a < b) {
        return -1;
    }
    if (a > b) {
        return  1;
    }
    return 0;
}

// Inserta recursivamente un nodo en el arbol
BSTNode* BST::insertNode(BSTNode* node, const std::string& key, long position, bool& inserted)
{
    // caso1, lugar vacio donde insertar
    if (node == nullptr)
    {
        inserted = true;
        return new BSTNode(key, position);
    }

    int cmp = this->compare(key, node->key);

    // si es menor
    if (cmp < 0){
        // ir a la izquierda
        node->left = this->insertNode(node->left, key, position, inserted);
    }
    //si es mayor
    else if (cmp > 0){
        // ir a la derecha
        node->right = this->insertNode(node->right, key, position, inserted);
    }
    else {
        // el valor ya existe no lo inserta
        inserted = false;
    }

    return node;
}

// Busca recursivamente un valor y retorna su posicion en disco
long BST::searchNode(BSTNode* node, const std::string& key)
{
    // si no se encontro
    if (node == nullptr)
    {
        return -1;
    }

    int cmp = this->compare(key, node->key);

    //si el valor ya es el que quiero
    if (cmp == 0)
    {
        //retornar la posicion en disco
        return node->position;
    }
    else if (cmp < 0)
    {
        // buscar en el lado izquierdo
        return this->searchNode(node->left, key);
    }
    else
    {
        // buscar en el lado derecho
        return this->searchNode(node->right, key);
    }
}

// Libera recursivamente todos los nodos del arbol
void BST::destroyTree(BSTNode* node)
{
    if (node == nullptr)
    {
        return;
    }

    // liberar hijos antes que el padre
    this->destroyTree(node->left);
    this->destroyTree(node->right);
    delete node;
}