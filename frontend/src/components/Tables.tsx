import { useQuery } from '@tanstack/react-query';
import { apiClient } from '../api/client';
import type { HubInfo, ServerInfo, NasIpInfo } from '../types';

export function Tables() {

  const { data: hubsResponse } = useQuery({
    queryKey: ['hubs'],
    queryFn: () => apiClient.getHubs(),
  });

  const { data: serversResponse } = useQuery({
    queryKey: ['servers'],
    queryFn: () => apiClient.getServers(),
  });

  const { data: nasIpsResponse } = useQuery({
    queryKey: ['nasIps'],
    queryFn: () => apiClient.getNasIps(),
  });

  const hubs = hubsResponse?.data as HubInfo[] | undefined;
  const servers = serversResponse?.data as ServerInfo[] | undefined;
  const nasIps = nasIpsResponse?.data as NasIpInfo[] | undefined;


  return (
    <div>
      <h1>📁 Системные справочники</h1>

      <div className="card">
        <h2>Хабы</h2>
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Название</th>
              <th>Локация</th>
              <th>Статус</th>
            </tr>
          </thead>
          <tbody>
            {hubs?.map((hub) => (
              <tr key={hub.id}>
                <td>{hub.id}</td>
                <td>{hub.name}</td>
                <td>{hub.location}</td>
                <td>
                  <span className={`badge ${hub.is_active ? 'active' : 'inactive'}`}>
                    {hub.is_active ? 'Активен' : 'Неактивен'}
                  </span>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <div className="card">
        <h2>Серверы</h2>
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>ID хаба</th>
              <th>Название</th>
              <th>IP адрес</th>
              <th>Статус</th>
            </tr>
          </thead>
          <tbody>
            {servers?.map((server) => (
              <tr key={server.id}>
                <td>{server.id}</td>
                <td>{server.hub_id}</td>
                <td>{server.name}</td>
                <td>{server.ip_address}</td>
                <td>
                  <span className={`badge ${server.is_active ? 'active' : 'inactive'}`}>
                    {server.is_active ? 'Активен' : 'Неактивен'}
                  </span>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <div className="card">
        <h2>IP-Адреса</h2>
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>ID сервера</th>
              <th>IP адрес</th>
              <th>Описание</th>
            </tr>
          </thead>
          <tbody>
            {nasIps?.map((nasIp) => (
              <tr key={nasIp.id}>
                <td>{nasIp.id}</td>
                <td>{nasIp.server_id}</td>
                <td>{nasIp.ip_address}</td>
                <td>{nasIp.description}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
