#include <iostream>
#include <sqlite3.h>
#include <string>
#include <windows.h>

int main()
{
    setlocale(LC_ALL, "rus");
    SetConsoleOutputCP(CP_UTF8);

    sqlite3* db;
    int exit = sqlite3_open("bankdb.db", &db);

    if (exit != SQLITE_OK) {
        std::cerr << "Ошибка при открытии базы данных: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }
    else {
        std::cout << "База данных успешно открыта/создана!" << std::endl;
    }

    const char* sqlCreateTable =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "first_name TEXT NOT NULL,"
        "last_name TEXT NOT NULL,"
        "password TEXT NOT NULL,"
        "cash FLOAT);";

    char* errMsg;
    int rc = sqlite3_exec(db, sqlCreateTable, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка создания таблицы: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        std::cout << "Таблица создана успешно." << std::endl;
    }

    std::string first_name, last_name, password;
    float cash;

    std::cout << "Введите имя пользователя: ";
    std::cin >> first_name;

    std::cout << "Введите фамилию пользователя: ";
    std::cin >> last_name;

    std::cout << "Введите пароль пользователя: ";
    std::cin >> password;

    std::cout << "Введите размер денежных средств (в рублях): ";
    std::cin >> cash;

    // Безопасная вставка через prepared statement
    const char* insertSQL = "INSERT INTO users (first_name, last_name, password, cash) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* insertStmt;

    rc = sqlite3_prepare_v2(db, insertSQL, -1, &insertStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса вставки: " << sqlite3_errmsg(db) << std::endl;
    }
    else {
        sqlite3_bind_text(insertStmt, 1, first_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertStmt, 2, last_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertStmt, 3, password.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(insertStmt, 4, cash);

        if (sqlite3_step(insertStmt) != SQLITE_DONE) {
            std::cerr << "Ошибка вставки данных: " << sqlite3_errmsg(db) << std::endl;
        }
        else {
            std::cout << "Пользователь успешно добавлен в базу данных." << std::endl;
        }
        sqlite3_finalize(insertStmt);
    }

    // Вывод всех пользователей
    const char* selectSQL = "SELECT id, first_name, last_name, password, cash FROM users;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        std::cout << "\nСписок пользователей:\n";
        std::cout << "------------------------\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char* f_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* l_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* passw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            double user_cash = sqlite3_column_double(stmt, 4);

            std::cout << "ID: " << id
                << " | Имя: " << (f_name ? f_name : "")
                << " | Фамилия: " << (l_name ? l_name : "")
                << " | Пароль: " << (passw ? passw : "")
                << " | Баланс: " << user_cash << " руб." << std::endl;
        }
        std::cout << "------------------------" << std::endl;
    }
    else {
        std::cerr << "Ошибка SELECT-запроса: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}
