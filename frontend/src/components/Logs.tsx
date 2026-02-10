import { useState, useEffect, useRef, useMemo } from 'react';
import { useQuery } from '@tanstack/react-query';
import { apiClient } from '../api/client';

interface LogsResponse {
  logs: string[];
  count: number;
  total: number;
}

interface LogStats {
  INFO: number;
  WARNING: number;
  ERROR: number;
  DEBUG: number;
  UNKNOWN: number;
}

type LogLevel = 'INFO' | 'WARNING' | 'ERROR' | 'DEBUG' | 'UNKNOWN' | 'ALL';

export function Logs() {
  const [linesCount, setLinesCount] = useState(100);
  const [autoRefresh, setAutoRefresh] = useState(true);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedLevel, setSelectedLevel] = useState<LogLevel>('ALL');
  const logsEndRef = useRef<HTMLDivElement>(null);

  const { data: logsResponse, isLoading, error } = useQuery<LogsResponse>({
    queryKey: ['logs', linesCount],
    queryFn: async () => {
      const response = await apiClient.getLogs(linesCount);
      console.log('Raw API response:', response);
      console.log('Response data:', response.data);
      return response.data as LogsResponse;
    },
    refetchInterval: autoRefresh ? 5000 : false,
  });

  const logs = logsResponse?.logs || [];
  const count = logsResponse?.count || 0;
  const total = logsResponse?.total || 0;

  // Отладочная информация
  console.log('Logs component state:', {
    logsResponse,
    logs,
    logsLength: logs.length,
    count,
    total,
    isLoading,
    error
  });

  // Вычисляем статистику по уровням логов
  const logStats = useMemo<LogStats>(() => {
    const stats: LogStats = {
      INFO: 0,
      WARNING: 0,
      ERROR: 0,
      DEBUG: 0,
      UNKNOWN: 0,
    };

    logs.forEach((log) => {
      if (log.includes('[INFO]')) stats.INFO++;
      else if (log.includes('[WARNING]')) stats.WARNING++;
      else if (log.includes('[ERROR]')) stats.ERROR++;
      else if (log.includes('[DEBUG]')) stats.DEBUG++;
      else stats.UNKNOWN++;
    });

    return stats;
  }, [logs]);

  // Фильтруем логи по уровню и поисковому запросу
  const filteredLogs = useMemo(() => {
    let filtered = logs;

    // Фильтр по уровню
    if (selectedLevel !== 'ALL') {
      filtered = filtered.filter((log) => log.includes(`[${selectedLevel}]`));
    }

    // Фильтр по поисковому запросу
    if (searchQuery.trim()) {
      const query = searchQuery.toLowerCase();
      filtered = filtered.filter((log) => log.toLowerCase().includes(query));
    }

    return filtered;
  }, [logs, selectedLevel, searchQuery]);

  // Автопрокрутка вниз при новых логах
  useEffect(() => {
    if (autoRefresh && logsEndRef.current) {
      logsEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [logs, autoRefresh]);

  const getLogLevel = (logLine: string): LogLevel => {
    if (logLine.includes('[ERROR]')) return 'ERROR';
    if (logLine.includes('[WARNING]')) return 'WARNING';
    if (logLine.includes('[INFO]')) return 'INFO';
    if (logLine.includes('[DEBUG]')) return 'DEBUG';
    return 'UNKNOWN';
  };

  const formatLogLine = (logLine: string) => {
    // Разбиваем строку на части: timestamp, level, message
    const match = logLine.match(/^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}) \[(\w+)\] (.+)$/);
    if (match) {
      const [, timestamp, level, message] = match;
      
      // Подсветка поискового запроса
      let highlightedMessage = message;
      if (searchQuery.trim()) {
        const regex = new RegExp(`(${searchQuery})`, 'gi');
        highlightedMessage = message.replace(regex, '<mark>$1</mark>');
      }
      
      return (
        <>
          <span className="log-timestamp">{timestamp}</span>
          <span className={`log-level log-level-${level.toLowerCase()}`}>[{level}]</span>
          <span 
            className="log-message" 
            dangerouslySetInnerHTML={{ __html: highlightedMessage }}
          />
        </>
      );
    }
    return logLine;
  };

  const getLevelColor = (level: LogLevel): string => {
    switch (level) {
      case 'INFO': return '#4ec9b0';
      case 'WARNING': return '#dcdcaa';
      case 'ERROR': return '#f48771';
      case 'DEBUG': return '#9cdcfe';
      default: return '#858585';
    }
  };

  const getLevelIcon = (level: LogLevel): string => {
    switch (level) {
      case 'INFO': return 'ℹ️';
      case 'WARNING': return '⚠️';
      case 'ERROR': return '❌';
      case 'DEBUG': return '🔍';
      default: return '📝';
    }
  };

  if (isLoading) {
    return (
      <div className="logs-container">
        <h1>📋 Логирование событий</h1>
        <div className="card">
          <div className="loading">Загрузка логов...</div>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="logs-container">
        <h1>📋 Логирование событий</h1>
        <div className="card">
          <div className="error-message">
            <div className="error-icon">⚠️</div>
            <div>Ошибка загрузки логов</div>
            <div className="error-details">{String(error)}</div>
          </div>
        </div>
      </div>
    );
  }

  const totalLogs = Object.values(logStats).reduce((sum, val) => sum + val, 0);

  return (
    <div className="logs-container">
      <h1>📋 Логирование событий</h1>

      {/* Статистика по уровням логов */}
      <div className="stats-grid">
        {(['INFO', 'WARNING', 'ERROR', 'DEBUG'] as const).map((level) => {
          const percentage = totalLogs > 0 ? (logStats[level] / totalLogs) * 100 : 0;
          return (
            <div 
              key={level} 
              className={`stat-card ${selectedLevel === level ? 'stat-card-active' : ''}`}
              onClick={() => setSelectedLevel(selectedLevel === level ? 'ALL' : level)}
              style={{ cursor: 'pointer' }}
            >
              <div className="stat-header">
                <span className="stat-icon">{getLevelIcon(level)}</span>
                <span className="stat-label">{level}</span>
              </div>
              <div className="stat-value">{logStats[level]}</div>
              <div className="stat-bar">
                <div 
                  className="stat-bar-fill" 
                  style={{ 
                    width: `${percentage}%`,
                    backgroundColor: getLevelColor(level)
                  }}
                />
              </div>
              <div className="stat-percentage">{percentage.toFixed(1)}%</div>
            </div>
          );
        })}
      </div>

      <div className="card">
        {/* Панель управления */}
        <div className="controls-panel">
          <div className="controls-row">
            <div className="control-group">
              <label>
                Показать последних:
                <select 
                  value={linesCount} 
                  onChange={(e) => setLinesCount(Number(e.target.value))}
                  className="control-select"
                >
                  <option value={50}>50</option>
                  <option value={100}>100</option>
                  <option value={200}>200</option>
                  <option value={500}>500</option>
                  <option value={1000}>1000</option>
                </select>
              </label>
            </div>

            <div className="control-group">
              <label className="checkbox-label">
                <input
                  type="checkbox"
                  checked={autoRefresh}
                  onChange={(e) => setAutoRefresh(e.target.checked)}
                />
                Автообновление (5 сек)
              </label>
            </div>

            <div className="control-group">
              <label>
                Уровень:
                <select 
                  value={selectedLevel} 
                  onChange={(e) => setSelectedLevel(e.target.value as LogLevel)}
                  className="control-select"
                >
                  <option value="ALL">Все уровни</option>
                  <option value="INFO">INFO</option>
                  <option value="WARNING">WARNING</option>
                  <option value="ERROR">ERROR</option>
                  <option value="DEBUG">DEBUG</option>
                </select>
              </label>
            </div>
          </div>

          <div className="controls-row">
            <div className="search-box">
              <span className="search-icon">🔍</span>
              <input
                type="text"
                placeholder="Поиск в логах..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="search-input"
              />
              {searchQuery && (
                <button 
                  className="search-clear"
                  onClick={() => setSearchQuery('')}
                  title="Очистить поиск"
                >
                  ✕
                </button>
              )}
            </div>

            <div className="logs-info">
              Показано: <strong>{filteredLogs.length}</strong> из <strong>{count}</strong> записей
              {selectedLevel !== 'ALL' && ` (фильтр: ${selectedLevel})`}
              {searchQuery && ` (поиск: "${searchQuery}")`}
            </div>
          </div>
        </div>

        {/* Отображение логов */}
        <div className="logs-display">
          {filteredLogs.length === 0 ? (
            <div className="logs-empty">
              {logs.length === 0 ? (
                <>
                  <div className="empty-icon">📭</div>
                  <div>Логи отсутствуют</div>
                </>
              ) : (
                <>
                  <div className="empty-icon">🔍</div>
                  <div>Логи не найдены</div>
                  <div className="empty-hint">
                    Попробуйте изменить фильтры или поисковый запрос
                  </div>
                </>
              )}
            </div>
          ) : (
            <div className="logs-content">
              {filteredLogs.map((log, index) => (
                <div 
                  key={index} 
                  className={`log-line log-line-${getLogLevel(log).toLowerCase()}`}
                >
                  {formatLogLine(log)}
                </div>
              ))}
              <div ref={logsEndRef} />
            </div>
          )}
        </div>
      </div>

      <style>{`
        .logs-container {
          padding: 1rem;
        }

        .loading {
          text-align: center;
          padding: 2rem;
          color: #666;
        }

        .error-message {
          text-align: center;
          padding: 2rem;
          color: #dc3545;
        }

        .error-icon {
          font-size: 3rem;
          margin-bottom: 1rem;
        }

        .error-details {
          margin-top: 1rem;
          font-size: 0.9rem;
          color: #666;
          font-family: monospace;
        }

        .stats-grid {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
          gap: 1rem;
          margin-bottom: 1.5rem;
        }

        .stat-card {
          background: white;
          border-radius: 8px;
          padding: 1.25rem;
          box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
          transition: all 0.3s ease;
          border: 2px solid transparent;
        }

        .stat-card:hover {
          box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);
          transform: translateY(-2px);
        }

        .stat-card-active {
          border-color: #007bff;
          background: #f0f8ff;
        }

        .stat-header {
          display: flex;
          align-items: center;
          gap: 0.5rem;
          margin-bottom: 0.75rem;
        }

        .stat-icon {
          font-size: 1.5rem;
        }

        .stat-label {
          font-weight: 600;
          color: #333;
          font-size: 0.9rem;
        }

        .stat-value {
          font-size: 2rem;
          font-weight: bold;
          color: #007bff;
          margin-bottom: 0.5rem;
        }

        .stat-bar {
          height: 6px;
          background: #e9ecef;
          border-radius: 3px;
          overflow: hidden;
          margin-bottom: 0.5rem;
        }

        .stat-bar-fill {
          height: 100%;
          transition: width 0.3s ease;
          border-radius: 3px;
        }

        .stat-percentage {
          font-size: 0.85rem;
          color: #666;
          text-align: right;
        }

        .controls-panel {
          display: flex;
          flex-direction: column;
          gap: 1rem;
          margin-bottom: 1rem;
        }

        .controls-row {
          display: flex;
          justify-content: space-between;
          align-items: center;
          flex-wrap: wrap;
          gap: 1rem;
        }

        .control-group {
          display: flex;
          align-items: center;
        }

        .control-group label {
          display: flex;
          align-items: center;
          gap: 0.5rem;
          font-size: 0.9rem;
          color: #333;
        }

        .control-select {
          padding: 0.5rem;
          border: 1px solid #ddd;
          border-radius: 4px;
          font-size: 0.9rem;
          background: white;
          cursor: pointer;
        }

        .control-select:focus {
          outline: none;
          border-color: #007bff;
        }

        .checkbox-label {
          cursor: pointer;
          user-select: none;
        }

        .checkbox-label input[type="checkbox"] {
          cursor: pointer;
        }

        .search-box {
          flex: 1;
          max-width: 500px;
          position: relative;
          display: flex;
          align-items: center;
        }

        .search-icon {
          position: absolute;
          left: 0.75rem;
          font-size: 1rem;
          pointer-events: none;
        }

        .search-input {
          width: 100%;
          padding: 0.6rem 2.5rem 0.6rem 2.5rem;
          border: 1px solid #ddd;
          border-radius: 4px;
          font-size: 0.9rem;
        }

        .search-input:focus {
          outline: none;
          border-color: #007bff;
          box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.1);
        }

        .search-clear {
          position: absolute;
          right: 0.5rem;
          background: #dc3545;
          color: white;
          border: none;
          border-radius: 50%;
          width: 24px;
          height: 24px;
          cursor: pointer;
          display: flex;
          align-items: center;
          justify-content: center;
          font-size: 0.8rem;
          transition: background 0.2s;
        }

        .search-clear:hover {
          background: #c82333;
        }

        .logs-info {
          font-size: 0.9rem;
          color: #666;
          white-space: nowrap;
        }

        .logs-info strong {
          color: #007bff;
        }

        .logs-display {
          background: #1e1e1e;
          border-radius: 8px;
          overflow: hidden;
          border: 1px solid #333;
        }

        .logs-content {
          max-height: 600px;
          overflow-y: auto;
          padding: 1rem;
          font-family: 'Courier New', monospace;
          font-size: 0.9rem;
          line-height: 1.6;
        }

        .logs-empty {
          padding: 3rem 2rem;
          text-align: center;
          color: #858585;
        }

        .empty-icon {
          font-size: 3rem;
          margin-bottom: 1rem;
        }

        .empty-hint {
          margin-top: 0.5rem;
          font-size: 0.85rem;
          color: #666;
        }

        .log-line {
          padding: 0.4rem 0.5rem;
          border-left: 3px solid transparent;
          margin-bottom: 0.25rem;
          border-radius: 3px;
          transition: background 0.2s;
        }

        .log-line:hover {
          background: #2a2a2a;
        }

        .log-line-info {
          border-left-color: #4ec9b0;
        }

        .log-line-warning {
          border-left-color: #dcdcaa;
        }

        .log-line-error {
          border-left-color: #f48771;
          background: rgba(244, 135, 113, 0.05);
        }

        .log-line-debug {
          border-left-color: #9cdcfe;
        }

        .log-timestamp {
          color: #858585;
          margin-right: 0.5rem;
        }

        .log-level {
          font-weight: bold;
          margin-right: 0.5rem;
          padding: 0.1rem 0.4rem;
          border-radius: 3px;
          font-size: 0.85rem;
        }

        .log-level-info {
          color: #4ec9b0;
          background: rgba(78, 201, 176, 0.15);
        }

        .log-level-warning {
          color: #dcdcaa;
          background: rgba(220, 220, 170, 0.15);
        }

        .log-level-error {
          color: #f48771;
          background: rgba(244, 135, 113, 0.15);
        }

        .log-level-debug {
          color: #9cdcfe;
          background: rgba(156, 220, 254, 0.15);
        }

        .log-message {
          color: #d4d4d4;
        }

        .log-message mark {
          background: #ffd700;
          color: #000;
          padding: 0.1rem 0.2rem;
          border-radius: 2px;
          font-weight: bold;
        }

        .logs-content::-webkit-scrollbar {
          width: 10px;
        }

        .logs-content::-webkit-scrollbar-track {
          background: #2a2a2a;
        }

        .logs-content::-webkit-scrollbar-thumb {
          background: #555;
          border-radius: 5px;
        }

        .logs-content::-webkit-scrollbar-thumb:hover {
          background: #666;
        }

        @media (max-width: 768px) {
          .stats-grid {
            grid-template-columns: repeat(2, 1fr);
          }

          .controls-row {
            flex-direction: column;
            align-items: stretch;
          }

          .search-box {
            max-width: 100%;
          }

          .logs-info {
            text-align: center;
          }
        }
      `}</style>
    </div>
  );
}
