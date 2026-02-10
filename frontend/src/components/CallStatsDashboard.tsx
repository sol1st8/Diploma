import { useQuery } from '@tanstack/react-query';
import { apiClient } from '../api/client';
import type { CallStatisticsInfo, TarifInfo, TrunkInfo } from '../types';
import {
  LineChart, Line, BarChart, Bar, PieChart, Pie, Cell,
  XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer
} from 'recharts';

interface CallStatsAggregated {
  totalCalls: number;
  totalRevenue: number;
  callsByTarif: Record<number, { count: number; revenue: number }>;
  callsByTrunk: Record<number, { count: number; revenue: number }>;
}

export function CallStatsDashboard() {
  const { data: callsResponse, isLoading } = useQuery({
    queryKey: ['callStatistics'],
    queryFn: apiClient.getCallStatistics,
    refetchInterval: 5000,
  });

  const { data: tarifsResponse } = useQuery({
    queryKey: ['tarifs'],
    queryFn: apiClient.getTarifs,
  });

  const { data: trunksResponse } = useQuery({
    queryKey: ['trunks'],
    queryFn: apiClient.getTrunks,
  });

  if (isLoading) return <div className="loading">Загрузка статистики...</div>;

  const calls = (callsResponse?.data || []) as CallStatisticsInfo[];
  const tarifs = (tarifsResponse?.data || []) as TarifInfo[];
  const trunks = (trunksResponse?.data || []) as TrunkInfo[];

  // Создаем мапы для быстрого поиска названий
  const tarifMap = tarifs.reduce((acc, tarif) => {
    acc[tarif.id] = tarif.name;
    return acc;
  }, {} as Record<number, string>);

  const trunkMap = trunks.reduce((acc, trunk) => {
    acc[trunk.id] = trunk.name;
    return acc;
  }, {} as Record<number, string>);

  // Агрегируем данные
  const stats: CallStatsAggregated = {
    totalCalls: calls.length,
    totalRevenue: calls.reduce((sum, call) => sum + call.cost, 0),
    callsByTarif: {},
    callsByTrunk: {},
  };

  // Группируем по тарифам
  calls.forEach(call => {
    if (!stats.callsByTarif[call.tarif_id]) {
      stats.callsByTarif[call.tarif_id] = { count: 0, revenue: 0 };
    }
    stats.callsByTarif[call.tarif_id].count++;
    stats.callsByTarif[call.tarif_id].revenue += call.cost;
  });

  // Группируем по транкам
  calls.forEach(call => {
    if (!stats.callsByTrunk[call.trunk_id]) {
      stats.callsByTrunk[call.trunk_id] = { count: 0, revenue: 0 };
    }
    stats.callsByTrunk[call.trunk_id].count++;
    stats.callsByTrunk[call.trunk_id].revenue += call.cost;
  });

  const formatCurrency = (amount: number) => {
    return `${amount.toFixed(2)} ₽`;
  };

  // Подготовка данных для графиков
  const COLORS = ['#667eea', '#f093fb', '#4facfe', '#43e97b', '#fa709a', '#30cfd0', '#ff9a9e', '#ffecd2'];

  // Данные для круговой диаграммы по тарифам
  const tarifPieData = Object.entries(stats.callsByTarif)
    .sort(([, a], [, b]) => b.revenue - a.revenue)
    .slice(0, 8)
    .map(([tarifId, data]) => ({
      name: tarifMap[Number(tarifId)] || `Тариф ${tarifId}`,
      value: data.revenue,
      count: data.count
    }));

  // Данные для круговой диаграммы по транкам
  const trunkPieData = Object.entries(stats.callsByTrunk)
    .sort(([, a], [, b]) => b.count - a.count)
    .slice(0, 8)
    .map(([trunkId, data]) => ({
      name: trunkMap[Number(trunkId)] || `Транк ${trunkId}`,
      value: data.count,
      revenue: data.revenue
    }));

  // Данные для графика по времени (группировка по часам)
  const callsByTime = calls.reduce((acc, call) => {
    const hour = new Date(call.call_time).getHours();
    const key = `${hour.toString().padStart(2, '0')}:00`;
    if (!acc[key]) {
      acc[key] = { time: key, calls: 0, revenue: 0, duration: 0 };
    }
    acc[key].calls++;
    acc[key].revenue += call.cost;
    acc[key].duration += call.duration_seconds;
    return acc;
  }, {} as Record<string, { time: string; calls: number; revenue: number; duration: number }>);

  const timeChartData = Object.values(callsByTime).sort((a, b) => a.time.localeCompare(b.time));

  // Данные для графика распределения по длительности
  const durationRanges = [
    { label: '0-30с', min: 0, max: 30 },
    { label: '30-60с', min: 30, max: 60 },
    { label: '1-2м', min: 60, max: 120 },
    { label: '2-5м', min: 120, max: 300 },
    { label: '5-10м', min: 300, max: 600 },
    { label: '10+м', min: 600, max: Infinity }
  ];

  const durationChartData = durationRanges.map(range => {
    const callsInRange = calls.filter(c => c.duration_seconds >= range.min && c.duration_seconds < range.max);
    return {
      range: range.label,
      calls: callsInRange.length,
      revenue: callsInRange.reduce((sum, c) => sum + c.cost, 0)
    };
  });

  // Данные для графика распределения по стоимости
  const costRanges = [
    { label: '0-1₽', min: 0, max: 1 },
    { label: '1-5₽', min: 1, max: 5 },
    { label: '5-10₽', min: 5, max: 10 },
    { label: '10-20₽', min: 10, max: 20 },
    { label: '20-50₽', min: 20, max: 50 },
    { label: '50+₽', min: 50, max: Infinity }
  ];

  const costChartData = costRanges.map(range => {
    const callsInRange = calls.filter(c => c.cost >= range.min && c.cost < range.max);
    return {
      range: range.label,
      calls: callsInRange.length,
      revenue: callsInRange.reduce((sum, c) => sum + c.cost, 0)
    };
  });

  return (
    <div>
      <h1>📊 Дашборд статистики звонков</h1>

      {/* Графики */}
      <div className="card">
        <h2>📈 График звонков по времени</h2>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={timeChartData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" />
            <YAxis yAxisId="left" />
            <YAxis yAxisId="right" orientation="right" />
            <Tooltip />
            <Legend />
            <Line yAxisId="left" type="monotone" dataKey="calls" stroke="#667eea" strokeWidth={2} name="Количество звонков" />
            <Line yAxisId="right" type="monotone" dataKey="revenue" stroke="#f093fb" strokeWidth={2} name="Выручка (₽)" />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1.5rem' }}>
        {/* Круговая диаграмма по тарифам */}
        <div className="card">
          <h2>🎯 Распределение выручки по тарифам</h2>
          <ResponsiveContainer width="100%" height={300}>
            <PieChart>
              <Pie
                data={tarifPieData}
                cx="50%"
                cy="50%"
                labelLine={false}
                label={({ name, percent }) => `${name}: ${(percent * 100).toFixed(0)}%`}
                outerRadius={80}
                fill="#8884d8"
                dataKey="value"
              >
                {tarifPieData.map((entry, index) => (
                  <Cell key={`cell-${index}`} fill={COLORS[index % COLORS.length]} />
                ))}
              </Pie>
              <Tooltip formatter={(value: number) => formatCurrency(value)} />
            </PieChart>
          </ResponsiveContainer>
        </div>

        {/* Круговая диаграмма по транкам */}
        <div className="card">
          <h2>🔌 Распределение звонков по транкам</h2>
          <ResponsiveContainer width="100%" height={300}>
            <PieChart>
              <Pie
                data={trunkPieData}
                cx="50%"
                cy="50%"
                labelLine={false}
                label={({ name, percent }) => `${name}: ${(percent * 100).toFixed(0)}%`}
                outerRadius={80}
                fill="#8884d8"
                dataKey="value"
              >
                {trunkPieData.map((entry, index) => (
                  <Cell key={`cell-${index}`} fill={COLORS[index % COLORS.length]} />
                ))}
              </Pie>
              <Tooltip />
            </PieChart>
          </ResponsiveContainer>
        </div>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1.5rem' }}>
        {/* График распределения по длительности */}
        <div className="card">
          <h2>⏱️ Распределение по длительности</h2>
          <ResponsiveContainer width="100%" height={300}>
            <BarChart data={durationChartData}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="range" />
              <YAxis />
              <Tooltip />
              <Legend />
              <Bar dataKey="calls" fill="#4facfe" name="Количество звонков" />
            </BarChart>
          </ResponsiveContainer>
        </div>

        {/* График распределения по стоимости */}
        <div className="card">
          <h2>💰 Распределение по стоимости</h2>
          <ResponsiveContainer width="100%" height={300}>
            <BarChart data={costChartData}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="range" />
              <YAxis />
              <Tooltip />
              <Legend />
              <Bar dataKey="calls" fill="#43e97b" name="Количество звонков" />
            </BarChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Статистика по тарифам */}
      <div className="card">
        <h2>📈 Статистика по тарифам</h2>
        <table>
          <thead>
            <tr>
              <th>Тариф</th>
              <th>Количество звонков</th>
              <th>Выручка</th>
              <th>Средняя стоимость</th>
              <th>Доля от общей выручки</th>
            </tr>
          </thead>
          <tbody>
            {Object.entries(stats.callsByTarif)
              .sort(([, a], [, b]) => b.revenue - a.revenue)
              .map(([tarifId, data]) => (
                <tr key={tarifId}>
                  <td><strong>{tarifMap[Number(tarifId)] || `Тариф #${tarifId}`}</strong></td>
                  <td>{data.count}</td>
                  <td>{formatCurrency(data.revenue)}</td>
                  <td>{formatCurrency(data.revenue / data.count)}</td>
                  <td>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                      <div style={{ 
                        flex: 1, 
                        height: '8px', 
                        background: '#e2e8f0', 
                        borderRadius: '4px',
                        overflow: 'hidden'
                      }}>
                        <div style={{ 
                          width: `${(data.revenue / stats.totalRevenue * 100)}%`, 
                          height: '100%', 
                          background: 'linear-gradient(90deg, #667eea 0%, #764ba2 100%)',
                          transition: 'width 0.3s'
                        }} />
                      </div>
                      <span style={{ minWidth: '50px', textAlign: 'right' }}>
                        {(data.revenue / stats.totalRevenue * 100).toFixed(1)}%
                      </span>
                    </div>
                  </td>
                </tr>
              ))}
          </tbody>
        </table>
      </div>

      {/* Статистика по транкам */}
      <div className="card">
        <h2>🔌 Статистика по транкам</h2>
        <table>
          <thead>
            <tr>
              <th>Транк</th>
              <th>Количество звонков</th>
              <th>Выручка</th>
              <th>Средняя стоимость</th>
              <th>Загрузка</th>
            </tr>
          </thead>
          <tbody>
            {Object.entries(stats.callsByTrunk)
              .sort(([, a], [, b]) => b.count - a.count)
              .map(([trunkId, data]) => (
                <tr key={trunkId}>
                  <td><strong>{trunkMap[Number(trunkId)] || `Транк #${trunkId}`}</strong></td>
                  <td>{data.count}</td>
                  <td>{formatCurrency(data.revenue)}</td>
                  <td>{formatCurrency(data.revenue / data.count)}</td>
                  <td>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                      <div style={{ 
                        flex: 1, 
                        height: '8px', 
                        background: '#e2e8f0', 
                        borderRadius: '4px',
                        overflow: 'hidden'
                      }}>
                        <div style={{ 
                          width: `${(data.count / stats.totalCalls * 100)}%`, 
                          height: '100%', 
                          background: 'linear-gradient(90deg, #4facfe 0%, #00f2fe 100%)',
                          transition: 'width 0.3s'
                        }} />
                      </div>
                      <span style={{ minWidth: '50px', textAlign: 'right' }}>
                        {(data.count / stats.totalCalls * 100).toFixed(1)}%
                      </span>
                    </div>
                  </td>
                </tr>
              ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
