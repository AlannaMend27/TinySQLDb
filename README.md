# TinySQLDb

Motor de base de datos relacional sencillo desarrollado en **C++** para el servidor y **JavaScript / ReactJS** para el cliente web.

**Autores:**
- Dylan Bonilla Barquero 
- Alanna Mendoza Fonseca 

---

## Descripcion

TinySQLDb es un sistema administrador de bases de datos que soporta un ciertos comandos del lenguaje SQL. Permite crear bases de datos y tablas, insertar, consultar, actualizar y eliminar registros, y crear indices de tipo BST o B-Tree sobre columnas para acelerar las busquedas. Los datos se almacenan en archivos binarios encriptados en el disco, organizados por carpetas.

El sistema sigue la arquitectura cliente-servidor de tres capas:

1. **Cliente web (React)** : editor de texto donde el usuario escribe sentencias SQL y visualiza los resultados en una tabla.
2. **Web API (C++ / httplib)** : recibe las peticiones HTTP en formato JSON, las traduce a sentencias SQL y devuelve la respuesta tambien en JSON.
3. **Query Processor (C++)** : parsea, valida y ejecuta cada sentencia SQL apoyandose en el System Catalog y el Stored Data Manager.
4. **Stored Data Manager (C++)** : lee y escribe los archivos binarios de cada tabla, encriptando y desencriptando la informacion en disco.

---

## Sentencias SQL soportadas

| Sentencia | Descripcion |
|---|---|
| `CREATE DATABASE <nombre>` | Crea una base de datos nueva (carpeta en disco) |
| `SET DATABASE <nombre>` | Establece el contexto de base de datos activa |
| `CREATE TABLE <nombre> (columnas)` | Crea una tabla con sus columnas y tipos |
| `DROP TABLE <nombre>` | Elimina una tabla vacia |
| `CREATE INDEX <nombre> ON <tabla>(<columna>) OF TYPE BST\|BTREE` | Crea un indice sobre una columna |
| `INSERT INTO <tabla> VALUES(...)` | Inserta una fila nueva |
| `SELECT * \| <columnas> FROM <tabla> [WHERE ...] [ORDER BY ...]` | Consulta filas, con filtro y ordenamiento opcional |
| `UPDATE <tabla> SET <columna> = <valor> [WHERE ...]` | Actualiza filas que cumplan la condicion |
| `DELETE FROM <tabla> [WHERE ...]` | Elimina filas que cumplan la condicion |

**Tipos de dato:** `INTEGER`, `DOUBLE`, `VARCHAR(n)`, `DATETIME`

**Operadores de comparacion en WHERE:** `=`, `>`, `<`, `LIKE`, `NOT`

---

## Algoritmos y estructuras implementadas

Las estructuras de datos fueron implementadas, sin usar contenedores de busqueda de la STL:

| Estructura / Algoritmo | Uso |
|---|---|
| **Arbol Binario de Busqueda (BST)** | Indice en memoria de tipo BST sobre una columna |
| **Arbol B (orden 3)** | Indice en memoria de tipo BTREE sobre una columna |
| **Quicksort** | Ordenamiento de resultados en `ORDER BY` |
| **Encriptacion XOR** | Encriptado y desencriptado de los archivos binarios en disco |

El proposito de los indices es evitar la busqueda secuencial registro por registro en disco. El arbol en memoria mapea el valor de la columna indexada a la posicion exacta del registro en el archivo, permitiendo saltar directamente a esa posicion con `seek`.

---

## Estructura del proyecto

```
TinySQLDb/
├── QueryProcessor/
│   ├── include/
│   │   ├── QueryProcessor.h        # Identifica el comando y coordina su ejecucion
│   │   ├── Commands.h              # Clase base con metodos compartidos (validaciones, WHERE, deserializacion)
│   │   ├── DatabaseCommands.h      # CREATE DATABASE / SET DATABASE
│   │   ├── TableCommands.h         # CREATE TABLE
│   │   ├── DropCommands.h          # DROP TABLE
│   │   ├── InsertCommands.h        # INSERT INTO
│   │   ├── SelectCommands.h        # SELECT (*) FROM (TABLA)
│   │   ├── UpdateCommands.h        # UPDATE
│   │   ├── DeleteCommands.h        # DELETE
│   │   ├── IndexCommands.h         # CREATE INDEX (nombre) ON (tabla)(<columna>) OF TYPE (tipo)
│   │   ├── IndexManager.h          # Administra los indices activos en memoria
│   │   ├── BST.h                   # Arbol binario de busqueda (BST/BTree)
│   │   ├── BTree.h                 # Arbol B (orden 3)
│   │   ├── SortAlgorithms.h        # Quicksort para ORDER BY
│   │   └── QueryResult.h           # Estructura del resultado de una consulta
│   └── src/
│       ├── QueryProcessor.cpp
│       ├── DatabaseCommands.cpp
│       ├── TableCommands.cpp
│       ├── DropCommands.cpp
│       ├── InsertCommands.cpp
│       ├── SelectCommands.cpp
│       ├── UpdateCommands.cpp
│       ├── DeleteCommands.cpp
│       ├── IndexCommands.cpp
│       ├── IndexManager.cpp
│       ├── BST.cpp
│       ├── BTree.cpp
│       └── SortAlgorithms.cpp
│
├── StoredDataManager/
│   ├── include/
│   │   ├── StoredDataManager.h     # Lectura y escritura de archivos binarios de las tablas
│   │   ├── SystemCatalog.h         # Metadata: bases de datos, tablas, columnas e indices
│   │   ├── Table.h
│   │   ├── Column.h
│   │   ├── Database.h
│   │   ├── Index.h
│   │   └── Records.h               # Constantes y structs de registros binarios, contiene el formato
│   └── src/
│       ├── StoredDataManager.cpp
│       ├── SystemCatalog.cpp
│       ├── Table.cpp
│       ├── Column.cpp
│       ├── Database.cpp
│       └── Index.cpp
│
├── WebAPI/
│   ├── include/
│   │   ├── httplib.h               # Biblioteca HTTP usada para exponer el API
│   ├── WebAPI.cpp                  # Servidor HTTP
│
└── tinysql-client/
    ├── src/
    │   ├── App.jsx                 # Editor SQL, historial y tabla de resultados
    │   └── App.css
    └── package.json
```

---

## Requisitos

- **Sistema operativo:** Windows 10 / 11
- **IDE:** Visual Studio 2022-2026  (QueryProcessor, StoredDataManager, WebAPI)
- **Node.js** y **npm** (para el cliente React "tinysql-client")
- **Arquitectura:** x64

---

## Como compilar y ejecutar

### 1. Clonar el repositorio

```bash
git clone https://github.com/AlannaMend27/TinySQLDb.git
```

### 2. Servidor (WebAPI)

1. Abrir la solucion en **Visual Studio**.
2. Establecer `WebAPI` como proyecto de inicio.
3. Compilar con `Ctrl + Shift + B` en configuracion **Debug** o **Release**, arquitectura **x64**.
4. Ejecutar con `F5` o `Ctrl + F5`. El servidor queda escuchando en:

```
http://localhost:8080
```

### 3. Cliente (React)

```bash
cd tinysql-client
npm install
npm run dev
```

El cliente queda disponible en `http://localhost:5173` y se conecta automaticamente al servidor en el puerto 8080.

---

## Ejemplo de uso

Script de ejemplo que se puede pegar directamente en el editor del cliente web (cada sentencia separada por `;`):

```sql
CREATE DATABASE Universidad;
SET DATABASE Universidad;

CREATE TABLE Estudiante (
    ID INTEGER,
    Nombre VARCHAR(30),
    PrimerApellido VARCHAR(30),
    Promedio DOUBLE
);

INSERT INTO Estudiante VALUES(1, "Isaac", "Ramirez", 9.0);
INSERT INTO Estudiante VALUES(2, "Juan", "Lopez", 7.5);
INSERT INTO Estudiante VALUES(3, "Pedro", "Herrera", 8.5);

CREATE INDEX Estudiante_ID ON Estudiante(ID) OF TYPE BTREE;

SELECT * FROM Estudiante WHERE ID = 2;
SELECT Nombre, Promedio FROM Estudiante ORDER BY Promedio DESC;

UPDATE Estudiante SET Promedio = 10.0 WHERE ID = 1;
DELETE FROM Estudiante WHERE Promedio < 8.0;

SELECT * FROM Estudiante;
```

Las sentencias pueden ser ejecutadas una a una, o bien varias al mismo tiempo. Las sentencias son separadas por ";".

---

## Arquitectura del sistema

El flujo de una sentencia SQL sigue este recorrido:

1. El **cliente React** separa el script por `;` y envia cada sentencia por separado mediante una peticion `POST` al **Web API**.
2. El **Web API** extrae la sentencia y la base de datos activa del JSON recibido y los pasa al **Query Processor**, sin manipular nada mas del protocolo de comunicacion.
3. El **Query Processor** identifica el tipo de comando, valida la sintaxis y la semantica contra el **System Catalog** (existencia de base de datos, tabla, columnas y tipos), y decide si usar un **indice** (BST o B-Tree) o una **busqueda secuencial** para resolver el `WHERE`.
4. El **Stored Data Manager** es quien finalmente lee o escribe los archivos binarios en disco, encriptando cada fila antes de guardarla y desencriptandola al leerla.
5. El resultado regresa por la misma cadena hasta el cliente, que lo renderiza en una tabla junto con el tiempo de ejecucion.

Cada capa desconoce los detalles internos de la siguiente: el Web API es el unico punto del sistema que manipula JSON, y el Stored Data Manager es el unico que sabe que los archivos estan encriptados.