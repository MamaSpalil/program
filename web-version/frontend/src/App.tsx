import { Routes, Route, Navigate } from 'react-router-dom';
import { Box } from '@mui/material';
import { useAppSelector } from './hooks/redux';
import LoginPage from './pages/LoginPage';
import RegisterPage from './pages/RegisterPage';
import DashboardPage from './pages/DashboardPage';
import TradingPage from './pages/TradingPage';
import Layout from './components/Layout';

function App() {
  const { isAuthenticated } = useAppSelector((state) => state.auth);

  return (
    <Box sx={{ display: 'flex', minHeight: '100vh' }}>
      <Routes>
        <Route path="/login" element={<LoginPage />} />
        <Route path="/register" element={<RegisterPage />} />

        <Route
          path="/"
          element={
            isAuthenticated ? (
              <Layout />
            ) : (
              <Navigate to="/login" replace />
            )
          }
        >
          <Route index element={<DashboardPage />} />
          <Route path="trading" element={<TradingPage />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Route>
      </Routes>
    </Box>
  );
}

export default App;
