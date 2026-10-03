import React from 'react'
import ReactDOM from 'react-dom/client'
import App from './App.jsx'
import './index.css'

class ErrorBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { hasError: false, error: null, errorInfo: null };
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error };
  }

  componentDidCatch(error, errorInfo) {
    console.error("Dashboard caught error:", error, errorInfo);
    this.setState({ error, errorInfo });
  }

  render() {
    if (this.state.hasError) {
      return (
        <div style={{ padding: '2rem', fontFamily: 'sans-serif', maxWidth: '800px', margin: '2rem auto', background: '#fff', border: '1px solid #e2ddd9', borderRadius: '24px' }}>
          <h2 style={{ color: '#C14000', marginBottom: '1rem' }}>Dashboard Render Notice</h2>
          <p style={{ color: '#756C66', marginBottom: '1rem' }}>An error occurred while rendering the interface component:</p>
          <pre style={{ background: '#F3EFEC', padding: '1rem', borderRadius: '12px', overflowX: 'auto', fontSize: '13px', color: '#141413' }}>
            {this.state.error?.toString()}
          </pre>
          <button
            onClick={() => { this.setState({ hasError: false }); window.location.reload(); }}
            style={{ marginTop: '1.5rem', padding: '0.6rem 1.2rem', borderRadius: '666px', background: '#C14000', color: '#fff', border: 'none', cursor: 'pointer', fontWeight: 'bold' }}
          >
            Reload Dashboard
          </button>
        </div>
      );
    }
    return this.props.children;
  }
}

ReactDOM.createRoot(document.getElementById('root')).render(
  <React.StrictMode>
    <ErrorBoundary>
      <App />
    </ErrorBoundary>
  </React.StrictMode>,
)
