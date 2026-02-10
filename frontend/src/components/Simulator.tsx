import { useState } from 'react';
import { useMutation, useQueryClient, useQuery } from '@tanstack/react-query';
import { apiClient } from '../api/client';

interface ConfigData {
  max_calls_per_request: number;
}

export function Simulator() {
  const queryClient = useQueryClient();
  const [count, setCount] = useState(10);
  const [serverId, setServerId] = useState(159);

  // Получаем конфиг для динамического максимума
  const { data: configResponse } = useQuery({
    queryKey: ['config'],
    queryFn: () => fetch('/api/config').then(r => r.json()),
    refetchInterval: 10000,
  });

  const config = configResponse as ConfigData;
  const maxCalls = config?.max_calls_per_request || 1000;

  const simulateMutation = useMutation({
    mutationFn: apiClient.simulateCalls,
    onSuccess: (response) => {
      queryClient.invalidateQueries({ queryKey: ['callStatistics'] });
      queryClient.invalidateQueries({ queryKey: ['systemStats'] });
    },
  });

  const syncFullMutation = useMutation({
    mutationFn: apiClient.syncFull,
  });

  return (
    <div>
      <h1>🎲 Симулятор и интеграция</h1>

      {/* Генератор звонков */}
      <div className="card">
        <h2>Генератор звонков</h2>
        <p style={{ color: 'var(--text-secondary)', marginBottom: '1rem' }}>
          Генерирует тестовые звонки с автоматическим расчетом стоимости
        </p>
        
        <div className="form-group">
          <label>Количество звонков (1-{maxCalls})</label>
          <input
            type="number"
            min="1"
            max={maxCalls}
            value={count}
            onChange={(e) => setCount(Number(e.target.value))}
            className="no-spinner"
          />
        </div>

        <button
          onClick={() => simulateMutation.mutate(count)}
          disabled={simulateMutation.isPending}
          className="success"
        >
          {simulateMutation.isPending ? 'Генерация...' : '🎲 Сгенерировать звонки'}
        </button>

        {simulateMutation.isSuccess && (
          <div className="success" style={{ marginTop: '1rem' }}>
            ✅ Успешно сгенерировано: {simulateMutation.data.data.saved} звонков
            <br />
            Запрошено: {simulateMutation.data.data.requested}, 
            Сгенерировано: {simulateMutation.data.data.generated}
          </div>
        )}

        {simulateMutation.isError && (
          <div className="error" style={{ marginTop: '1rem' }}>
            ❌ Ошибка: {(simulateMutation.error as any)?.response?.data?.message || 'Неизвестная ошибка'}
          </div>
        )}
      </div>

      {/* Полная синхронизация */}
      <div className="card">
        <h2>Полная синхронизация сервера</h2>
        <p style={{ color: 'var(--text-secondary)', marginBottom: '1rem' }}>
          Вызывает полную синхронизацию всех таблиц на новом сервере
        </p>
        
        <div className="form-group">
          <label>Server ID</label>
          <input
            type="number"
            value={serverId}
            onChange={(e) => setServerId(Number(e.target.value))}
            className="no-spinner"
          />
        </div>

        <button
          onClick={() => syncFullMutation.mutate(serverId)}
          disabled={syncFullMutation.isPending}
          className="primary"
        >
          {syncFullMutation.isPending ? 'Синхронизация...' : '🔄 Запустить полную синхронизацию'}
        </button>

        {syncFullMutation.isSuccess && (
          <div className="success" style={{ marginTop: '1rem' }}>
            ✅ {syncFullMutation.data.data.message}
            <br />
            Server ID: {syncFullMutation.data.data.server_id}
            <br />
            {syncFullMutation.data.data.note}
          </div>
        )}

        {syncFullMutation.isError && (
          <div className="error" style={{ marginTop: '1rem' }}>
            ❌ Ошибка: {(syncFullMutation.error as any)?.response?.data?.message || 'Неизвестная ошибка'}
          </div>
        )}
      </div>

      {/* Информация */}
      <div className="card">
        <h2>ℹ️ Информация</h2>
        <ul style={{ lineHeight: '1.8' }}>
          <li><strong>Генератор звонков:</strong> Создает тестовые звонки с маршрутизацией через hub → server → trunk</li>
          <li><strong>Расчет стоимости:</strong> Автоматический расчет по формуле: базовая + наценка + стоимость транка</li>
          <li><strong>Полная синхронизация:</strong> Синхронизирует все таблицы (hub, server, nas_ip, trunk, pricelist, tarif, call_statistics)</li>
        </ul>
      </div>
    </div>
  );
}
