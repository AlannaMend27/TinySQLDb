#define _CRT_SECURE_NO_WARNINGS
#include "SystemCatalog.h"
#include <fstream>
#include <cstring>
#include <filesystem>
#include <iostream>

// Constructor completo
SystemCatalog::SystemCatalog(const std::string& catalogPath)
{
    this->path = catalogPath;
    this->initialize();
}

// Constructor vacio usa la ruta por defecto
SystemCatalog::SystemCatalog()
{
    this->path = CATALOG_PATH;
    this->initialize();
}

// Inicializa los archivos de system catalog en caso de que no existan
void SystemCatalog::initialize() {

    // crear las carpetas donde se guardaran los archvivos
    std::filesystem::create_directories(path);
    std::filesystem::create_directories(DATA_PATH);

    // crear los archivos en cada una de sus respectivas carpetas
    this->createFileIfNotExists(this->buildPath("SystemDatabases"));
    this->createFileIfNotExists(this->buildPath("SystemTables"));
    this->createFileIfNotExists(this->buildPath("SystemColumns"));
    this->createFileIfNotExists(this->buildPath("SystemIndexes"));


    // re-registrar bases de datos que existan en disco
    // pero no estén en el catalog (por si se borraron los archivos)
    for (const auto& entry : std::filesystem::directory_iterator(DATA_PATH))
    {
        if (entry.is_directory())
        {
            std::string dbName = entry.path().filename().string();
            if (!this->databaseExists(dbName))
            {
                Database db(dbName);
                this->registerDatabase(db);
            }
        }
    }
}

// Metodos relacionados con bases de datos

// registra una base de datos en el system catalog
bool SystemCatalog::registerDatabase(Database& db) {
    // verificar el base de datos sea valida

    if (!db.isValid()) {
        return false;
    }
    //  retornar en caso de que la base de datos exista
    if (this->databaseExists(db.name)) {
        return false;
    }

    //crear el struct de tamanio fijo (para escribirlo en el archivo)
    DatabaseRecord record;

    // limpiar la memoria de de cualquier basura
    memset(&record, 0, sizeof(DatabaseRecord));
    record.flag = 1;

    // Copiar de forma segura el string dinámico al arreglo char[] estático
    strncpy(record.name, db.name.c_str(), sizeof(record.name) - 1);

    // abrir el archivo para aniadir datos
    std::ofstream file(buildPath("SystemDatabases"), std::ios::binary | std::ios::app);

    // Escritura del bloque de bytes en el disco duro (usando file write el puntero de escritura siempre se encuentra al principio de espacio vacio)
    file.write(reinterpret_cast<const char*>(&record), sizeof(DatabaseRecord));

    return true;
}

// verifica que una base de datos exista
bool SystemCatalog::databaseExists(const std::string& name) const {
    
    // obtener la ruta de la base de datos
    std::filesystem::path pathFile = buildPath("SystemDatabases");

    // abrir el archivo binario
    std::ifstream file(pathFile, std::ios::binary);
    
    // si la apertura fallo retornar false
    if (!file.is_open()) {
        return false;
    }

    DatabaseRecord record;

    // leer secuencialmente los registros de tamanio fijo (tamanio del DatabaseRecord)
    while (file.read(reinterpret_cast<char*>(&record), sizeof(DatabaseRecord))) {
        // si el registro esta activo y el nombre coincide, retornar true
        if (record.flag == 1 && std::string(record.name) == name)
            return true;
    }
    return false;
}

// Retorna todas las bases de datos activas en el system catalog
Database* SystemCatalog::getAllDatabases(int& count) const {

    // obtener la ruta de la base de datos
    std::filesystem::path pathFile = buildPath("SystemDatabases");

    // abrir el archivo lectura
    std::ifstream file(pathFile, std::ios::binary);

    // retornar nullptr en case de tener fallo al abrirlo
    if (!file.is_open()) 
    {
        return nullptr;
    }

    // primero paso: registros activos
    DatabaseRecord record;

    while (file.read(reinterpret_cast<char*>(&record), sizeof(DatabaseRecord))) 
    {
        // si el registro esta activo contabilizarlo
        if (record.flag == 1)
        {
            count++;
        }
    }

    // si no hay registros activos, retornal nullptr
    if (count == 0) {
        return nullptr;
    }

    // crear el array dinamico con exactamente el tamanio necesario
    Database* result = new Database[count];

    // segundo paso: llenar el array
    // limpiar flag de fin de archivo y volver al inicio
    file.clear();
    file.seekg(0, std::ios::beg);

    int i = 0;

    // mientras que hayan datos que leer
    while (file.read(reinterpret_cast<char*>(&record), sizeof(DatabaseRecord))) {
        if (record.flag == 1) {
            // si se encontro un registro activo, crear un objeto de base de datos y guardarla en un array dinamico
            result[i] = Database(std::string(record.name));
            i++;
        }
    }
    return result;
}

// Metodos relacionados con tablas

// Registra una tabla y sus columnas en el system catalog
bool SystemCatalog::registerTable(const Table& table) {
    // verificar que la tabla es valida
    if (!table.isValid()) 
    {
        return false;
    }

    // verificar que la tablaexiste 
    if (this->tableExists(table.dbName, table.name))
    {
        return false;
    }

    // escribir el registro de la tabla en SystemTables
    TableRecord tableRec;

    // limpiar la memoria de de cualquier basura
    memset(&tableRec, 0, sizeof(TableRecord));

    // Configura el struct de tamaño fijo con los datos de la tabla para guardarlo en formato binario
    tableRec.flag = 1;
    tableRec.rowSize = table.rowSize;

    // Copia segura de los strings para evitar desbordamiento en el archivo binario
    strncpy(tableRec.tableName, table.name.c_str(), sizeof(tableRec.tableName) - 1);
    strncpy(tableRec.dbName, table.dbName.c_str(), sizeof(tableRec.dbName) - 1);

    // abrir archivo para escritura
    std::filesystem::path tableFilePath = buildPath("SystemTables");
    std::ofstream tableFile(tableFilePath, std::ios::binary | std::ios::app);
    if (!tableFile.is_open()) {
        return false;
    }

    // escribir en archivo
    tableFile.write(reinterpret_cast<const char*>(&tableRec), sizeof(TableRecord));

    // cerrar archivos de tablas para continuar con el de columnas
    tableFile.close();

    // escribir un registro por cada columna en SystemColumns
    std::filesystem::path colFilePath = buildPath("SystemColumns");

    // abrir archivo para escritura
    std::ofstream colFile(colFilePath, std::ios::binary | std::ios::app);
    if (!colFile.is_open()) {
        return false;
    }

    for (int i = 0; i < table.columnCount; i++) {
        // crear el struct y limpiarlo de basura rellenandolo con 0
        ColumnRecord colRec;
        memset(&colRec, 0, sizeof(ColumnRecord));

        // establecer los datos de la columna
        colRec.flag = 1;
        colRec.type = table.columns[i].type;
        colRec.size = table.columns[i].size;
        colRec.offset = table.columns[i].offset;
        colRec.position = table.columns[i].position;
        colRec.constraint = table.columns[i].constraint;
        if (table.columns[i].nullable)
        {
            colRec.nullable = 1;
        }
        else
        {
            colRec.nullable = 0;
        }

        // Copia segura de los strings para evitar desbordamiento en el archivo binario
        strncpy(colRec.tableName, table.name.c_str(), sizeof(colRec.tableName) - 1);
        strncpy(colRec.dbName, table.dbName.c_str(), sizeof(colRec.dbName) - 1);
        strncpy(colRec.columnName, table.columns[i].name.c_str(), sizeof(colRec.columnName) - 1);

        // escribir
        colFile.write(reinterpret_cast<const char*>(&colRec), sizeof(ColumnRecord));
    }
    return true;
}

// Verifica si una tabla existe en una base de datos especifica
bool SystemCatalog::tableExists(const std::string& dbName, const std::string& tableName) const {
    // obtener ruta del archivo d elas tablas
    std::filesystem::path pathFile = buildPath("SystemTables");

    // abrir archivo para lectura
    std::ifstream file(pathFile, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // mientras que se pueda leer, movernos TableRecord bytes para buscar el nombre de la tablita
    TableRecord record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(TableRecord))) {
        if (record.flag == 1 && std::string(record.tableName) == tableName && std::string(record.dbName) == dbName) {
            return true;
        }
    }
    return false;
}

// verifica si es posible insertar una fila en una tabla 
bool SystemCatalog::validationsToInsertRow(const Table table, const std::string values[], int valueCount)
{
    for (int i = 0; i < valueCount; i++)
    {
        std::cout << "Columna[" << i << "]: " << table.columns[i].name
            << " tipo: " << table.columns[i].typeToString()
            << " size: " << table.columns[i].size << std::endl;
        std::cout << "Valor[" << i << "]: '" << values[i]
            << "' longitud: " << values[i].size()
            << " compatible: " << table.columns[i].isValueCompatible(values[i])
            << std::endl;
    }

    // verificar que la tabla sea valida y que la cant valores sea igual a cant filas
    if (!table.isValid() && valueCount != (int)table.columnCount)
    {
        return false;
    }

    // validar que cada valor sea compatible con el tipo de su columna
    for (int i = 0; i < (int)table.columnCount; i++)
    {
        if (!table.columns[i].isValueCompatible(values[i]))
        {
            return false;
        }

        // si la columna es NOT NULL (nullable == false), el valor no puede venir vacio
        if (!table.columns[i].nullable && values[i].empty())
        {
            return false;
        }
    }
    return true;

}


// retorna todas las tablas activas registradas en el system catalog, de cualquier base de datos
// usado para SELECT * FROM SystemTables
Table* SystemCatalog::getAllTables(int& count) const {

    // obtener la ruta del archivo de tablas
    std::filesystem::path pathFile = buildPath("SystemTables");

    // abrir el archivo para lectura
    std::ifstream file(pathFile, std::ios::binary);

    // verificar que se abrio correctamente
    if (!file.is_open()) {
        count = 0;
        return nullptr;
    }

    // primer paso: contar registros activos
    count = 0;
    TableRecord record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(TableRecord))) {
        if (record.flag == 1) {
            count++;
        }
    }

    // si no hay tablas activas, retornar nullptr
    if (count == 0) {
        return nullptr;
    }

    // crear el arreglo dinamico con el tamanio exacto
    Table* result = new Table[count];

    // segundo paso: llenar el arreglo
    // limpiar el flag de fin de archivo y volver al inicio
    file.clear();
    file.seekg(0, std::ios::beg);

    int i = 0;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(TableRecord))) {
        if (record.flag == 1) {
            // crear una tabla "ligera" solo con nombre y base de datos
            // no se cargan columnas aqui porque SystemTables no las necesita
            result[i] = Table(std::string(record.tableName), std::string(record.dbName), nullptr, 0);
            i++;
        }
    }

    return result;
}

// retorna todas las columnas activas registradas en el system catalog, de cualquier tabla
// usado para SELECT * FROM SystemColumns
Column* SystemCatalog::getAllColumns(int& count) const {

    // obtener la ruta del archivo de columnas
    std::filesystem::path pathFile = buildPath("SystemColumns");

    // abrir el archivo para lectura
    std::ifstream file(pathFile, std::ios::binary);

    // verificar que se abrio correctamente
    if (!file.is_open()) {
        count = 0;
        return nullptr;
    }

    // primer paso: contar registros activos
    count = 0;
    ColumnRecord record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(ColumnRecord))) {
        if (record.flag == 1) {
            count++;
        }
    }

    // si no hay columnas activas, retornar nullptr
    if (count == 0) {
        return nullptr;
    }

    // crear el arreglo dinamico con el tamanio exacto
    Column* result = new Column[count];

    // segundo paso: llenar el arreglo
    // limpiar el flag de fin de archivo y volver al inicio
    file.clear();
    file.seekg(0, std::ios::beg);

    int i = 0;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(ColumnRecord))) {
        if (record.flag == 1) {
            // reutilizamos el metodo existente que convierte el record en un objeto Column
            result[i] = this->recordToColumn(record);
            i++;
        }
    }

    return result;
}

// Retorna una tabla completa con sus columnas cargadas
Table SystemCatalog::getTable(const std::string& dbName, const std::string& tableName) const {

    // verifica que exista la tabla
    if (!this->tableExists(dbName, tableName)) {
        // devolver una tabla nula
        return Table();
    }

    // contar la cantidad de columnas de esa e esta tabla
    std::filesystem::path colFilePath = buildPath("SystemColumns");

    // abrir el archivo para lectura
    std::ifstream colFile(colFilePath, std::ios::binary);

    // verificar si se abrio correctamente
    if (!colFile.is_open()) {
        return Table();
    }

    // leer ColumnRecord bytes mientras sea posible leer y haya elementos
    int count = 0;
    ColumnRecord rec;
    while (colFile.read(reinterpret_cast<char*>(&rec), sizeof(ColumnRecord))) {
        // contar todas las columnas que tienen el nombre de tabla buscado
        if (rec.flag == 1 && std::string(rec.tableName) == tableName && std::string(rec.dbName) == dbName)
            count++;
    }
    if (count == 0) {
        return Table();
    }

    // crear array dinamico y llenarlo en la segunda pasada
    Column* cols = new Column[count];

    colFile.clear();
    colFile.seekg(0, std::ios::beg);

    // hacer un arreglo dinamico con todas las columnas de la tala
    int i = 0;
    while (colFile.read(reinterpret_cast<char*>(&rec), sizeof(ColumnRecord))) {
        if (rec.flag == 1 && std::string(rec.tableName) == tableName && std::string(rec.dbName) == dbName) {
            cols[i] = this->recordToColumn(rec);
            i++;
        }
    }

    // ordenar columnas por position usando bubble sort 
    // esto es necesario para insert into, ya que al insertar los valores vienen ordenados de acuerdo a las tablas
    for (int a = 0; a < count - 1; a++) {
        for (int b = 0; b < count - a - 1; b++) {
            if (cols[b].position > cols[b + 1].position) {
                Column temp = cols[b];
                cols[b] = cols[b + 1];
                cols[b + 1] = temp;
            }
        }
    }

    // construir la tabla
    Table result(tableName, dbName, cols, count);
    delete[] cols;
    return result;
}

// Retorna todas las tablas activas de una base de datos
Table* SystemCatalog::getTablesForDatabase(const std::string& dbName) const {

     // obtener la ruta de la base de datos
     std::filesystem::path pathFile = buildPath("SystemTables");

     // abrir archivo para lectura
     std::ifstream file(pathFile, std::ios::binary);
     if (!file.is_open()) {
         return nullptr;
     }

     // contar las tablas activas en la base de datos
     int count = 0;

     // leer de tableRecord bytes mientras se haya elementos que leer en el archivo
     TableRecord record;
     while (file.read(reinterpret_cast<char*>(&record), sizeof(TableRecord))) {
        // si esta activa y el nombre d ela base de datos coincide, contablizar
        if (record.flag == 1 && std::string(record.dbName) == dbName)
             count++;
     }

        if (count == 0) {
            return nullptr;
        }

        // array dinamica del tamanio la cant de tablas de la databse
        Table* result = new Table[count];

        //llenar el array con las tablas completas
        //nos colocamos en el inicio del archivo
        file.clear();
        file.seekg(0, std::ios::beg);

        // guardamos los resultados mientras haya elementos que leer
        int i = 0;
        while (file.read(reinterpret_cast<char*>(&record), sizeof(TableRecord))) {
            if (record.flag == 1 && std::string(record.dbName) == dbName) {
                result[i] = this->getTable(dbName, std::string(record.tableName));
                i++;
            }
        }
        // retornar arreglo dinamico
        return result;
    }

    // retorna el nombre de la base de datos a la que pertenece una tabla
    std::string SystemCatalog::getDatabaseForTable(const std::string& tableName) const
    {
        // obtener la ruta del archivo de tablas
        std::filesystem::path pathFile = buildPath("SystemTables");

        // abrir archivo para lectura
        std::ifstream file(pathFile, std::ios::binary);
        if (!file.is_open())
        {
            return "";
        }

        // leer registro por registro buscando la tabla
        TableRecord record;
        while (file.read(reinterpret_cast<char*>(&record), sizeof(TableRecord)))
        {
            // si el registro esta activo y el nombre coincide, rfetornar su base de datos
            if (record.flag == 1 && std::string(record.tableName) == tableName)
            {
                return std::string(record.dbName);
            }
        }

        // si no se encontro la tabla
        return "";
    }

bool SystemCatalog::unregisterTable(const std::string& dbName, const std::string& tableName) {

        // obtener path del archivo que guarda la metadata de las tablas
        std::filesystem::path pathFile = buildPath("SystemTables");

        // abrir para leer y escribir sin truncar el archivo
        std::fstream file(pathFile, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open()) {
            return false;
        }

        // leer de tableRecord bytes mientras se haya elementos que leer en el archivo
        TableRecord record;
        bool tableFound = false;
        while (file.read(reinterpret_cast<char*>(&record), sizeof(TableRecord))) {

            // si la tabla esta activa y coincide con el nombre y la bse de datos brindada
            if (record.flag == 1 &&
                std::string(record.tableName) == tableName &&
                std::string(record.dbName) == dbName) {

                // retroceder el cursor al inicio de este registro
                file.seekp(-(std::streamoff)sizeof(TableRecord), std::ios::cur);

                // marcar como no disponible
                record.flag = 0;

                // volver a escribir record pero con el cambio en la flag( para que el cambio se guarde en disco)
                file.write(reinterpret_cast<const char*>(&record), sizeof(TableRecord));
                tableFound = true;
                break;
            }
        }

        file.close();

        // si no se encontro la tabla, no hay nada mas que hacer
        if (!tableFound) {
            return false;
        }

    // tambien hay que desactivar las columnas asociadas a esta tabla en SystemColumns
    std::filesystem::path colFilePath = buildPath("SystemColumns");
    std::fstream colFile(colFilePath, std::ios::binary | std::ios::in | std::ios::out);

    if (colFile.is_open()) {
        ColumnRecord colRec;

        // posicion actual de lectura, la vamos avanzando manualmente
        std::streampos readPos = 0;

         while (true) {
            // colocar el cursor de lectura en la posicion exacta antes de leer
             colFile.seekg(readPos);

            if (!colFile.read(reinterpret_cast<char*>(&colRec), sizeof(ColumnRecord))) {
                // ya no hay mas registros que leer, terminar el ciclo
                break;
             }

            // si la columna esta activa y pertenece a la tabla y base de datos eliminadas
            if (colRec.flag == 1 && std::string(colRec.tableName) == tableName && std::string(colRec.dbName) == dbName) {

                // marcar la columna como no disponible
                colRec.flag = 0;

                // colocar el cursor de escritura exactamente en la misma posicion que se leyo
                colFile.seekp(readPos);

                // volver a escribir el registro con el cambio
                colFile.write(reinterpret_cast<const char*>(&colRec), sizeof(ColumnRecord));
            }

            // avanzar manualmente a la posicion del siguiente registro
            readPos += sizeof(ColumnRecord);
        }

        colFile.close();
   }

   return true;
}

// Metodos relacionados con indices

// Registra un indice en el system catalog
bool SystemCatalog::registerIndex(const Index& index) {

    // verificar que el indice sea valido
    if (!index.isValid()) {
        return false; 
    }

    // crear struct y limpiar la memoria donde se guarda el struct en caso de que haya basura en ella
    IndexRecord record;
    memset(&record, 0, sizeof(IndexRecord));

    // establecer los datos del index en el struct
    record.flag = 1;
    record.type = index.type;

    // Copia segura de los strings para evitar desbordamiento en el archivo binario
    strncpy(record.indexName, index.name.c_str(), sizeof(record.indexName) - 1);
    strncpy(record.tableName, index.tableName.c_str(), sizeof(record.tableName) - 1);
    strncpy(record.columnName, index.columnName.c_str(), sizeof(record.columnName) - 1);

    // obtene rpath del archio que almacena la metadata de los indices
    std::filesystem::path pathFile = buildPath("SystemIndexes");

    // abrir el archivo para escritura
    std::ofstream file(pathFile, std::ios::binary | std::ios::app);

    // verificar que se abrio correctamnete
    if (!file.is_open()) {
        return false;
    }
    // escirbir los datos de record en el archivo 
    file.write(reinterpret_cast<const char*>(&record), sizeof(IndexRecord));
    return true;
}

// Retorna todos los indices activos del system catalog
Index* SystemCatalog::getAllIndexes(int& count) const {

    // obtener la ruta del archivo que contiene la metadata de los indices
    std::filesystem::path pathFile = buildPath("SystemIndexes");
    // abrir el archivo para lectura
    std::ifstream file(pathFile, std::ios::binary);

    // veriicar que se abrio correctamnete
    if (!file.is_open()) {
        count = 0;
        return nullptr;
    }

    // contar la cantidad de indices activos
    count = 0;
    IndexRecord record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(IndexRecord))) {
        if (record.flag == 1) count++;
    }
    if (count == 0) {
        return nullptr;
    }

    Index* result = new Index[count];

    // llenar el array

    // colocarnos al inicio del archivo
    file.clear();
    file.seekg(0, std::ios::beg);

    // leer 
    int i = 0;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(IndexRecord))) {
        if (record.flag == 1) {
            // si se encuntran indices activos guardarlos en el arreglo dinamico
            result[i] = this->recordToIndex(record);
            i++;
        }
    }
    return result;
}

// Busca el indice asociado a una columna especifica
Index SystemCatalog::getIndexForColumn(const std::string& tableName, const std::string& columnName) const {
    // obtener la ruta del archivo de indices
    std::filesystem::path pathFile = buildPath("SystemIndexes");
    // abrir archivo para lectura
    std::ifstream file(pathFile, std::ios::binary);
    // si la apertura fallo retornar un Index vacio (isValid() = false)
    if (!file.is_open()) return Index();

    IndexRecord record;
    // leer de IndexRecord bytes mientras haya elementos que leer en el archivo
    while (file.read(reinterpret_cast<char*>(&record), sizeof(IndexRecord))) {
        // si el registro esta activo y coincide con la tabla y columna brindadas
        if (record.flag == 1 &&
            std::string(record.tableName) == tableName &&
            std::string(record.columnName) == columnName)
            // convertir el struct binario a clase Index y retornar
            return this->recordToIndex(record);
    }
    // no se encontro ningun indice para esa columna
    return Index();
}

// Marca un indice como eliminado (soft delete)
bool SystemCatalog::unregisterIndex(const std::string& indexName) {
    // obtener la ruta del archivo de indices
    std::filesystem::path pathFile = buildPath("SystemIndexes");
    // abrir para leer y escribir sin truncar el archivo
    std::fstream file(pathFile, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        return false;
    }
    // leer de IndexRecord bytes mientras haya elementos que leer en el archivo
    IndexRecord record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(IndexRecord))) {
        // si el indice esta activo y el nombre coincide con el brindado
        if (record.flag == 1 && std::string(record.indexName) == indexName) {
            // retroceder el cursor al inicio de este registro
            file.seekp(-(std::streamoff)sizeof(IndexRecord), std::ios::cur);
            record.flag = 0;
            // volver a escribir record con el cambio en el flag (para que el cambio se guarde en disco)
            file.write(reinterpret_cast<const char*>(&record), sizeof(IndexRecord));
            return true;
        }
    }
    return false;
}

// Metodos privados

// construye el path para la creacion de los archivos principales del system catalog
std::filesystem::path SystemCatalog::buildPath(const std::string& fileName) const {

    // Convertimos la variable base 'path' en un objeto path
    std::filesystem::path rutaBase(this->path);

    // El operador / une rutas de forma segura (ej: "catalog" / "SystemDatabases.bin")
    return rutaBase / (fileName + ".bin");
}

// crear un archivo si no existe
void SystemCatalog::createFileIfNotExists(const std::filesystem::path& filePath) const {

    // verificar si el archivo existe
    if (std::filesystem::exists(filePath)) {
        return; 
    }
    // Si no existe lo creamos vacio
    std::ofstream create(filePath, std::ios::binary | std::ios::out);

}

// convierte un registro en una columna
Column SystemCatalog::recordToColumn(const ColumnRecord& rec) const {
    return Column(
        std::string(rec.columnName),
        std::string(rec.tableName),
        static_cast<ColumnType>(rec.type),
        rec.size,
        rec.offset,
        rec.position,
        rec.nullable == 1,
        static_cast<ColumnConstraint>(rec.constraint)
    );
}

// convierte un registro en un indice
Index SystemCatalog::recordToIndex(const IndexRecord& rec) const {
    return Index(
        std::string(rec.indexName),
        std::string(rec.tableName),
        std::string(rec.columnName),
        static_cast<IndexType>(rec.type)
    );
}