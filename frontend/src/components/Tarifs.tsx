import { useState } from 'react';
import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';
import { apiClient } from '../api/client';
import type { TarifInfo, PricelistInfo } from '../types';

export function Tarifs() {
  const queryClient = useQueryClient();
  const [editingId, setEditingId] = useState<number | null>(null);
  const [formData, setFormData] = useState<Partial<TarifInfo>>({});

  const { data: tarifsResponse, isLoading } = useQuery({
    queryKey: ['tarifs'],
    queryFn: () => apiClient.getTarifs(),
  });

  const { data: pricelistsResponse } = useQuery({
    queryKey: ['pricelists'],
    queryFn: () => apiClient.getPricelists(),
  });

  const addMutation = useMutation({
    mutationFn: apiClient.addTarif,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['tarifs'] });
      setEditingId(null);
      setFormData({});
    },
  });

  const updateMutation = useMutation({
    mutationFn: ({ id, data }: { id: number; data: TarifInfo }) =>
      apiClient.updateTarif(id, data),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['tarifs'] });
      setEditingId(null);
      setFormData({});
    },
  });



  const tarifs = tarifsResponse?.data as TarifInfo[] | undefined;
  const pricelists = pricelistsResponse?.data as PricelistInfo[] | undefined;

  const getPricelistName = (id: number) => {
    return pricelists?.find(p => p.id === id)?.name || `ID: ${id}`;
  };

  const handleEdit = (tarif: TarifInfo) => {
    setEditingId(tarif.id);
    setFormData(tarif);
  };

  const handleAdd = () => {
    setEditingId(-1);
    setFormData({
      id: Math.max(...(tarifs?.map(t => t.id) || [0])) + 1,
      name: '',
      pricelist_id: pricelists?.[0]?.id || 1,
      markup_percent: 0,
      free_minutes: 0,
    });
  };

  const handleSave = () => {
    // Проверка на пустое название
    if (!formData.name || formData.name.trim() === '') {
      alert('Название тарифа не может быть пустым!');
      return;
    }
    // Проверка на пустой ID
    if (!formData.id || formData.id <= 0) {
      alert('ID не может быть пустым или отрицательным!');
      return;
    }
    // Проверка на дублирование ID при добавлении
    if (editingId === -1 && tarifs?.some(t => t.id === formData.id)) {
      alert('Тариф с таким ID уже существует!');
      return;
    }
    if (editingId === -1) {
      addMutation.mutate(formData as TarifInfo);
    } else if (editingId) {
      updateMutation.mutate({ id: editingId, data: formData as TarifInfo });
    }
  };

  if (isLoading) return <div className="loading">Загрузка...</div>;

  return (
    <div>
      <h1>📋 Тарифы</h1>
      
      <button onClick={handleAdd} className="success" style={{ marginBottom: '1rem' }}>
        + Добавить тариф
      </button>

      <div className="card">
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Название</th>
              <th>Прайс-лист</th>
              <th>Наценка (%)</th>
              <th>Бесплатные минуты</th>
              <th>Действия</th>
            </tr>
          </thead>
          <tbody>
            {tarifs?.map((tarif) => (
              <tr key={tarif.id}>
                <td>{tarif.id}</td>
                <td>{tarif.name}</td>
                <td>{getPricelistName(tarif.pricelist_id)}</td>
                <td>{tarif.markup_percent}%</td>
                <td>{tarif.free_minutes}</td>
                <td>
                  <div className="actions">
                    <button onClick={() => handleEdit(tarif)}>
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
          <h2>{editingId === -1 ? 'Добавить' : 'Редактировать'} тариф</h2>
          
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
            <label>Прайс-лист</label>
            <select
              value={formData.pricelist_id || ''}
              onChange={(e) => setFormData({ ...formData, pricelist_id: Number(e.target.value) })}
            >
              {pricelists?.map((pl) => (
                <option key={pl.id} value={pl.id}>
                  {pl.name} ({pl.rate_per_minute} ₽/мин)
                </option>
              ))}
            </select>
          </div>

          <div className="form-group">
            <label>Наценка (%)</label>
            <input
              type="text"
              inputMode="numeric"
              value={formData.markup_percent !== undefined ? formData.markup_percent.toString() : '0'}
              onChange={(e) => {
                const value = e.target.value;
                // Принимаем только целые числа, минимум 0
                if (value === '' || /^\d+$/.test(value)) {
                  const numValue = value === '' ? 0 : parseInt(value, 10);
                  setFormData({ ...formData, markup_percent: numValue < 0 ? 0 : numValue });
                }
              }}
            />
            <small style={{ color: 'var(--text-secondary)', marginTop: '0.25rem', display: 'block' }}>
              Только целые числа, минимум 0
            </small>
          </div>

          <div className="form-group">
            <label>Бесплатные минуты</label>
            <input
              type="text"
              inputMode="numeric"
              value={formData.free_minutes !== undefined ? formData.free_minutes.toString() : '0'}
              onChange={(e) => {
                const value = e.target.value;
                // Принимаем только целые числа, минимум 0
                if (value === '' || /^\d+$/.test(value)) {
                  const numValue = value === '' ? 0 : parseInt(value, 10);
                  setFormData({ ...formData, free_minutes: numValue < 0 ? 0 : numValue });
                }
              }}
            />
            <small style={{ color: 'var(--text-secondary)', marginTop: '0.25rem', display: 'block' }}>
              Только целые числа, минимум 0
            </small>
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
