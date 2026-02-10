import { useState } from 'react';
import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';
import { apiClient } from '../api/client';
import type { PricelistInfo } from '../types';

export function Pricelists() {
  const queryClient = useQueryClient();
  const [editingId, setEditingId] = useState<number | null>(null);
  const [formData, setFormData] = useState<Partial<PricelistInfo>>({});

  const { data: pricelistsResponse, isLoading } = useQuery({
    queryKey: ['pricelists'],
    queryFn: () => apiClient.getPricelists(),
  });

  const addMutation = useMutation({
    mutationFn: apiClient.addPricelist,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['pricelists'] });
      setEditingId(null);
      setFormData({});
    },
  });

  const updateMutation = useMutation({
    mutationFn: ({ id, data }: { id: number; data: PricelistInfo }) =>
      apiClient.updatePricelist(id, data),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['pricelists'] });
      setEditingId(null);
      setFormData({});
    },
  });



  const pricelists = pricelistsResponse?.data as PricelistInfo[] | undefined;

  const handleEdit = (pricelist: PricelistInfo) => {
    setEditingId(pricelist.id);
    setFormData(pricelist);
  };

  const handleAdd = () => {
    setEditingId(-1);
    setFormData({
      id: Math.max(...(pricelists?.map(p => p.id) || [0])) + 1,
      name: '',
      currency: 'RUB',
      rate_per_minute: 0.01,
      is_active: true,
    });
  };

  const handleSave = () => {
    // Проверка на пустое название
    if (!formData.name || formData.name.trim() === '') {
      alert('Название прайс-листа не может быть пустым!');
      return;
    }
    // Проверка на пустой ID
    if (!formData.id || formData.id <= 0) {
      alert('ID не может быть пустым или отрицательным!');
      return;
    }
    // Проверка на дублирование ID при добавлении
    if (editingId === -1 && pricelists?.some(p => p.id === formData.id)) {
      alert('Прайс-лист с таким ID уже существует!');
      return;
    }
    if (editingId === -1) {
      addMutation.mutate(formData as PricelistInfo);
    } else if (editingId) {
      updateMutation.mutate({ id: editingId, data: formData as PricelistInfo });
    }
  };

  if (isLoading) return <div className="loading">Загрузка...</div>;

  return (
    <div>
      <h1>💰 Прайс-листы</h1>
      
      <button onClick={handleAdd} className="success" style={{ marginBottom: '1rem' }}>
        + Добавить прайс-лист
      </button>

      <div className="card">
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Название</th>
              <th>Валюта</th>
              <th>Цена за минуту</th>
              <th>Статус</th>
              <th>Действия</th>
            </tr>
          </thead>
          <tbody>
            {pricelists?.map((pricelist) => (
              <tr key={pricelist.id}>
                <td>{pricelist.id}</td>
                <td>{pricelist.name}</td>
                <td>{pricelist.currency}</td>
                <td>{pricelist.rate_per_minute.toFixed(2)} ₽</td>
                <td>
                  <span className={`badge ${pricelist.is_active ? 'active' : 'inactive'}`}>
                    {pricelist.is_active ? 'Активен' : 'Неактивен'}
                  </span>
                </td>
                <td>
                  <div className="actions">
                    <button onClick={() => handleEdit(pricelist)}>
                      Редактировать
                    </button>
                  </div>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {editingId !== null && (
        <div className="card" style={{ marginTop: '1rem' }}>
          <h2>{editingId === -1 ? 'Добавить' : 'Редактировать'} прайс-лист</h2>
          
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
            />
          </div>

          <div className="form-group">
            <label>Валюта</label>
            <select
              value={formData.currency || 'RUB'}
              onChange={(e) => setFormData({ ...formData, currency: e.target.value })}
            >
              <option value="RUB">RUB</option>
              <option value="USD">USD</option>
              <option value="EUR">EUR</option>
            </select>
          </div>

          <div className="form-group">
            <label>Цена за минуту</label>
            <input
              type="text"
              inputMode="decimal"
              value={formData.rate_per_minute !== undefined ? formData.rate_per_minute.toFixed(2) : '0.01'}
              onChange={(e) => {
                const value = e.target.value;
                // Принимаем любое числовое значение с точкой или запятой, минимум 0.01
                if (value === '' || /^\d*[.,]?\d*$/.test(value)) {
                  const normalizedValue = value.replace(',', '.');
                  const numValue = normalizedValue === '' ? 0.01 : parseFloat(normalizedValue);
                  // Минимальное значение 0.01
                  setFormData({ ...formData, rate_per_minute: numValue < 0.01 ? 0.01 : numValue });
                }
              }}
            />
            <small style={{ color: 'var(--text-secondary)', marginTop: '0.25rem', display: 'block' }}>
              Минимальное значение: 0.01 ₽
            </small>
          </div>

          <div className="form-group checkbox-inline">
            <label>Активен</label>
            <input
              type="checkbox"
              checked={formData.is_active || false}
              onChange={(e) => setFormData({ ...formData, is_active: e.target.checked })}
            />
          </div>

          <div className="actions button-group-equal">
            <button onClick={handleSave} className="success">
              Сохранить
            </button>
            <button onClick={() => { setEditingId(null); setFormData({}); }} className="secondary">
              Отмена
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
