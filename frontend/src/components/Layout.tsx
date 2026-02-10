import { Outlet, Link, useNavigate } from 'react-router-dom';
import { useEffect } from 'react';

export function Layout() {
  const navigate = useNavigate();

  useEffect(() => {
    const token = localStorage.getItem('accessToken');
    if (!token) {
      navigate('/login');
    }
  }, [navigate]);

  const handleLogout = () => {
    const refreshToken = localStorage.getItem('refreshToken');
    if (refreshToken) {
      // Можно вызвать API logout, но для простоты просто очищаем
      localStorage.clear();
      navigate('/login');
    }
  };

  return (
    <div>
      <nav className="nav">
        <Link to="/">Панель администратора</Link>
        <Link to="/pricelists">Прайс-листы</Link>
        <Link to="/tarifs">Тарифы</Link>
        <Link to="/tables">Системные справочники</Link>
        <Link to="/call-stats">Статистика звонков</Link>
        <Link to="/simulator">Симулятор</Link>
        <Link to="/config">Конфигурация</Link>
        <Link to="/logs">Логи</Link>
        <div style={{ marginLeft: 'auto' }}>
          <button onClick={handleLogout} className="secondary">
            Выход
          </button>
        </div>
      </nav>
      <div className="container">
        <Outlet />
      </div>
    </div>
  );
}
