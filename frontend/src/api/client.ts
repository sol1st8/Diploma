import axios from 'axios';
import type { PricelistInfo, TarifInfo, TrunkInfo, LoginResponse, SystemStats } from '../types';

const api = axios.create({
  baseURL: '/api',
  headers: {
    'Content-Type': 'application/json',
  },
});

// Интерцептор для добавления токена
api.interceptors.request.use((config) => {
  const token = localStorage.getItem('accessToken');
  if (token) {
    config.headers.Authorization = token;
  }
  return config;
});

export const apiClient = {
  // Аутентификация
  login: (email: string, password: string) =>
    api.post<LoginResponse>('/login', { email, password }),
  
  logout: (token: string) =>
    api.post('/logout', { token }),

  // Получение данных
  getHubs: () => api.get('/get/hub'),
  getServers: () => api.get('/get/server'),
  getTrunks: () => api.get('/get/trunk'),
  getPricelists: () => api.get('/get/pricelist'),
  getTarifs: () => api.get('/get/tarif'),
  getCallStatistics: () => api.get('/get/call-statistics'),
  getNasIps: () => api.get('/get/nas-ip'),

  // Управление прайс-листами
  addPricelist: (data: PricelistInfo) =>
    api.post('/add/pricelist', data),
  
  updatePricelist: (id: number, data: PricelistInfo) =>
    api.put(`/update/pricelist/${id}`, data),
  
  // Управление тарифами
  addTarif: (data: TarifInfo) =>
    api.post('/add/tarif', data),
  
  updateTarif: (id: number, data: TarifInfo) =>
    api.put(`/update/tarif/${id}`, data),
  
  // Управление транками
  addTrunk: (data: TrunkInfo) =>
    api.post('/add/trunk', data),
  
  updateTrunk: (id: number, data: TrunkInfo) =>
    api.put(`/update/trunk/${id}`, data),
  
  // Симуляция звонков
  simulateCalls: (count: number) =>
    api.post('/simulate/calls', { count }),

  // Синхронизация
  syncTrigger: () => api.post('/sync/trigger'),
  syncStatus: () => api.get('/sync/status'),
  syncFull: (serverId: number) =>
    api.post('/sync/full', { server_id: serverId }),

  // Система
  systemHealth: () => api.get('/system/health'),
  systemStats: () => api.get<SystemStats>('/system/stats'),

  // Логи
  getLogs: (lines: number = 100) => api.get(`/logs`),
};
