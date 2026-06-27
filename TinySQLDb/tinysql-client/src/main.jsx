import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'

// importar los estilo sglobales de la aplicacion
import './index.css'

// importar la logica de la app
import App from './App.jsx'

// Creamos el root (raíz) de React en el elemento con id 'root'
createRoot(document.getElementById('root')).render(
  // tomamos el componente app y lo renderizamos
  <StrictMode>
    <App />
  </StrictMode>,
)
