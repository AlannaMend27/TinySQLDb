#include "../QueryProcessor/include/QueryProcessor.h"
#include <iostream>
#include <filesystem>
#include <fstream>

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

void pruebaQueryProcessor()
{
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);

    QueryProcessor processor;
    QueryResult r;

    std::cout << "=== PRUEBA 1: CREATE DATABASE ===" << std::endl;

    processor.execute(r, "CREATE DATABASE Universidad", "");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE DATABASE Universidad", "");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE DATABASE Uni@versidad", "");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 2: keywords en minuscula ===" << std::endl;

    processor.execute(r, "create database Ventas", "");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 3: SET DATABASE ===" << std::endl;

    processor.execute(r, "SET DATABASE Universidad", "");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "SET DATABASE Fantasma", "");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 4: punto y coma y espacios ===" << std::endl;

    processor.execute(r, "  SET DATABASE Ventas ;  ", "");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 5: sentencia no reconocida ===" << std::endl;

    processor.execute(r, "BORRAR TODO", "");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 6: CREATE TABLE ===" << std::endl;

    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30))", "");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30), FechaNacimiento DATETIME)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Otra (ID NUMERO)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Otra (Nombre VARCHAR)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Producto (ID INTEGER NOT NULL PRIMARY KEY, Precio DOUBLE)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE SinParentesis ID INTEGER", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Algo (ID INTEGER)", "BasuraDB");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 7: verificar persistencia ===" << std::endl;

    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Producto (ID INTEGER)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Completa (A INTEGER, B DOUBLE, C VARCHAR(50), D DATETIME)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== Pruebas completadas ===" << std::endl;
}

void pruebaInsert()
{
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);

    QueryProcessor processor;
    QueryResult r;

    std::cout << "PRUEBA INSERT" << std::endl;
    std::cout << "=== SETUP: crear BD y tabla ===" << std::endl;

    processor.execute(r, "CREATE DATABASE Universidad", "");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30), Apellido VARCHAR(30), FechaNacimiento DATETIME)", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 1: INSERT basico ===" << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(1, \"Isaac\", \"Ramirez\", \"2000-01-01 01:02:00\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(2, \"Juan\", \"Lopez\", \"1999-05-15 00:00:00\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(3, \"Maria\", \"Herrera\", \"2001-03-20 00:00:00\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 2: errores de validacion ===" << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(4, \"Pedro\", \"Mora\", \"2000-01-01 00:00:00\")", "");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Fantasma VALUES(1, \"a\", \"b\", \"2000-01-01 00:00:00\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(\"abc\", \"Pedro\", \"Mora\", \"2000-01-01 00:00:00\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(4, \"Pedro\", \"Mora\", \"01/01/2000\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(4, \"Pedro\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Estudiante VALUES(4, \"NombreMuyLargoQueExcedeElLimiteDeTreintaCaracteres\", \"Mora\", \"2000-01-01 00:00:00\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== PRUEBA 3: verificar persistencia en disco ===" << std::endl;

    std::string tablePath = std::string(DATA_PATH) + "Universidad/Estudiante.bin";
    std::ifstream file(tablePath, std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        long fileSize = (long)file.tellg();
        std::cout << "Archivo existe: 1" << std::endl;
        std::cout << "Tamano del archivo: " << fileSize << " bytes" << std::endl;
        std::cout << "Correcto: " << (fileSize == 219 ? 1 : 0) << std::endl;
    }
    else
    {
        std::cout << "Archivo existe: 0" << std::endl;
    }

    std::cout << "=== PRUEBA 4: INSERT con DOUBLE ===" << std::endl;

    processor.execute(r, "CREATE TABLE Producto (ID INTEGER, Precio DOUBLE, Nombre VARCHAR(20))", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Producto VALUES(1, 9999.99, \"Laptop\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    processor.execute(r, "INSERT INTO Producto VALUES(1, \"precio\", \"Laptop\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== Pruebas INSERT completadas ===" << std::endl;
}

void pruebaSelectCompleto()
{
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);

    QueryProcessor processor;
    QueryResult r;

    // setup
    processor.execute(r, "CREATE DATABASE Universidad", "");
    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30), Apellido VARCHAR(30), Promedio DOUBLE)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(3, \"Pedro\", \"Herrera\", 8.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(1, \"Isaac\", \"Ramirez\", 9.0)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(2, \"Juan\", \"Lopez\", 7.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(4, \"Maria\", \"Ramirez\", 9.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(5, \"Ana\", \"Lopez\", 6.0)", "Universidad");

    // SELECT *
    std::cout << "=== SELECT * ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // SELECT columnas especificas
    std::cout << "=== SELECT Nombre, Promedio ===" << std::endl;
    processor.execute(r, "SELECT Nombre, Promedio FROM Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // WHERE con =
    std::cout << "=== WHERE ID = 3 ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante WHERE ID = 3", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // WHERE con >
    std::cout << "=== WHERE Promedio > 8.5 ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante WHERE Promedio > 8.5", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // WHERE con 
    std::cout << "=== WHERE ID < 3 ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante WHERE ID < 3", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // WHERE con LIKE
    std::cout << "=== WHERE Apellido LIKE *ez* ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante WHERE Apellido LIKE *ez*", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // WHERE con NOT
    std::cout << "=== WHERE Apellido NOT Ramirez ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante WHERE Apellido NOT Ramirez", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // ORDER BY ASC
    std::cout << "=== ORDER BY Promedio ASC ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante ORDER BY Promedio ASC", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // ORDER BY DESC
    std::cout << "=== ORDER BY Promedio DESC ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante ORDER BY Promedio DESC", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // WHERE + ORDER BY
    std::cout << "=== WHERE Promedio > 7.5 ORDER BY Nombre ASC ===" << std::endl;
    processor.execute(r, "SELECT Nombre, Promedio FROM Estudiante WHERE Promedio > 7.5 ORDER BY Nombre ASC", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // errores
    std::cout << "=== ERROR: sin base de datos ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante", "");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== ERROR: tabla inexistente ===" << std::endl;
    processor.execute(r, "SELECT * FROM Fantasma", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== ERROR: columna inexistente ===" << std::endl;
    processor.execute(r, "SELECT Fantasma FROM Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== Pruebas SELECT completadas ===" << std::endl;
}

void pruebaUpdate()
{
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);
    QueryProcessor processor;
    QueryResult r;

    // setup
    processor.execute(r, "CREATE DATABASE Universidad", "");
    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30), Apellido VARCHAR(30), Promedio DOUBLE)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(1, \"Isaac\", \"Ramirez\", 9.0)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(2, \"Juan\", \"Lopez\", 7.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(3, \"Pedro\", \"Herrera\", 8.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(4, \"Maria\", \"Ramirez\", 9.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(5, \"Ana\", \"Lopez\", 6.0)", "Universidad");

    // verificar datos iniciales
    std::cout << "=== DATOS INICIALES ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // UPDATE con WHERE =
    std::cout << "=== UPDATE con WHERE = ===" << std::endl;
    processor.execute(r, "UPDATE Estudiante SET Nombre = \"Felipe\" WHERE ID = 1", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 1 fila(s) actualizada(s)

    // verificar cambio
    processor.execute(r, "SELECT * FROM Estudiante WHERE ID = 1", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // debe mostrar: 1  Felipe  Ramirez  9.0

    // UPDATE con WHERE >
    std::cout << "=== UPDATE con WHERE > ===" << std::endl;
    processor.execute(r, "UPDATE Estudiante SET Promedio = 10.0 WHERE Promedio > 9.0", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 1 fila(s) actualizada(s) (solo Maria)

    // verificar cambio
    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // Maria debe tener Promedio = 10.0

    // UPDATE sin WHERE — actualiza todas las filas
    std::cout << "=== UPDATE sin WHERE ===" << std::endl;
    processor.execute(r, "UPDATE Estudiante SET Apellido = \"Gonzalez\"", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 5 fila(s) actualizada(s)

    // verificar que todos cambiaron
    processor.execute(r, "SELECT Nombre, Apellido FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // todos deben tener Apellido = Gonzalez

    // UPDATE con WHERE LIKE
    std::cout << "=== UPDATE con WHERE LIKE ===" << std::endl;
    processor.execute(r, "UPDATE Estudiante SET Promedio = 5.0 WHERE Nombre LIKE *an*", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 2 fila(s) (Juan y Ana)

    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // Juan y Ana deben tener Promedio = 5.0

    // UPDATE con WHERE NOT
    std::cout << "=== UPDATE con WHERE NOT ===" << std::endl;
    processor.execute(r, "UPDATE Estudiante SET Promedio = 0.0 WHERE Nombre NOT Juan", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 4 fila(s) (todos menos Felipe)

    processor.execute(r, "SELECT Nombre, Promedio FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // solo Felipe mantiene su Promedio, el resto tiene 0.0

    std::cout << "=== PRUEBAS DE ERROR ===" << std::endl;

    // sin base de datos activa
    processor.execute(r, "UPDATE Estudiante SET Nombre = \"X\" WHERE ID = 1", "");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // tabla que no existe
    processor.execute(r, "UPDATE Fantasma SET Nombre = \"X\"", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // columna del SET que no existe
    processor.execute(r, "UPDATE Estudiante SET ColumnaFantasma = \"X\"", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // tipo incorrecto en el SET
    processor.execute(r, "UPDATE Estudiante SET ID = \"abc\"", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // columna del WHERE que no existe
    processor.execute(r, "UPDATE Estudiante SET Nombre = \"X\" WHERE Fantasma = 1", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // sintaxis incorrecta sin SET
    processor.execute(r, "UPDATE Estudiante Nombre = \"X\"", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    std::cout << "=== Pruebas UPDATE completadas ===" << std::endl;
}

void pruebaDelete()
{
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);
    QueryProcessor processor;
    QueryResult r;

    // setup
    processor.execute(r, "CREATE DATABASE Universidad", "");
    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30), Apellido VARCHAR(30), Promedio DOUBLE)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(1, \"Isaac\", \"Ramirez\", 9.0)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(2, \"Juan\", \"Lopez\", 7.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(3, \"Pedro\", \"Herrera\", 8.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(4, \"Maria\", \"Ramirez\", 9.5)", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(5, \"Ana\", \"Lopez\", 6.0)", "Universidad");

    // datos iniciales
    std::cout << "=== DATOS INICIALES ===" << std::endl;
    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }

    // DELETE con WHERE =
    std::cout << "=== DELETE WHERE ID = 1 ===" << std::endl;
    processor.execute(r, "DELETE FROM Estudiante WHERE ID = 1", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 1 fila(s) eliminada(s)

    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // Isaac ya no debe aparecer

    // DELETE con WHERE >
    std::cout << "=== DELETE WHERE Promedio > 9.0 ===" << std::endl;
    processor.execute(r, "DELETE FROM Estudiante WHERE Promedio > 9.0", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 1 fila(s) (Maria)

    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // deben quedar Juan, Pedro y Ana

    // DELETE con WHERE LIKE
    std::cout << "=== DELETE WHERE Apellido LIKE *ez* ===" << std::endl;
    processor.execute(r, "DELETE FROM Estudiante WHERE Apellido LIKE *ez*", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 1 fila(s) (Juan Lopez)

    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    for (int i = 0; i < r.rowCount; i++)
    {
        for (int j = 0; j < r.columnCount; j++) std::cout << r.rows[i][j] << "\t";
        std::cout << std::endl;
    }
    // deben quedar Pedro y Ana

    // DELETE sin WHERE — elimina todo
    std::cout << "=== DELETE sin WHERE ===" << std::endl;
    processor.execute(r, "DELETE FROM Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 2 fila(s) eliminada(s)

    processor.execute(r, "SELECT * FROM Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1 | 0 fila(s) encontrada(s)
    // tabla vacia

    std::cout << "=== PRUEBAS DE ERROR ===" << std::endl;

    // sin base de datos activa
    processor.execute(r, "DELETE FROM Estudiante WHERE ID = 1", "");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // tabla que no existe
    processor.execute(r, "DELETE FROM Fantasma", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // columna del WHERE que no existe
    processor.execute(r, "DELETE FROM Estudiante WHERE ColumnaFalsa = 1", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // sintaxis incorrecta
    processor.execute(r, "DELETE Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    std::cout << "=== Pruebas DELETE completadas ===" << std::endl;
}

void pruebaDrop()
{
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);
    QueryProcessor processor;
    QueryResult r;

    // setup
    processor.execute(r, "CREATE DATABASE Universidad", "");
    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30))", "Universidad");
    processor.execute(r, "CREATE TABLE Cursos (ID INTEGER, Nombre VARCHAR(50))", "Universidad");

    // DROP tabla vacia — debe funcionar
    std::cout << "=== DROP tabla vacia ===" << std::endl;
    processor.execute(r, "DROP TABLE Cursos", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1

    // verificar que ya no existe
    processor.execute(r, "DROP TABLE Cursos", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0 — ya no existe

    // eliminar los datos y luego dropear
    std::cout << "=== DROP despues de DELETE ===" << std::endl;
    processor.execute(r, "DELETE FROM Estudiante", "Universidad");
    processor.execute(r, "DROP TABLE Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 1

    // verificar que el archivo ya no existe
    std::filesystem::path tablePath = std::filesystem::path(DATA_PATH) / "Universidad" / "Estudiante.bin";
    std::cout << "Archivo eliminado: " << (!std::filesystem::exists(tablePath) ? 1 : 0) << std::endl; // 1

    std::cout << "=== PRUEBAS DE ERROR ===" << std::endl;

    // sin base de datos activa
    processor.execute(r, "DROP TABLE Estudiante", "");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // tabla que no existe
    processor.execute(r, "DROP TABLE Fantasma", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    // sintaxis incorrecta
    processor.execute(r, "DROP Estudiante", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl; // 0

    std::cout << "=== Pruebas DROP completadas ===" << std::endl;
}

void pruebaIndex()
{
    std::filesystem::remove_all(CATALOG_PATH);
    std::filesystem::remove_all(DATA_PATH);

    QueryProcessor processor;
    QueryResult r;

    // setup
    processor.execute(r, "CREATE DATABASE Universidad", "");
    processor.execute(r, "CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(30), Apellido VARCHAR(30))", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(1, \"Isaac\", \"Ramirez\")", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(2, \"Juan\", \"Lopez\")", "Universidad");
    processor.execute(r, "INSERT INTO Estudiante VALUES(3, \"Pedro\", \"Herrera\")", "Universidad");

    // prueba CREATE INDEX exitoso
    std::cout << "=== CREATE INDEX BST ===" << std::endl;
    processor.execute(r, "CREATE INDEX Estudiante_ID ON Estudiante(ID) OF TYPE BST", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    // prueba duplicado de indice en misma columna
    std::cout << "=== ERROR: indice duplicado en misma columna ===" << std::endl;
    processor.execute(r, "CREATE INDEX Otro_ID ON Estudiante(ID) OF TYPE BST", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    // prueba columna inexistente
    std::cout << "=== ERROR: columna inexistente ===" << std::endl;
    processor.execute(r, "CREATE INDEX Idx ON Estudiante(Fantasma) OF TYPE BST", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    // prueba tabla inexistente
    std::cout << "=== ERROR: tabla inexistente ===" << std::endl;
    processor.execute(r, "CREATE INDEX Idx ON Fantasma(ID) OF TYPE BST", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    // prueba duplicados en columna
    std::cout << "=== ERROR: duplicados en columna ===" << std::endl;
    processor.execute(r, "CREATE TABLE Productos (ID INTEGER, Nombre VARCHAR(30))", "Universidad");
    processor.execute(r, "INSERT INTO Productos VALUES(1, \"Laptop\")", "Universidad");
    processor.execute(r, "INSERT INTO Productos VALUES(1, \"Celular\")", "Universidad");
    processor.execute(r, "CREATE INDEX Productos_ID ON Productos(ID) OF TYPE BST", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    // prueba INSERT con duplicado despues de crear indice
    std::cout << "=== ERROR: INSERT duplicado con indice activo ===" << std::endl;
    processor.execute(r, "INSERT INTO Estudiante VALUES(1, \"Otro\", \"Apellido\")", "Universidad");
    std::cout << r.success << " | " << r.message << std::endl;

    std::cout << "=== Pruebas INDEX completadas ===" << std::endl;
}

int main()
{
    //pruebaCatalog();
    //pruebaQueryProcessor();
    //pruebaInsert();
    //pruebaSelectCompleto();
    //pruebaUpdate();
    //pruebaDelete();
    //pruebaDrop();
    pruebaIndex();
    std::cin.get();
    return 0;
}