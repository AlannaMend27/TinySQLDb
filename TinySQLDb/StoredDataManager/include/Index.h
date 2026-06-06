#pragma once
#include <string>
#include "Records.h"

// index -> representa un indice registrado en el system catalog

class Index {
public:
    std::string name;        // nombre del índice
    std::string tableName;   // tabla sobre la que está
    std::string columnName;  // columna indexada
    IndexType   type;        // BST o BTREE

    // Constructor vacío
    Index();

    // Constructor completo
    Index(const std::string& name, const std::string& tableName, const std::string& columnName, IndexType type);

    std::string typeToString() const;
    bool isValid() const;
};
