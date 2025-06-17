#include <iostream>
#include <sqlite3.h>
#include <string>
#include <windows.h>

void menu(sqlite3* db);
void admin_menu(sqlite3* db);
void register_user(sqlite3* db);
void input_user(sqlite3* db);

void input_user(sqlite3* db) {
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
}

void register_user(sqlite3* db) {
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

    const char* insertSQL = "INSERT INTO users (first_name, last_name, password, cash) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* insertStmt;
    int rc = sqlite3_prepare_v2(db, insertSQL, -1, &insertStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса вставки: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

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
    system("pause");
    system("cls");
}

void admin_menu(sqlite3* db) {
    std::string y;
    do {
        system("cls");
        std::cout << "-----------------меню администратора-----------------" << std::endl;
        std::cout << "0 - создать пользователя" << std::endl;
        std::cout << "1 - увеличение баланса" << std::endl;
        std::cout << "2 - удаление пользователя" << std::endl;
        std::cout << "3 - вывод всех пользователей" << std::endl;
        std::cout << "4 - назад" << std::endl;
        std::cout << ">> ";
        std::cin >> y;

        if (y == "0") {
            register_user(db);
        }
        else if (y == "1") {
            std::cout << "Функция в разработке." << std::endl;
        }
        else if (y == "2") {
            std::cout << "Функция в разработке." << std::endl;
        }
        else if (y == "3") {
            input_user(db);
        }
        else if (y != "4") {
            std::cout << "Неверный синтаксис, попробуйте еще раз." << std::endl;
        }

        system("pause");
        system("cls");
    } while (y != "4");
}

void menu(sqlite3* db) {
    std::string y;
    do {
        system("cls");
        std::cout << "-----------------меню управления-----------------" << std::endl;
        std::cout << "0 - админское управление" << std::endl;
        std::cout << "1 - регистрация нового пользователя" << std::endl;
        std::cout << "2 - вход в аккаунт (в разработке)" << std::endl;
        std::cout << "3 - выход" << std::endl;
        std::cout << ">> ";
        std::cin >> y;

        if (y == "0") {
            admin_menu(db);
        }
        else if (y == "1") {
            register_user(db);
        }
        else if (y == "2") {
            std::cout << "Функция в разработке." << std::endl;
        }
        else if (y != "3") {
            std::cout << "Неверный ввод, попробуйте еще раз." << std::endl;
        }

        system("pause");
        system("cls");
    } while (y != "3");
}

int main() {
    setlocale(LC_ALL, "rus");
    SetConsoleOutputCP(CP_UTF8);

    sqlite3* db;
    int exit = sqlite3_open("bankdb.db", &db);

    if (exit != SQLITE_OK) {
        std::cerr << "Ошибка при открытии базы данных: " << sqlite3_errmsg(db) << std::endl;
        return -1;
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
        sqlite3_close(db);
        return -1;
    }

    menu(db);

    sqlite3_close(db);
    return 0;
}
