export interface HubInfo {
  id: number;
  name: string;
  location: string;
  is_active: boolean;
}

export interface ServerInfo {
  id: number;
  hub_id: number;
  name: string;
  ip_address: string;
  is_active: boolean;
}

export interface TrunkInfo {
  id: number;
  server_id: number;
  name: string;
  capacity: number;
  cost_per_channel: number;
}

export interface PricelistInfo {
  id: number;
  name: string;
  currency: string;
  rate_per_minute: number;
  is_active: boolean;
}

export interface TarifInfo {
  id: number;
  name: string;
  pricelist_id: number;
  markup_percent: number;
  free_minutes: number;
}

export interface CallStatisticsInfo {
  id: number;
  call_id: string;
  trunk_id: number;
  tarif_id: number;
  duration_seconds: number;
  cost: number;
  call_time: string;
}

export interface NasIpInfo {
  id: number;
  server_id: number;
  ip_address: string;
  description: string;
}

export interface SystemStats {
  database: {
    hubs: { total: number; active: number };
    servers: { total: number; active: number };
    trunks: number;
    nas_ips: number;
    tarifs: number;
    pricelists: { total: number; active: number };
  };
  calls: {
    total: number;
    total_revenue: number;
    total_duration_seconds: number;
    total_duration_minutes: number;
  };
}

export interface LoginResponse {
  success: boolean;
  accessToken: string;
  refreshToken: string;
  user: {
    email: string;
    name: string;
    role: string;
  };
}
