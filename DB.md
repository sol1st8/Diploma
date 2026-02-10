# Упрощенная схема БД для биллинга VoIP

## Описание
Минималистичная система биллинга для расчета платы за использование узлов связи (транков) на основе тарифов и прайс-листов.

---

## Структура базы данных

### 1. **hub** - Хабы (узлы сети)

```sql
CREATE TABLE hub (
    id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL UNIQUE,
    location VARCHAR(100),
    is_active BOOLEAN DEFAULT TRUE
);
```

**Атрибуты:**
- `id` - идентификатор хаба
- `name` - название хаба (например, "Moscow-Hub")
- `location` - географическое расположение
- `is_active` - активен ли хаб

---

### 2. **server** - Серверы

```sql
CREATE TABLE server (
    id INT PRIMARY KEY,
    hub_id INT NOT NULL,
    name VARCHAR(100) NOT NULL,
    ip_address VARCHAR(15),
    is_active BOOLEAN DEFAULT TRUE,
    
    FOREIGN KEY (hub_id) REFERENCES hub(id)
);
```

**Атрибуты:**
- `id` - идентификатор сервера
- `hub_id` - ссылка на хаб (FK)
- `name` - название сервера
- `ip_address` - IP-адрес сервера
- `is_active` - активен ли сервер

---

### 3. **nas_ip** - IP-адреса NAS

```sql
CREATE TABLE nas_ip (
    id INT PRIMARY KEY,
    server_id INT NOT NULL,
    ip_address VARCHAR(15) NOT NULL UNIQUE,
    description VARCHAR(255),
    
    FOREIGN KEY (server_id) REFERENCES server(id)
);
```

**Атрибуты:**
- `id` - идентификатор записи
- `server_id` - ссылка на сервер (FK)
- `ip_address` - IP-адрес NAS
- `description` - описание

---

### 4. **trunk** - Транки (каналы связи)

```sql
CREATE TABLE trunk (
    id INT PRIMARY KEY,
    server_id INT NOT NULL,
    name VARCHAR(100) NOT NULL UNIQUE,
    capacity INT DEFAULT 100,
    cost_per_channel DECIMAL(10, 6) DEFAULT 0,
    
    FOREIGN KEY (server_id) REFERENCES server(id)
);
```

**Атрибуты:**
- `id` - идентификатор транка
- `server_id` - ссылка на сервер (FK)
- `name` - название транка
- `capacity` - емкость (количество каналов)
- `cost_per_channel` - стоимость аренды одного канала

**Описание:** Транк - это канал связи, за использование которого взимается плата.

---

### 5. **pricelist** - Прайс-листы

```sql
CREATE TABLE pricelist (
    id INT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    currency VARCHAR(3) DEFAULT 'RUB',
    rate_per_minute DECIMAL(10, 6) DEFAULT 0,
    is_active BOOLEAN DEFAULT TRUE
);
```

**Атрибуты:**
- `id` - идентификатор прайс-листа
- `name` - название прайс-листа
- `currency` - валюта (RUB, USD, EUR)
- `rate_per_minute` - базовая ставка за минуту
- `is_active` - активен ли прайс-лист

**Описание:** Прайс-лист определяет базовую стоимость звонков.

---

### 6. **tarif** - Тарифы

```sql
CREATE TABLE tarif (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    pricelist_id INT NOT NULL,
    markup_percent INT DEFAULT 0,
    free_minutes INT DEFAULT 0,
    
    FOREIGN KEY (pricelist_id) REFERENCES pricelist(id)
);
```

**Атрибуты:**
- `id` - идентификатор тарифа
- `name` - название тарифа
- `pricelist_id` - ссылка на прайс-лист (FK)
- `markup_percent` - наценка в процентах
- `free_minutes` - бесплатные минуты

**Описание:** Тариф применяет наценку к базовому прайс-листу и может включать бесплатные минуты.

---

### 7. **call_statistics** - Статистика звонков

```sql
CREATE TABLE call_statistics (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    call_id VARCHAR(100) NOT NULL,
    trunk_id INT NOT NULL,
    tarif_id INT NOT NULL,
    duration_seconds INT DEFAULT 0,
    cost DECIMAL(10, 6) DEFAULT 0,
    call_time TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (trunk_id) REFERENCES trunk(id),
    FOREIGN KEY (tarif_id) REFERENCES tarif(id),
);
```

**Атрибуты:**
- `id` - идентификатор записи
- `call_id` - идентификатор звонка
- `trunk_id` - транк, через который прошел звонок (FK)
- `tarif_id` - примененный тариф (FK)
- `duration_seconds` - длительность звонка в секундах
- `cost` - итоговая стоимость звонка
- `call_time` - время звонка

**Описание:** Основная таблица статистики, связывающая звонки с транками и тарифами для расчета платы.

---

## ER-диаграмма

```
┌─────────┐
│   hub   │
│         │
│ id (PK) │
│ name    │
└────┬────┘
     │ 1:N
     ▼
┌─────────┐
│ server  │
│         │
│ id (PK) │
│ hub_id  │◄────FK
└────┬────┘
     │ 1:N
     ├──────────────┐
     ▼              ▼
┌─────────┐    ┌─────────┐
│ nas_ip  │    │  trunk  │
│         │    │         │
│ id (PK) │    │ id (PK) │
│server_id│◄FK │server_id│◄────FK
└─────────┘    │ cost_per│
               │ channel │
               └────┬────┘
                    │ 1:N
                    │
                    ▼
               ┌──────────────┐
               │     call     │
               │  statistics  │
               │              │
               │ id (PK)      │
               │ trunk_id ────┼──FK
               │ tarif_id ────┼──FK
               │ duration     │
               │ cost         │
               └──────┬───────┘
                      │ N:1
                      ▼
               ┌─────────┐
               │  tarif  │
               │         │
               │ id (PK) │
               │pricelist│◄────FK
               │  _id    │
               │ markup  │
               └────┬────┘
                    │ N:1
                    ▼
               ┌──────────┐
               │pricelist │
               │          │
               │ id (PK)  │
               │ rate_per │
               │ minute   │
               └──────────┘
```

---

## Связи между таблицами

1. **hub → server** (1:N)
   - Один хаб содержит много серверов

2. **server → nas_ip** (1:N)
   - Один сервер имеет несколько IP-адресов

3. **server → trunk** (1:N)
   - Один сервер управляет несколькими транками

4. **trunk → call_statistics** (1:N)
   - Один транк обрабатывает много звонков
   - **Ключевая связь для расчета платы за транк**

5. **pricelist → tarif** (1:N)
   - Один прайс-лист используется в нескольких тарифах

6. **tarif → call_statistics** (1:N)
   - Один тариф применяется ко многим звонкам
   - **Ключевая связь для расчета стоимости звонка**

---

## Процесс расчета платы за звонок

### Формула расчета:
```
1. Базовая стоимость = (duration_seconds / 60) × pricelist.rate_per_minute
2. Наценка = базовая_стоимость × (tarif.markup_percent / 100)
3. Стоимость транка = trunk.cost_per_channel (фиксированная плата)
4. Итоговая стоимость = базовая_стоимость + наценка + стоимость_транка
```

### Алгоритм:
1. Получаем звонок с `trunk_id` и `tarif_id`
2. Из `tarif` получаем `pricelist_id` и `markup_percent`
3. Из `pricelist` получаем `rate_per_minute`
4. Из `trunk` получаем `cost_per_channel`
5. Рассчитываем стоимость по формуле
6. Сохраняем в `call_statistics`

---

## Примеры SQL-запросов

### 1. Расчет выручки по транкам
```sql
SELECT 
    t.name AS trunk_name,
    COUNT(cs.id) AS total_calls,
    SUM(cs.duration_seconds) / 60 AS total_minutes,
    SUM(cs.cost) AS total_revenue,
    t.cost_per_channel * COUNT(DISTINCT DATE(cs.call_time)) AS trunk_cost
FROM call_statistics cs
JOIN trunk t ON cs.trunk_id = t.id
WHERE cs.call_time >= DATE_SUB(NOW(), INTERVAL 30 DAY)
GROUP BY t.id, t.name, t.cost_per_channel
ORDER BY total_revenue DESC;
```

### 2. Анализ эффективности тарифов
```sql
SELECT 
    tf.name AS tarif_name,
    pl.name AS pricelist_name,
    COUNT(cs.id) AS call_count,
    AVG(cs.duration_seconds) AS avg_duration,
    SUM(cs.cost) AS total_revenue
FROM call_statistics cs
JOIN tarif tf ON cs.tarif_id = tf.id
JOIN pricelist pl ON tf.pricelist_id = pl.id
GROUP BY tf.id, tf.name, pl.name
ORDER BY total_revenue DESC;
```

### 3. Загрузка транков
```sql
SELECT 
    t.name AS trunk_name,
    t.capacity,
    COUNT(cs.id) AS calls_count,
    ROUND(COUNT(cs.id) * 100.0 / t.capacity, 2) AS load_percent
FROM trunk t
LEFT JOIN call_statistics cs ON t.id = cs.trunk_id 
    AND cs.call_time >= DATE_SUB(NOW(), INTERVAL 1 HOUR)
GROUP BY t.id, t.name, t.capacity
ORDER BY load_percent DESC;
```

### 4. Прибыль по хабам
```sql
SELECT 
    h.name AS hub_name,
    COUNT(cs.id) AS total_calls,
    SUM(cs.cost) AS total_revenue,
    SUM(t.cost_per_channel) AS trunk_costs,
    SUM(cs.cost) - SUM(t.cost_per_channel) AS profit
FROM call_statistics cs
JOIN trunk t ON cs.trunk_id = t.id
JOIN server s ON t.server_id = s.id
JOIN hub h ON s.hub_id = h.id
WHERE cs.call_time >= DATE_SUB(NOW(), INTERVAL 7 DAY)
GROUP BY h.id, h.name
ORDER BY profit DESC;
```

---

## Расчет стоимости звонка (пример)

**Дано:**
- Звонок длительностью 180 секунд (3 минуты)
- Транк: `MSK-Trunk-MTS` (cost_per_channel = 0.50)
- Тариф: `Business` (markup_percent = 20%)
- Прайс-лист: `Standard-RUB` (rate_per_minute = 1.50)

**Расчет:**
1. Базовая стоимость = (180 / 60) × 1.50 = 4.50 RUB
2. Наценка = 4.50 × 0.20 = 0.90 RUB
3. Стоимость транка = 0.50 RUB
4. **Итого: 4.50 + 0.90 + 0.50 = 5.90 RUB**
