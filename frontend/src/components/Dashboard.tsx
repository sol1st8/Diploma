import { useState } from 'react';
import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';
import { apiClient } from '../api/client';
import type { SystemStats, CallStatisticsInfo, TrunkInfo, TarifInfo, ServerInfo } from '../types';

export function Dashboard() {
  const queryClient = useQueryClient();
  const [editingId, setEditingId] = useState<number | null>(null);
  const [formData, setFormData] = useState<Partial<TrunkInfo>>({});
  const [trunksPage, setTrunksPage] = useState(0);
  const [callsPage, setCallsPage] = useState(0);
  const [hoursPage, setHoursPage] = useState(0);
  const itemsPerPage = 10;

  const { data: statsResponse, isLoading: statsLoading } = useQuery({
    queryKey: ['systemStats'],
    queryFn: () => apiClient.systemStats(),
    refetchInterval: 30000, // Обновление каждые 30 сек
  });

  const { data: callsResponse } = useQuery({
    queryKey: ['callStatistics'],
    queryFn: () => apiClient.getCallStatistics(),
    refetchInterval: 30000,
  });

  const { data: trunksResponse } = useQuery({
    queryKey: ['trunks'],
    queryFn: () => apiClient.getTrunks(),
  });

  const { data: tarifsResponse } = useQuery({
    queryKey: ['tarifs'],
    queryFn: () => apiClient.getTarifs(),
  });

  const { data: serversResponse } = useQuery({
    queryKey: ['servers'],
    queryFn: () => apiClient.getServers(),
  });

  const addMutation = useMutation({
    mutationFn: apiClient.addTrunk,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['trunks'] });
      queryClient.invalidateQueries({ queryKey: ['systemStats'] });
      setEditingId(null);
      setFormData({});
    },
  });

  const updateMutation = useMutation({
    mutationFn: ({ id, data }: { id: number; data: TrunkInfo }) =>
      apiClient.updateTrunk(id, data),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['trunks'] });
      queryClient.invalidateQueries({ queryKey: ['systemStats'] });
      setEditingId(null);
      setFormData({});
    },
  });

  const stats = statsResponse?.data as SystemStats | undefined;
  const calls = callsResponse?.data as CallStatisticsInfo[] | undefined;
  const trunks = trunksResponse?.data as TrunkInfo[] | undefined;
  const tarifs = tarifsResponse?.data as TarifInfo[] | undefined;
  const servers = serversResponse?.data as ServerInfo[] | undefined;

  // Создаем мапы для быстрого поиска названий
  const trunkMap = trunks?.reduce((acc, trunk) => {
    acc[trunk.id] = trunk.name;
    return acc;
  }, {} as Record<number, string>);

  const tarifMap = tarifs?.reduce((acc, tarif) => {
    acc[tarif.id] = tarif.name;
    return acc;
  }, {} as Record<number, string>);

  const getServerName = (serverId: number) => {
    const server = servers?.find(s => s.id === serverId);
    return server ? server.name : `Server ${serverId}`;
  };

  const handleEdit = (trunk: TrunkInfo) => {
    setEditingId(trunk.id);
    setFormData(trunk);
  };

  const handleAdd = () => {
    setEditingId(-1);
    setFormData({
      id: Math.max(...(trunks?.map(t => t.id) || [0])) + 1,
      server_id: servers?.[0]?.id || 1,
      name: '',
      capacity: 30,
      cost_per_channel: 0.01,
    });
  };

  const handleSave = () => {
    // Проверка на пустой ID
    if (!formData.id || formData.id <= 0) {
      alert('ID не может быть пустым или отрицательным!');
      return;
    }
    // Проверка на дублирование ID при добавлении нового транка
    if (editingId === -1 && trunks?.some(t => t.id === formData.id)) {
      alert('Транк с таким ID уже существует!');
      return;
    }
    // Проверка на пустое название
    if (!formData.name || formData.name.trim() === '') {
      alert('Название транка не может быть пустым!');
      return;
    }
    // Формируем полные данные для отправки
    const trunkData: TrunkInfo = {
      id: formData.id!,
      server_id: formData.server_id || 1,
      name: formData.name,
      capacity: formData.capacity || 30,
      cost_per_channel: Number((formData.cost_per_channel ?? 0.0).toFixed(2)),
    };
    if (editingId === -1) {
      addMutation.mutate(trunkData);
    } else if (editingId) {
      updateMutation.mutate({ id: editingId, data: trunkData });
    }
  };

  const handleCancel = () => {
    setEditingId(null);
    setFormData({});
  };

  // Компонент пагинации
  const Pagination = ({ currentPage, totalItems, onPageChange }: { 
    currentPage: number; 
    totalItems: number; 
    onPageChange: (page: number) => void;
  }) => {
    const totalPages = Math.ceil(totalItems / itemsPerPage);
    if (totalPages <= 1) return null;

    return (
      <div style={{ 
        display: 'flex', 
        justifyContent: 'center', 
        alignItems: 'center', 
        gap: '0.5rem', 
        marginTop: '1rem',
        padding: '0.5rem'
      }}>
        <button 
          onClick={() => onPageChange(currentPage - 1)}
          disabled={currentPage === 0}
          className="secondary"
          style={{ padding: '0.5rem 1rem' }}
        >
          ← Назад
        </button>
        <span style={{ padding: '0 1rem' }}>
          Страница {currentPage + 1} из {totalPages}
        </span>
        <button 
          onClick={() => onPageChange(currentPage + 1)}
          disabled={currentPage >= totalPages - 1}
          className="secondary"
          style={{ padding: '0.5rem 1rem' }}
        >
          Вперед →
        </button>
      </div>
    );
  };

  if (statsLoading) {
    return <div className="loading">Загрузка...</div>;
  }

  // Группировка звонков по часам для графика
  const callsByHour = calls?.reduce((acc, call) => {
    const hour = call.call_time.substring(11, 13);
    if (!acc[hour]) {
      acc[hour] = { count: 0, revenue: 0 };
    }
    acc[hour].count++;
    acc[hour].revenue += call.cost;
    return acc;
  }, {} as Record<string, { count: number; revenue: number }>);

  return (
    <div className="dashboard">
      <h1>📊 Панель администратора</h1>
      
      {/* Карточки со статистикой */}
      <div className="stats-grid">
        <div className="stat-card">
          <h3>Всего звонков</h3>
          <div className="value">{stats?.calls.total || 0}</div>
        </div>
        
        <div className="stat-card">
          <h3>Выручка</h3>
          <div className="value">{stats?.calls.total_revenue.toFixed(2)} ₽</div>
        </div>
        
        <div className="stat-card">
          <h3>Активные хабы</h3>
          <div className="value">{stats?.database.hubs.active || 0}</div>
        </div>
        
        <div className="stat-card">
          <h3>Активные серверы</h3>
          <div className="value">{stats?.database.servers.active || 0}</div>
        </div>

        <div className="stat-card">
          <h3>Транки</h3>
          <div className="value">{stats?.database.trunks || 0}</div>
        </div>

        <div className="stat-card">
          <h3>Общая длительность</h3>
          <div className="value">{stats?.calls.total_duration_minutes || 0} мин</div>
        </div>
      </div>

      {/* Управление транками */}
      <div className="card">
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1rem' }}>
          <h2>📞 Управление транками</h2>
          <button onClick={handleAdd} className="success">
            + Добавить транк
          </button>
        </div>

        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Название</th>
              <th>Сервер</th>
              <th>Емкость</th>
              <th>Стоимость канала</th>
              <th>Действия</th>
            </tr>
          </thead>
          <tbody>
            {trunks
              ?.slice(trunksPage * itemsPerPage, (trunksPage + 1) * itemsPerPage)
              .map((trunk) => (
                <tr key={trunk.id}>
                  <td>{trunk.id}</td>
                  <td>{trunk.name}</td>
                  <td>{getServerName(trunk.server_id)}</td>
                  <td>{trunk.capacity}</td>
                  <td>{trunk.cost_per_channel.toFixed(2)} ₽</td>
                  <td>
                    <button onClick={() => handleEdit(trunk)}>
                      Редактировать
                    </button>
                  </td>
                </tr>
              ))}
          </tbody>
        </table>

        <Pagination 
          currentPage={trunksPage}
          totalItems={trunks?.length || 0}
          onPageChange={setTrunksPage}
        />

        {editingId !== null && (
          <div style={{ marginTop: '1.5rem', padding: '1.5rem', background: '#f8f9fa', borderRadius: '8px' }}>
            <h3>{editingId === -1 ? '➕ Добавить' : '✏️ Редактировать'} транк</h3>
            
            <div className="form-group">
              <label>ID</label>
              <input
                type="number"
                value={formData.id || ''}
                onChange={(e) => setFormData({ ...formData, id: Number(e.target.value) })}
                disabled={true}
                style={{ backgroundColor: '#e9ecef', cursor: 'not-allowed' }}
              />
              <small style={{ color: 'var(--text-secondary)', marginTop: '0.25rem', display: 'block' }}>
                ID назначается автоматически
              </small>
            </div>

            <div className="form-group">
              <label>Название</label>
              <input
                type="text"
                value={formData.name || ''}
                onChange={(e) => setFormData({ ...formData, name: e.target.value })}
                placeholder="Например: Trunk-MSK-01"
              />
            </div>

            <div className="form-group">
              <label>Сервер</label>
              <select
                value={formData.server_id || ''}
                onChange={(e) => setFormData({ ...formData, server_id: Number(e.target.value) })}
              >
                {servers?.map((server) => (
                  <option key={server.id} value={server.id}>
                    {server.name} ({server.ip_address})
                  </option>
                ))}
              </select>
            </div>

            <div className="form-group">
              <label>Емкость (каналов)</label>
              <input
                type="number"
                step="1"
                value={formData.capacity || 0}
                onChange={(e) => {
                  const value = e.target.value;
                  // Принимаем только целые числа
                  if (value === '' || /^\d+$/.test(value)) {
                    setFormData({ ...formData, capacity: Number(value) });
                  }
                }}
                min="1"
              />
              <small style={{ color: 'var(--text-secondary)', marginTop: '0.25rem', display: 'block' }}>
                Требуется ввести целое число
              </small>
            </div>

            <div className="form-group">
              <label>Стоимость канала (₽)</label>
              <input
                type="text"
                inputMode="decimal"
                value={formData.cost_per_channel !== undefined ? formData.cost_per_channel.toFixed(2) : '0.01'}
                onChange={(e) => {
                  const value = e.target.value;
                  // Принимаем любое числовое значение с точкой или запятой, минимум 0.01
                  if (value === '' || /^\d*[.,]?\d*$/.test(value)) {
                    const normalizedValue = value.replace(',', '.');
                    const numValue = normalizedValue === '' ? 0.01 : parseFloat(normalizedValue);
                    // Минимальное значение 0.01
                    setFormData({ ...formData, cost_per_channel: numValue < 0.01 ? 0.01 : numValue });
                  }
                }}
              />
              <small style={{ color: 'var(--text-secondary)', marginTop: '0.25rem', display: 'block' }}>
                Минимальное значение: 0.01 ₽
              </small>
            </div>

            <div className="button-group-equal">
              <button onClick={handleSave} className="success">
                💾 Сохранить
              </button>
              <button onClick={handleCancel} className="secondary">
                ✖️ Отмена
              </button>
            </div>
          </div>
        )}
      </div>

      {/* Последние звонки */}
      <div className="card">
        <h2>Последние звонки</h2>
        <table>
          <thead>
            <tr>
              <th>ID звонка</th>
              <th>Транк</th>
              <th>Тариф</th>
              <th>Длительность</th>
              <th>Стоимость</th>
              <th>Время</th>
            </tr>
          </thead>
          <tbody>
            {calls
              ?.slice()
              .reverse()
              .slice(callsPage * itemsPerPage, (callsPage + 1) * itemsPerPage)
              .map((call) => (
                <tr key={call.id}>
                  <td>{call.call_id}</td>
                  <td>{trunkMap?.[call.trunk_id] || `ID: ${call.trunk_id}`}</td>
                  <td>{tarifMap?.[call.tarif_id] || `ID: ${call.tarif_id}`}</td>
                  <td>{Math.floor(call.duration_seconds / 60)}:{(call.duration_seconds % 60).toString().padStart(2, '0')}</td>
                  <td>{call.cost.toFixed(2)} ₽</td>
                  <td>{call.call_time}</td>
                </tr>
              ))}
          </tbody>
        </table>

        <Pagination 
          currentPage={callsPage}
          totalItems={calls?.length || 0}
          onPageChange={setCallsPage}
        />
      </div>

      {/* Статистика по часам */}
      {callsByHour && Object.keys(callsByHour).length > 0 && (
        <div className="card">
          <h2>Звонки по часам</h2>
          <table>
            <thead>
              <tr>
                <th>Час</th>
                <th>Количество</th>
                <th>Выручка</th>
              </tr>
            </thead>
            <tbody>
              {Object.entries(callsByHour)
                .sort(([a], [b]) => a.localeCompare(b))
                .slice(hoursPage * itemsPerPage, (hoursPage + 1) * itemsPerPage)
                .map(([hour, data]) => (
                  <tr key={hour}>
                    <td>{hour}:00</td>
                    <td>{data.count}</td>
                    <td>{data.revenue.toFixed(2)} ₽</td>
                  </tr>
                ))}
            </tbody>
          </table>

          <Pagination 
            currentPage={hoursPage}
            totalItems={Object.keys(callsByHour).length}
            onPageChange={setHoursPage}
          />
        </div>
      )}
    </div>
  );
}
