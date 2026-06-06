#include "Index.h"

// constructor
Index::Index()
    : type(INDEX_BST) {
}

Index::Index(const std::string& name, const std::string& tableName, const std::string& columnName, IndexType type)
{
    this->name = name;
    this->tableName = tableName;
    this->columnName = columnName;
    this->type = type;
}

// devuelve el tipo de index existente en la tabla
std::string Index::typeToString() const {
    switch (type) {
    case INDEX_BST:   
        return "BST";
    case INDEX_BTREE: 
        return "BTREE";
    default:          
        return "UNKNOWN";
    }
}

// verifica si el indice es valido 
bool Index::isValid() const {
    return !name.empty() && !tableName.empty() && !columnName.empty();
}
