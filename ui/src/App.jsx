import { BrowserRouter, Routes, Route } from 'react-router-dom';
import { SimulationProvider } from './context/SimulationContext';
import MainLayout from './layouts/MainLayout';
import DashboardPage from './pages/DashboardPage';
import Phase1Page from './pages/Phase1Page';
import Phase2Page from './pages/Phase2Page';
import Phase3Page from './pages/Phase3Page';
import Phase4Page from './pages/Phase4Page';

export default function App() {
  return (
    <SimulationProvider>
      <BrowserRouter>
        <Routes>
          <Route element={<MainLayout />}>
            <Route index element={<DashboardPage />} />
            <Route path="fase-1" element={<Phase1Page />} />
            <Route path="fase-2" element={<Phase2Page />} />
            <Route path="fase-3" element={<Phase3Page />} />
            <Route path="fase-4" element={<Phase4Page />} />
          </Route>
        </Routes>
      </BrowserRouter>
    </SimulationProvider>
  );
}
