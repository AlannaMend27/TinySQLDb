#include "../QueryProcessor/include/QueryProcessor.h"
#include <iostream>
#include <filesystem>

// Prueba del SystemCatalog directamente 
void pruebaCatalog()
{
    // limpiar archivos de pruebas anteriores
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);

    SystemCatalog catalog(CATALOG_PATH);

    std::cout << "=== PRUEBA 1: Bases de datos ===" << std::endl;

    // registrar bases de datos
    Database db1("Universidad");
    Database db2("Ventas");
    Database dbInvalida(""); // nombre vacio — debe fallar

    std::cout << catalog.registerDatabase(db1) << std::endl; // 1
    std::cout << catalog.registerDatabase(db2) << std::endl; // 1
    std::cout << catalog.registerDatabase(db1) << std::endl; // 0 — duplicado
    std::cout << catalog.registerDatabase(dbInvalida) << std::endl; // 0 — invalida

    // verificar existencia
    std::cout << catalog.databaseExists("Universidad") << std::endl; // 1
    std::cout << catalog.databaseExists("Ventas") << std::endl; // 1
    std::cout << catalog.databaseExists("Fantasma") << std::endl; // 0

    std::cout << "=== PRUEBA 2: Tablas ===" << std::endl;

    // construir columnas manualmente para la tabla Estudiante
    // flag(1) + ID(4) → Nombre empieza en offset 5
    // flag(1) + ID(4) + Nombre(30) → Apellido empieza en offset 35
    Column col1("ID", "Estudiante", TYPE_INTEGER, 4, 1, 0, false, CONSTRAINT_PRIMARY_KEY);
    Column col2("Nombre", "Estudiante", TYPE_VARCHAR, 30, 5, 1, false, CONSTRAINT_NONE);
    Column col3("Apellido", "Estudiante", TYPE_VARCHAR, 30, 35, 2, true, CONSTRAINT_NONE);

    Column cols[3] = { col1, col2, col3 };
    Table tabla("Estudiante", "Universidad", cols, 3);

    std::cout << catalog.registerTable(tabla) << std::endl; // 1
    std::cout << catalog.registerTable(tabla) << std::endl; // 0 — duplicada
    std::cout << catalog.tableExists("Universidad", "Estudiante") << std::endl; // 1
    std::cout << catalog.tableExists("Universidad", "Fantasma") << std::endl; // 0

    std::cout << "=== PRUEBA 3: getTable ===" << std::endl;

    Table t = catalog.getTable("Universidad", "Estudiante");
    std::cout << t.isValid() << std::endl; // 1
    std::cout << t.name << std::endl; // Estudiante
    std::cout << t.columnCount << std::endl; // 3
    std::cout << t.rowSize << std::endl; // 1 + 4 + 30 + 30 = 65

    // verificar que las columnas quedaron en orden correcto
    for (int i = 0; i < t.columnCount; i++) {
        std::cout << t.columns[i].position << " "
            << t.columns[i].name << " "
            << t.columns[i].typeToString() << std::endl;
        // debe imprimir:
        // 0 ID INTEGER
        // 1 Nombre VARCHAR(30)
        // 2 Apellido VARCHAR(30)
    }

    std::cout << "=== PRUEBA 4: Indices ===" << std::endl;

    Index idx("Estudiante_ID", "Estudiante", "ID", INDEX_BTREE);
    std::cout << catalog.registerIndex(idx) << std::endl; // 1

    Index encontrado = catalog.getIndexForColumn("Estudiante", "ID");
    std::cout << encontrado.isValid() << std::endl; // 1
    std::cout << encontrado.name << std::endl; // Estudiante_ID
    std::cout << encontrado.typeToString() << std::endl; // BTREE

    Index noExiste = catalog.getIndexForColumn("Estudiante", "Nombre");
    std::cout << noExiste.isValid() << std::endl; // 0 — no hay indice en Nombre

    std::cout << "=== PRUEBA 5: unregisterTable ===" << std::endl;

    std::cout << catalog.unregisterTable("Universidad", "Estudiante") << std::endl; // 1
    std::cout << catalog.tableExists("Universidad", "Estudiante") << std::endl; // 0

    std::cout << "=== PRUEBA 6: unregisterIndex ===" << std::endl;

    std::cout << catalog.unregisterIndex("Estudiante_ID") << std::endl; // 1
    Index eliminado = catalog.getIndexForColumn("Estudiante", "ID");
    std::cout << eliminado.isValid() << std::endl; // 0

    std::cout << "=== Todas las pruebas completadas ===" << std::endl;
}

// Prueba del QueryProcessor 
void pruebaQueryProcessor()
{
    // limpiar archivos de pruebas anteriores
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);

    QueryProcessor processor;

    std::cout << "=== PRUEBA 1: CREATE DATABASE ===" << std::endl;

    // crear una base de datos nueva, debe funcionar
    QueryResult r1 = processor.execute("CREATE DATABASE Universidad", "");
    std::cout << r1.success << " | " << r1.message << std::endl; // 1

    // crear la misma otra vez, debe fallar por duplicada
    QueryResult r2 = processor.execute("CREATE DATABASE Universidad", "");
    std::cout << r2.success << " | " << r2.message << std::endl; // 0

    // nombre invalido con caracteres raros, debe fallar
    QueryResult r3 = processor.execute("CREATE DATABASE Uni@versidad", "");
    std::cout << r3.success << " | " << r3.message << std::endl; // 0

    std::cout << "=== PRUEBA 2: keywords en minuscula ===" << std::endl;

    // los keywords deben funcionar sin importar el caso
    QueryResult r4 = processor.execute("create database Ventas", "");
    std::cout << r4.success << " | " << r4.message << std::endl; // 1

    std::cout << "=== PRUEBA 3: SET DATABASE ===" << std::endl;

    // establecer una base de datos que existe, debe funcionar
    QueryResult r5 = processor.execute("SET DATABASE Universidad", "");
    std::cout << r5.success << " | " << r5.message << std::endl; // 1

    // establecer una que no existe, debe fallar
    QueryResult r6 = processor.execute("SET DATABASE Fantasma", "");
    std::cout << r6.success << " | " << r6.message << std::endl; // 0

    std::cout << "=== PRUEBA 4: punto y coma y espacios ===" << std::endl;

    // con punto y coma y espacios sobrantes, debe limpiarlo y funcionar
    QueryResult r7 = processor.execute("  SET DATABASE Ventas ;  ", "");
    std::cout << r7.success << " | " << r7.message << std::endl; // 1

    std::cout << "=== PRUEBA 5: sentencia no reconocida ===" << std::endl;

    // un comando que no existe, debe fallar
    QueryResult r8 = processor.execute("BORRAR TODO", "");
    std::cout << r8.success << " | " << r8.message << std::endl; // 0

    std::cout << "=== PRUEBA 6: CREATE TABLE ===" << std::endl;

    // sin contexto de base de datos, debe fallar
    QueryResult t1 = processor.execute("CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30))", "");
    std::cout << t1.success << " | " << t1.message << std::endl; // 0

    // con contexto valido, debe funcionar
    QueryResult t2 = processor.execute("CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30), FechaNacimiento DATETIME)", "Universidad");
    std::cout << t2.success << " | " << t2.message << std::endl; // 1

    // duplicada, debe fallar
    QueryResult t3 = processor.execute("CREATE TABLE Estudiante (ID INTEGER)", "Universidad");
    std::cout << t3.success << " | " << t3.message << std::endl; // 0

    // tipo de dato desconocido, debe fallar
    QueryResult t4 = processor.execute("CREATE TABLE Otra (ID NUMERO)", "Universidad");
    std::cout << t4.success << " | " << t4.message << std::endl; // 0

    // VARCHAR sin tamanio especificado, debe fallar
    QueryResult t5 = processor.execute("CREATE TABLE Otra (Nombre VARCHAR)", "Universidad");
    std::cout << t5.success << " | " << t5.message << std::endl; // 0

    // con PRIMARY KEY y NOT NULL, debe funcionar
    QueryResult t6 = processor.execute("CREATE TABLE Producto (ID INTEGER NOT NULL PRIMARY KEY, Precio DOUBLE)", "Universidad");
    std::cout << t6.success << " | " << t6.message << std::endl; // 1

    // sin parentesis, debe fallar
    QueryResult t7 = processor.execute("CREATE TABLE SinParentesis ID INTEGER", "Universidad");
    std::cout << t7.success << " | " << t7.message << std::endl; // 0

    // base de datos que no existe en el contexto, debe fallar
    QueryResult t8 = processor.execute("CREATE TABLE Algo (ID INTEGER)", "BasuraDB");
    std::cout << t8.success << " | " << t8.message << std::endl; // 0

    std::cout << "=== PRUEBA 7: verificar que las tablas quedaron en disco ===" << std::endl;

    // verificar que Estudiante existe en Universidad
    QueryResult v1 = processor.execute("CREATE TABLE Estudiante (ID INTEGER)", "Universidad");
    std::cout << v1.success << " | " << v1.message << std::endl; // 0 — ya existe, confirma que se persisto

    // verificar que Producto existe en Universidad
    QueryResult v2 = processor.execute("CREATE TABLE Producto (ID INTEGER)", "Universidad");
    std::cout << v2.success << " | " << v2.message << std::endl; // 0 — ya existe, confirma que se persisto

    // crear una tabla con todos los tipos soportados
    QueryResult v3 = processor.execute("CREATE TABLE Completa (A INTEGER, B DOUBLE, C VARCHAR(50), D DATETIME)", "Universidad");
    std::cout << v3.success << " | " << v3.message << std::endl; // 1

    std::cout << "=== Pruebas completadas ===" << std::endl;
}

int main()
{
    pruebaCatalog();
    pruebaQueryProcessor();
    std::cin.get();
    return 0;
}