# Инструкция по сборке и настройке myRPC

## 1. Сборка проекта

В директориях с исходным кодом `myRPC-client` и `myRPC-server` выполняем команды:

```bash
make
```
Это создаст исполняемые файлы `myRPC-client` и `myRPC-server` в текущей директории.

```bash
make deb
```
Это создаст deb-пакеты в директориях `deb/myrpc-client/` и `deb/myrpc-server/`.

---

## 2. Установка клиент и сервер

После сборки и создания deb-пакетов можно установить клиент и сервер с помощью команды `dpkg`:

```bash
sudo dpkg -i deb/myrpc-client_1.0-1_amd64.deb
sudo dpkg -i deb/myrpc-server_1.0-1_amd64.deb
sudo dpkg -i deb/libmysyslog_1.0-1_amd64.deb
```

---

## 3. Настройка сервера

Создаём конфигурационный файл `/etc/myRPC/config_files/myRPC.conf` для настройки параметров сервера:

```bash
sudo mkdir -p /etc/myRPC/config_files
echo -e "port=5555\nsocket_type=stream" | sudo tee /etc/myRPC/config_files/myRPC.conf
```

---

## 4. Создание файла пользователей

Создаём файл пользователей `/etc/myRPC/config_files/users.conf`, в котором указаны разрешённые пользователи:

```bash
echo "vboxuser" | sudo tee /etc/myRPC/config_files/users.conf
```

---

## 5. Запуск сервера

Запускаем сервер, который ожидает подключения от клиентов:

```bash
sudo myrpc-server
```

---

## 6. Подключение клиента к серверу

### 6.1 TCP-соединение

```bash
myrpc-client -h 127.0.0.1 -p 5555 -s -c "date"
```

### 6.2 UDP-соединение

```bash
myrpc-client -h 127.0.0.1 -p 5555 -d -c "date"
```
