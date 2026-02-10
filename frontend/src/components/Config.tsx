import { useState, useEffect } from 'react';
import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';

interface ConfigData {
  sync_enabled: boolean;
  sync_interval_seconds: number;
  central_db: {
    host: string;
    port: number;
    database: string;
    user: string;
    password: string;
    server_id: number;
  };
  regional_db: {
    host: string;
    port: number;
    database: string;
    user: string;
    password: string;
    server_id: number;
  };
  default_call_count: number;
  min_call_duration: number;
  max_call_duration: number;
  max_calls_per_request: number;
  version: number;
  last_updated: string;
}

export function Config() {
  const queryClient = useQueryClient();
  const [editing, setEditing] = useState(false);
  const [formData, setFormData] = useState<ConfigData | null>(null);

  const { data: configResponse, isLoading } = useQuery({
    queryKey: ['config'],
    queryFn: () => fetch('/api/config').then(r => r.json()),
    refetchInterval: 10000, // Обновление каждые 10 секунд
  });

  const updateMutation = useMutation({
    mutationFn: (data: ConfigData) =>
      fetch('/api/config', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      }).then(r => r.json()),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['config'] });
      setEditing(false);
    },
  });

  useEffect(() => {
    if (configResponse && !formData) {
      setFormData(configResponse);
    }
  }, [configResponse, formData]);

  if (isLoading) return <div className="loading">Загрузка...</div>;

  const config = configResponse as ConfigData;

  return (
    <div>
      <h1>⚙️ Конфигурация (application.conf)</h1>

      <div className="card">
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1rem' }}>
          <div>
            <strong>Версия:</strong> {config?.version} | 
            <strong> Обновлено:</strong> {config?.last_updated || 'N/A'}
          </div>
          {!editing && (
            <button onClick={() => { setEditing(true); setFormData(config); }}>
              Редактировать
            </button>
          )}
        </div>

        {!editing ? (
          <div>
            <h2>Синхронизация</h2>
            <table>
              <tbody>
                <tr>
                  <td><strong>Включена:</strong></td>
                  <td>
                    <span className={`badge ${config?.sync_enabled ? 'active' : 'inactive'}`}>
                      {config?.sync_enabled ? 'Да' : 'Нет'}
                    </span>
                  </td>
                </tr>
                <tr>
                  <td><strong>Интервал:</strong></td>
                  <td>{config?.sync_interval_seconds} секунд</td>
                </tr>
              </tbody>
            </table>

            <h2>Центральная БД</h2>
            <table>
              <tbody>
                <tr><td><strong>Host:</strong></td><td>{config?.central_db.host}</td></tr>
                <tr><td><strong>Port:</strong></td><td>{config?.central_db.port}</td></tr>
                <tr><td><strong>Database:</strong></td><td>{config?.central_db.database}</td></tr>
                <tr><td><strong>User:</strong></td><td>{config?.central_db.user}</td></tr>
                <tr><td><strong>Server ID:</strong></td><td>{config?.central_db.server_id}</td></tr>
              </tbody>
            </table>

            <h2>Региональная БД</h2>
            <table>
              <tbody>
                <tr><td><strong>Host:</strong></td><td>{config?.regional_db.host}</td></tr>
                <tr><td><strong>Port:</strong></td><td>{config?.regional_db.port}</td></tr>
                <tr><td><strong>Database:</strong></td><td>{config?.regional_db.database}</td></tr>
                <tr><td><strong>User:</strong></td><td>{config?.regional_db.user}</td></tr>
                <tr><td><strong>Server ID:</strong></td><td>{config?.regional_db.server_id}</td></tr>
              </tbody>
            </table>

            <h2>Параметры симуляции</h2>
            <table>
              <tbody>
                <tr><td><strong>Звонков по умолчанию:</strong></td><td>{config?.default_call_count}</td></tr>
                <tr><td><strong>Мин. длительность:</strong></td><td>{config?.min_call_duration} сек</td></tr>
                <tr><td><strong>Макс. длительность:</strong></td><td>{config?.max_call_duration} сек</td></tr>
                <tr><td><strong>Макс. за запрос:</strong></td><td>{config?.max_calls_per_request}</td></tr>
              </tbody>
            </table>
          </div>
        ) : (
          <div>
            <h2>Редактирование конфигурации</h2>
            
            <h3>Синхронизация</h3>
            <div className="form-group checkbox-inline">
              <input
                type="checkbox"
                checked={formData?.sync_enabled || false}
                onChange={(e) => setFormData({
                  ...formData!,
                  sync_enabled: e.target.checked
                })}
              />
              <label>Включить синхронизацию</label>
            </div>
            
            <div className="form-group">
              <label>Интервал синхронизации (секунды)</label>
              <input
                type="number"
                value={formData?.sync_interval_seconds || 60}
                onChange={(e) => setFormData({
                  ...formData!,
                  sync_interval_seconds: Number(e.target.value)
                })}
                className="no-spinner"
              />
            </div>

            <h3>Центральная БД</h3>
            <div className="form-group">
              <label>Host</label>
              <input
                type="text"
                value={formData?.central_db.host || ''}
                onChange={(e) => setFormData({
                  ...formData!,
                  central_db: { ...formData!.central_db, host: e.target.value }
                })}
              />
            </div>
            
            <div className="form-group">
              <label>Port</label>
              <input
                type="number"
                value={formData?.central_db.port || 5432}
                onChange={(e) => setFormData({
                  ...formData!,
                  central_db: { ...formData!.central_db, port: Number(e.target.value) }
                })}
                className="no-spinner"
              />
            </div>

            <div className="form-group">
              <label>Database</label>
              <input
                type="text"
                value={formData?.central_db.database || ''}
                onChange={(e) => setFormData({
                  ...formData!,
                  central_db: { ...formData!.central_db, database: e.target.value }
                })}
              />
            </div>

            <div className="form-group">
              <label>Server ID</label>
              <input
                type="number"
                value={formData?.central_db.server_id || 777}
                onChange={(e) => setFormData({
                  ...formData!,
                  central_db: { ...formData!.central_db, server_id: Number(e.target.value) }
                })}
                className="no-spinner"
              />
            </div>

            <h3>Параметры симуляции</h3>
            <div className="form-group">
              <label>Макс. длительность звонка (сек)</label>
              <input
                type="number"
                value={formData?.max_call_duration || 600}
                onChange={(e) => setFormData({
                  ...formData!,
                  max_call_duration: Number(e.target.value)
                })}
                className="no-spinner"
              />
            </div>

            <div className="form-group">
              <label>Макс. звонков за запрос</label>
              <input
                type="number"
                value={formData?.max_calls_per_request || 1000}
                onChange={(e) => setFormData({
                  ...formData!,
                  max_calls_per_request: Number(e.target.value)
                })}
                className="no-spinner"
              />
            </div>

            <div className="button-group-equal">
              <button 
                onClick={() => updateMutation.mutate(formData!)}
                disabled={updateMutation.isPending}
                className="success"
              >
                {updateMutation.isPending ? 'Сохранение...' : 'Сохранить'}
              </button>
              <button 
                onClick={() => { setEditing(false); setFormData(config); }}
                className="secondary"
              >
                Отмена
              </button>
            </div>

            {updateMutation.isSuccess && (
              <div className="success" style={{ marginTop: '1rem' }}>
                ✅ Конфигурация сохранена! Файл application.conf обновлен.
                <br />
                Новая версия: {updateMutation.data.version}
              </div>
            )}

            {updateMutation.isError && (
              <div className="error" style={{ marginTop: '1rem' }}>
                ❌ Ошибка: {(updateMutation.error as any)?.message || 'Неизвестная ошибка'}
              </div>
            )}
          </div>
        )}
      </div>

      <div className="card">
        <h2>ℹ️ Информация</h2>
        <ul style={{ lineHeight: '1.8' }}>
          <li><strong>Файл:</strong> backend/application.conf</li>
          <li><strong>Автообновление:</strong> Файл перечитывается каждые 10 секунд</li>
        </ul>
      </div>
    </div>
  );
}
