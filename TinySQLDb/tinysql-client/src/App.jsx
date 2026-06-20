import { useState } from 'react'
import axios from 'axios'
import './App.css'

// URL del servidor backend (WebApi)
const API_URL = 'http://localhost:8080/query'

function App() {
  /* 
    ESTADO DE LA APLICACIÓN:
    - statement: La consulta SQL que el usuario escribe
    - database: La base de datos activa (contexto)
    - result: El resultado de la última consulta ejecutada
    - loading: Indica si estamos esperando respuesta del servidor
    - history: Lista de todas las consultas ejecutadas
  */

  const [statement, setStatement] = useState('')
  const [database, setDatabase] = useState('')
  const [result, setResult] = useState(null)
  const [loading, setLoading] = useState(false)
  const [history, setHistory] = useState([])

  // Ejecuta la sentencia SQL recibida en el editor de texto
  const execute = async () => {

    // Si el editor esta vacio, no hacer nada
    if (!statement.trim()) {
      return
    }

    // Separar el texto del editor en sentencias individuales
    // Se separan cada punto y coma, guardando cada una en un array de la forma: ["CREATE DATABASE X", "SELECT * FROM Y"]
    const allStatements = statement.split(';')

    // Limpiar las sentencias
    const cleanStatements = []
    for (let i = 0; i < allStatements.length; i++) {

      // Quitarle espacios blanco al inicio o al final de cada sentencia
      const cleanStatement = allStatements[i].trim()

      // Verificar que no son vacias antes de colocarlas en el array
      if (cleanStatement.length > 0) {

        cleanStatements.push(cleanStatement)

      }
    }

    // Si despues de limpiar no quedo ninguna sentencia valida, no hacer nada
    if (cleanStatements.length === 0) {
      return
    }

    // Activar el estado de carga
    setLoading(true)

    // Variables para guardar resultados durante la ejecucion
    let currentDatabase = database
    let lastSelectResult = null
    let lastStatementResult = null
    const newHistoryEntries = []

    // Ejecutar cada sentencia una por una en orden
    for (let i = 0; i < cleanStatements.length; i++) {

      // Sentencia actual que vamos a ejecutar
      const currentStatement = cleanStatements[i]

      try {
        // Enviar la sentencia al servidor
        const response = await axios.post(API_URL, {
          statement: currentStatement,
          database: currentDatabase
        })

        // Guardar esta sentencia en el historial
        const historyEntry = {
          id: Date.now() + i, // id unico usando el tiempo + indice
          statement: currentStatement,
          result: response.data,
          timestamp: new Date().toLocaleTimeString()
        }
        newHistoryEntries.push(historyEntry)

        // Verificar si esta sentencia era un SET DATABASE exitoso
        const upperStatement = currentStatement.toUpperCase()

        // Si lo era, actualizar la base de datos activa para las siguientes sentencias
        if (upperStatement.startsWith('SET DATABASE') && response.data.success) {
          const words = currentStatement.trim().split(' ')

          if (words.length >= 3) {
            currentDatabase = words[2]
          }

        }

        // Verificar si esta sentencia era un SELECT exitoso
        // Si lo era, guardar su resultado para mostrarlo al final
        if (upperStatement.startsWith('SELECT') && response.data.success) {
          lastSelectResult = response.data
        }

        // Guardar el resultado de la ultima sentencia ejecutada
        lastStatementResult = response.data

      } catch (error) {

        // Si hubo un error de conexion con el servidor
        const errorData = {
          success: false,
          message: 'Error de conexion con el servidor',
          columns: [],
          rows: [],
          timeMs: 0
        }

        // Guardar el error en el historial 
        newHistoryEntries.push({
          id: Date.now() + i,
          statement: currentStatement,
          result: errorData,
          timestamp: new Date().toLocaleTimeString()
        })

        // Guardar como ultimo resultado
        lastStatementResult = errorData
      }
    }

    // Mostrar en React la ultima sentencia ejecutada

    // Si la base de datos cambio durante el script, actualizar el estado
    if (currentDatabase !== database) {
      setDatabase(currentDatabase)
    }

    // Decidir que resultado mostrar en el panel de resultados:
    // - Si hubo algun SELECT, mostrar el resultado del ULTIMO select
    // - Si no hubo ningun SELECT, mostrar el resultado de la ultima sentencia
    if (lastSelectResult !== null) {
      setResult(lastSelectResult)
    } 
    else {
      setResult(lastStatementResult)
    }

    // Agregar las nuevas entradas al inicio del historial usando reverse() para que la primera sentencia quede arriba
    const orderedEntries = newHistoryEntries.reverse()
    setHistory(function (previousHistory) {
      return [...orderedEntries, ...previousHistory]
    })

    // Desactivar el estado de carga
    setLoading(false)
  }

  // Manejo del teclado
  const handleKeyDown = (e) => {
    // Verifica si la tecla Enter y Ctrl fueron presionadas
    if (e.ctrlKey && e.key === 'Enter') {
      // Ejecuta la sentencia SQL
      execute()
    }
  }

  // Limpiar el historial
  const clearHistory = () => {
    setHistory([])
    setResult(null)
  }

  // Se encarga de copiar la consulta al editor
  const loadQuery = (statement) => {
    setStatement(statement)
  }

  // Renderizado de la interfaz
  return (
    <div className="app">
      {/* HEADER de la app web */}
      <header className="app-header">

        {/* Título */}
        <div className="logo-container">
          <div className="logo-text">TinySQLDb</div>
          <div className="logo-subtitle">Motor de Base de Datos</div>
        </div>

        {/* Indica la base de datos activa */}
        <div className="header-context">
          <span className="context-label"> Base de datos: </span>
          <strong className="context-db">
            {/* Si la variable database no es vacia, mostrarla */}
            {database || 'Ninguna seleccionada'}
          </strong>
        </div>
      </header>

      {/* INTERFAZ PRINCIPAL */}
      <div className="main-layout">

        {/* COLUMNA 1: Editor SQL */}
        <div className="editor-panel">
          {/* Cabecera del panel */}
          <div className="panel-header">
            <h2>Editor SQL</h2>
            <span className="panel-helper">Ctrl+Enter para ejecutar</span>
          </div>

          {/* 
            TEXTAREA: Editor de SQL
            - value: El contenido del textarea (ligado al estado)
            - onChange: Actualiza el estado cuando el usuario escribe
            - onKeyDown: Detecta Ctrl+Enter
            - disabled: Deshabilita mientras se ejecuta una consulta
          */}

          <textarea
            className="sql-editor"
            value={statement}
            onChange={(e) => setStatement(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder="-- Escribe tu consulta SQL aquí --"
            rows={12}
            disabled={loading}
            spellCheck={false}
            autoCorrect="off"
          />

          {/* BOTONES BASICOS DEL EDITOR */}
          <div className="editor-actions">

            {/* Boton de ejecutar */}
            <button className="btn-execute" onClick={execute} disabled={loading || !statement.trim()}>
              {loading ? 'Ejecutando...' : '▶ Ejecutar'}
            </button>

            {/* Boton de limpiar */}
            <button className="btn-clear" onClick={() => setStatement('')} disabled={loading}>
              Limpiar
            </button>

            {/* Boton de borrar historial */}
            <button className="btn-clear-history" onClick={clearHistory} disabled={history.length === 0}>
              Limpiar Historial
            </button>
          </div>

          {/* BOTONES DE AYUDA DE SINTAXIS */}
          <div className="quick-examples">
            <span className="examples-label"> Ejemplos: </span>

            {/* Boton de CREATE DATABASE */}
            <button className="example-btn" onClick={() => setStatement('CREATE DATABASE ejemplo')}>
              CREATE DATABASE
            </button>

            {/* Boton de SET DATABASE */}
            <button className="example-btn" onClick={() => setStatement('SET DATABASE ejemplo')}>
              SET DATABASE
            </button>

            {/* Boton de CREATE TABLE */}
            <button className="example-btn" onClick={() => setStatement('CREATE TABLE Usuarios AS (ID INTEGER, Nombre VARCHAR(30), Edad INTEGER)')}>
              CREATE TABLE
            </button>

            {/* Boton de SELECT */}
            <button className="example-btn" onClick={() => setStatement('SELECT * FROM Usuarios')}>
              SELECT *
            </button>

            {/* Boton de INSERT INTO VALUES */}
            <button className="example-btn" onClick={() => setStatement('INSERT INTO Usuarios VALUES(1, "Ana", 25)')}>
              INSERT
            </button>
          </div>

          {/* HISTORIAL DE CONSULTAS */}
          {history.length > 0 && (
            <div className="history">
              <h3> Historial ({history.length})</h3>
              <div className="history-list">

                {/* Iterar sobre las consultas para mostrarlas en la web */}
                {history.map((item) => (
                  <div
                    key={item.id}
                    className={`history-item ${item.result.success ? 'success' : 'error'}`}
                    onClick={() => loadQuery(item.statement)}
                    title="Click para cargar esta consulta"
                  >
                    <span className="history-time">{item.timestamp}</span>
                    <span className="history-query">{item.statement}</span>

                  </div>
                ))}
              </div>
            </div>
          )}
        </div>

        {/* COLUMNA DERECHA: Panel de resultados */}
        <div className="results-panel">

          {/* Cabecera del panel con contador de filas */}
          <div className="panel-header">
            <h2> Resultados</h2>
            {result && (
              <span className="results-count">
                {result.rowCount || 0} filas
              </span>
            )}
          </div>

          {/* RESULTADO ULTIMA CONSULTA EJECUTADA */}
          <div className="results-content">
            {!result ? (
              // 
              // ESTADO VACÍO (No hay resultados)
              // Se muestra cuando el usuario aún no ha ejecutado ninguna consulta
              //
              <div className="empty-state">
                <p>Ejecuta una consulta para ver los resultados aquí</p>
                <p className="empty-sub">Las tablas y mensajes aparecerán en este panel</p>
              </div>
            ) : (
              // 
              // RESULTADO DE LA CONSULTA
              // Se muestra con colores según éxito/error
              //
              <div className={`result ${result.success ? 'success' : 'error'}`}>
                {/* Cabecera de resultado, muestra el tiempo y el mensaje */}
                <div className="result-header">
                  <span>
                    {result.success} {result.message}
                  </span>
                  <span className="time">{result.timeMs} ms</span>
                </div>

                {/* TABLA DE RESULTADOS */}
                {/* Se itera sobre las columnas y filas obtenidas para mostrar en la web */}
                {result.columns && result.columns.length > 0 && (
                  <div className="table-container">
                    <table>
                      <thead>
                        <tr>
                          {result.columns.map((col, i) => (
                            <th key={i}>{col}</th>
                          ))}
                        </tr>
                      </thead>
                      <tbody>
                        {result.rows.map((row, i) => (
                          <tr key={i}>
                            {row.map((cell, j) => (
                              <td key={j}>{cell}</td>
                            ))}
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                )}
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  )
}

export default App