#include "StoredDataManager.h"
#include <filesystem>



// Constructor vacio, usa la ruta por defecto definida en Records.h
StoredDataManager::StoredDataManager()
{
    this->catalog = SystemCatalog(CATALOG_PATH);
}


// Crea una base de datos nueva en el system catalog y su carpeta en disco
bool StoredDataManager::createDatabase(const std::string& name) {

    // crear el objeto Database con el nombre recibido
    Database db(name);

    // intentar registrar la base de datos en el system catalog
    bool registered = this->catalog.registerDatabase(db);

    // si no se pudo registrar, retornar false
    if (!registered) {
        return false;
    }

    // crear la carpeta de la base de datos en disco
    std::string dbFolderPath = DATA_PATH + name + "/";
    std::filesystem::create_directories(dbFolderPath);

    return true;
}

// Verifica si una base de datos existe en el system catalog
bool StoredDataManager::databaseExists(const std::string& name) {

    // delegar la verificacion al system catalog
    return this->catalog.databaseExists(name);
}