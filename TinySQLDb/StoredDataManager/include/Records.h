#pragma once
# include<cstdint>


// orden del arbol B cada nodo tiene maximo ORDER-1 claves y ORDER hijos
const int BTREE_ORDER = 3;

// Records.h contiene structs para leer y escribir archivos binarios en el disco
// Establece el formato que tienen los archivos binarios



// enum ColumnType: Indica el tipo de de dato almacenado en columna en una tabla
enum ColumnType : uint8_t {
    TYPE_INTEGER = 0,
    TYPE_DOUBLE = 1,
    TYPE_VARCHAR = 2,
    TYPE_DATETIME = 3
};

// enum IndexType: Indica el tipo de indice que tiene una tabla en caso de haberse configurado
enum IndexType : uint8_t {
    INDEX_BST = 0,
    INDEX_BTREE = 1
};

// enum ColumnConstraint: indica si la columna tiene una restriccion especial
enum ColumnConstraint : uint8_t {
    CONSTRAINT_NONE = 0,
    CONSTRAINT_PRIMARY_KEY = 1,
    CONSTRAINT_UNIQUE = 2
};

// indicar al compilador que no agregue bytes de padding entre los campos del struct
#pragma pack(push, 1)

// DatabaseRecord — 51 bytes exactos 

struct DatabaseRecord {
    uint8_t flag;     // 1 = activa, 0 = eliminada, se desactiva es dificil con grandes bases de datos
    char    name[50]; // nombre 0
};

// TableRecord — 105 bytes exactos 
struct TableRecord {
    uint8_t  flag;
    char     tableName[50];
    char     dbName[50];
    uint32_t rowSize; // tamaño en bytes de una fila completa
};

// ColumnRecord — 116 bytes exactos 
struct ColumnRecord {
    uint8_t  flag;
    char     tableName[50];
    char     columnName[50];
    uint8_t  type;      // usa ColumnType enum
    uint32_t size;      // bytes que ocupa en la fila
    uint32_t offset;    // en qué byte de la fila empieza esta columna
    uint32_t position;  // orden: 0, 1, 2... para reconstruir la fila
    uint8_t nullable;   // 1 = puede ser NULL, 0 no NULL
    uint8_t constraint; //usa ColumnConstraint enum
};

// IndexRecord — 152 bytes exactos (1 + 50 + 50 + 50 + 1)
struct IndexRecord {
    uint8_t flag;
    char    indexName[50];
    char    tableName[50];
    char    columnName[50];
    uint8_t type; // usa IndexType enum
};

#pragma pack(pop)
// Restaura el comportamiento normal del compilador