import { useState } from 'react'
import axios from 'axios'

function App() {
  const [respuesta, setRespuesta] = useState('')

  const probarConexion = async () => {
    try {
      const res = await axios.post('http://localhost:8080/query', {
        statement: 'TEST'
      })
      setRespuesta(JSON.stringify(res.data))
    } catch (e) {
      setRespuesta('Error: no se pudo conectar al servidor')
    }
  }

  return (
    <div>
      <h1>TinySQLDb</h1>
      <button onClick={probarConexion}>Probar conexión</button>
      <p>{respuesta}</p>
    </div>
  )
}

export default App