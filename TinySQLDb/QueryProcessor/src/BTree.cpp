#define _CRT_SECURE_NO_WARNINGS
#include "BTree.h"

// Constructor
BTree::BTree(ColumnType colType)
{
    this->root = nullptr;
    this->colType = colType;
}

// Destructor 
BTree::~BTree()
{
    this->destroyTree(this->root);
}

// Inserta un valor y su posicion en disco
bool BTree::insert(const std::string& key, long position)
{
    // verificar duplicado antes de insertar
    if (this->valueExists(key))
    {
        return false;
    }

    // si el arbol esta vacio, crear la raiz
    if (this->root == nullptr)
    {
        this->root = new BTreeNode();
        this->root->keys[0] = key;
        this->root->positions[0] = position;
        this->root->keyCount = 1;
        this->root->isLeaf = true;
        return true;
    }

    // si la raiz esta llena, hay que dividirla
    if (this->root->keyCount == BTREE_ORDER - 1)
    {
        // crear nueva raiz
        BTreeNode* newRoot = new BTreeNode();
        newRoot->isLeaf = false;
        newRoot->children[0] = this->root;

        // dividir la raiz vieja
        this->splitChild(newRoot, 0);

        // insertar en el hijo correcto
        int childIndex = 0;
        if (this->compare(newRoot->keys[0], key) < 0)
        {
            childIndex = 1;
        }
        this->insertNonFull(newRoot->children[childIndex], key, position);

        this->root = newRoot;
    }
    else
    {
        // la raiz no esta llena, insertar directamente
        this->insertNonFull(this->root, key, position);
    }

    return true;
}

// Busca un valor y retorna su posicion en disco
long BTree::search(const std::string& key)
{
    return this->searchNode(this->root, key);
}

// Verifica si un valor ya existe en el arbol
bool BTree::valueExists(const std::string& key)
{
    return this->searchNode(this->root, key) != -1;
}

// Limpia todos los nodos del arbol dejandolo vacio
void BTree::clear()
{
    // liberar todos los nodos recursivamente
    this->destroyTree(this->root);

    // reiniciar la raiz
    this->root = nullptr;
}

// retorna -1 si a < b, 0 si a == b, 1 si a > b
int BTree::compare(const std::string& a, const std::string& b)
{
    if (this->colType == TYPE_INTEGER || this->colType == TYPE_DOUBLE)
    {
        //si es numerico
        double numA = std::stod(a);
        double numB = std::stod(b);

        if (numA < numB)
        {
            return -1;
        }
        if (numA > numB)
        {
            return 1;
        }
        return 0;
    }

    //si es varchar o datetime
    if (a < b)
    {
        return -1;
    }
    if (a > b)
    {
        return 1;
    }
    return 0;
}

// Busca recursivamente un valor en el arbol
long BTree::searchNode(BTreeNode* node, const std::string& key)
{
    // caso base: nodo vacio
    if (node == nullptr)
    {
        return -1;
    }

    // buscar la posicion correcta en las claves del nodo
    int i = 0;
    while (i < node->keyCount && this->compare(key, node->keys[i]) > 0)
    {
        i++;
    }

    // verificar si encontramos la clave
    if (i < node->keyCount && this->compare(key, node->keys[i]) == 0)
    {
        return node->positions[i];
    }

    // si es hoja y no encontramos, no existe
    if (node->isLeaf)
    {
        return -1;
    }

    // buscar en el hijo correspondiente
    return this->searchNode(node->children[i], key);
}

// Inserta en un nodo que no esta lleno
void BTree::insertNonFull(BTreeNode* node, const std::string& key, long position)
{
    // indice del ultimo elemento
    int lastIndex = node->keyCount - 1;

    if (node->isLeaf)
    {
        // desplazar claves mayores hacia la derecha para hacer espacio
        while (lastIndex >= 0 && this->compare(key, node->keys[lastIndex]) < 0)
        {
            node->keys[lastIndex + 1] = node->keys[lastIndex];
            node->positions[lastIndex + 1] = node->positions[lastIndex];
            lastIndex--;
        }

        // insertar la nueva clave
        node->keys[lastIndex + 1] = key;
        node->positions[lastIndex + 1] = position;
        node->keyCount++;
    }
    else
    {
        // encontrar el hijo donde debe ir la clave
        while (lastIndex >= 0 && this->compare(key, node->keys[lastIndex]) < 0)
        {
            lastIndex--;
        }

        int childIndex = lastIndex + 1;

        // si el hijo esta lleno, dividirlo primero
        if (node->children[childIndex]->keyCount == BTREE_ORDER - 1)
        {
            this->splitChild(node, childIndex);

            // despues del split, determinar en cual de los dos hijos insertar
            if (this->compare(key, node->keys[childIndex]) > 0)
            {
                childIndex++;
            }
        }

        this->insertNonFull(node->children[childIndex], key, position);
    }
}

// Divide el hijo en la posicion childIndex del nodo padre
void BTree::splitChild(BTreeNode* parent, int childIndex)
{
    // nodo hijo que esta lleno
    BTreeNode* fullChild = parent->children[childIndex];

    // nuevo nodo que recibira la mitad derecha
    BTreeNode* newChild = new BTreeNode();
    newChild->isLeaf = fullChild->isLeaf;

    // indice de la clave del medio
    int midIndex = (BTREE_ORDER - 1) / 2;

    // copiar la mitad derecha al nuevo nodo
    newChild->keyCount = fullChild->keyCount - midIndex - 1;
    for (int i = 0; i < newChild->keyCount; i++)
    {
        newChild->keys[i] = fullChild->keys[midIndex + 1 + i];
        newChild->positions[i] = fullChild->positions[midIndex + 1 + i];
    }

    // copiar hijos si no es hoja
    if (!fullChild->isLeaf)
    {
        for (int i = 0; i <= newChild->keyCount; i++)
        {
            newChild->children[i] = fullChild->children[midIndex + 1 + i];
        }
    }

    // actualizar el contador del hijo lleno
    fullChild->keyCount = midIndex;

    // hacer espacio en el padre para el nuevo hijo
    for (int i = parent->keyCount; i > childIndex; i--)
    {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[childIndex + 1] = newChild;

    // subir la clave del medio al padre
    for (int i = parent->keyCount - 1; i >= childIndex; i--)
    {
        parent->keys[i + 1] = parent->keys[i];
        parent->positions[i + 1] = parent->positions[i];
    }
    parent->keys[childIndex] = fullChild->keys[midIndex];
    parent->positions[childIndex] = fullChild->positions[midIndex];
    parent->keyCount++;
}

// Libera recursivamente todos los nodos
void BTree::destroyTree(BTreeNode* node)
{
    if (node == nullptr)
    {
        return;
    }

    // liberar hijos primero
    if (!node->isLeaf)
    {
        for (int i = 0; i <= node->keyCount; i++)
        {
            this->destroyTree(node->children[i]);
        }
    }

    delete node;
}