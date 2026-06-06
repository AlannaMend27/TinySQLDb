#include "../StoredDataManager/include/SystemCatalog.h"
#include <iostream>

int main() {

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
    Column col1("ID", "Estudiante", TYPE_INTEGER, 4, 1, 0);
    Column col2("Nombre", "Estudiante", TYPE_VARCHAR, 30, 5, 1);
    Column col3("Apellido", "Estudiante", TYPE_VARCHAR, 30, 35, 2);

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

    return 0;
}