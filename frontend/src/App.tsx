import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { Layout } from './components/Layout';
import { Login } from './components/Login';
import { Dashboard } from './components/Dashboard';
import { Pricelists } from './components/Pricelists';
import { Tarifs } from './components/Tarifs';
import { Tables } from './components/Tables';
import { CallStatsDashboard } from './components/CallStatsDashboard';
import { Simulator } from './components/Simulator';
import { Config } from './components/Config';
import { Logs } from './components/Logs';

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      refetchOnWindowFocus: false,
      retry: 1,
    },
  },
});

function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <BrowserRouter>
        <Routes>
          <Route path="/login" element={<Login />} />
          <Route path="/" element={<Layout />}>
            <Route index element={<Dashboard />} />
            <Route path="pricelists" element={<Pricelists />} />
            <Route path="tarifs" element={<Tarifs />} />
            <Route path="tables" element={<Tables />} />
            <Route path="call-stats" element={<CallStatsDashboard />} />
            <Route path="simulator" element={<Simulator />} />
            <Route path="config" element={<Config />} />
            <Route path="logs" element={<Logs />} />
          </Route>
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </BrowserRouter>
    </QueryClientProvider>
  );
}

export default App;
