#pragma once
#include <string>
#include "Table.h"

// SortAlgorithms implementaciones de los algoritmos necesarios para el proyecto
// ordena filas representadas como arreglos de strings

class SortAlgorithms {
public:

    // Constructor vacio
    SortAlgorithms();

    // ordena las filas por una columna especifica
    void quickSort(std::string rows[][MAX_COLUMNS], int left, int right, int colIndex, ColumnType colType, bool ascending);

private:

    // divide el arreglo y retorna la posicion del pivot
    int partition(std::string rows[][MAX_COLUMNS], int left, int right, int colIndex, ColumnType colType, bool ascending);

    // intercambia dos filas completas
    void swapRows(std::string rows[][MAX_COLUMNS], int rowA, int rowB);

    // compara dos valores considerando el tipo de dato y direccion
    bool shouldSwap(const std::string& valueA, const std::string& valueB, ColumnType colType, bool ascending);
};