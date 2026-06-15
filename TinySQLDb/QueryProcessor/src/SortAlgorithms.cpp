#define _CRT_SECURE_NO_WARNINGS
#include "SortAlgorithms.h"

// Constructor vacio
SortAlgorithms::SortAlgorithms()
{
}

// intercambia dos filas completas del arreglo
void SortAlgorithms::swapRows(std::string rows[][MAX_COLUMNS], int rowA, int rowB)
{
    // intercambiar columna por columna entre las dos filas
    for (int col = 0; col < (int)MAX_COLUMNS; col++)
    {
        std::string temp = rows[rowA][col];
        rows[rowA][col] = rows[rowB][col];
        rows[rowB][col] = temp;
    }
}

// compara dos valores considerando el tipo de dato y la direccion
// retorna true si valueA debe ir antes que valueB
bool SortAlgorithms::shouldSwap(const std::string& valueA, const std::string& valueB, ColumnType colType, bool ascending)
{
    if (colType == TYPE_INTEGER || colType == TYPE_DOUBLE)
    {
        // comparar como numeros
        if (ascending)
        {
            return std::stod(valueA) < std::stod(valueB);
        }
        return std::stod(valueA) > std::stod(valueB);
    }

    // comparar como strings
    if (ascending)
    {
        return valueA < valueB;
    }
    return valueA > valueB;
}

// divide el arreglo en dos partes y retorna la posicion del pivote
int SortAlgorithms::partition(std::string rows[][MAX_COLUMNS], int low, int high, int colIndex, ColumnType colType, bool ascending)
{
    // el pivot es la ultima fila
    std::string pivot = rows[high][colIndex];

    //Indice de la ultima fila menor al pivote
    int lastSmaller = low - 1;

    // recorrer todas las filas excepto el pivote
    for (int current = low; current <= high - 1; current++)
    {
        //Si la fila actual debe ir antes que el pivote
        if (this->shouldSwap(rows[current][colIndex], pivot, colType, ascending))
        {
            lastSmaller++;
            // mover la fila actual al lado izquierdo
            this->swapRows(rows, lastSmaller, current);
        }
    }

    //Colocamos el pivote correctamente
    this->swapRows(rows, lastSmaller + 1, high);
    return lastSmaller + 1;
}

// ordena las filas usando Quicksort
void SortAlgorithms::quickSort(std::string rows[][MAX_COLUMNS], int low, int high, int colIndex, ColumnType colType, bool ascending)
{
    if (low < high)
    {
        // pi es la posicion final del pivot
        int pi = this->partition(rows, low, high, colIndex, colType, ascending);

        // ordenar la mitad izquierda
        this->quickSort(rows, low, pi - 1, colIndex, colType, ascending);

        // ordenar la mitad derecha
        this->quickSort(rows, pi + 1, high, colIndex, colType, ascending);
    }
}