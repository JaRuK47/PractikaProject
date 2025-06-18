#pragma once
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <windows.h>

std::string utf8_to_cp1251(const char* utf8_str) {
    if (!utf8_str) return "";

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, nullptr, 0);
    if (wide_len == 0) return "";

    std::wstring wide_str(wide_len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, &wide_str[0], wide_len);

    int cp1251_len = WideCharToMultiByte(1251, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (cp1251_len == 0) return "";

    std::string cp1251_str(cp1251_len, 0);
    WideCharToMultiByte(1251, 0, wide_str.c_str(), -1, &cp1251_str[0], cp1251_len, nullptr, nullptr);

    return cp1251_str;
}

void menu(sqlite3* db);
void admin_menu(sqlite3* db);
void register_user(sqlite3* db, bool money);
void input_user(sqlite3* db);
void delete_user_by_id(sqlite3* db);
void login_user(sqlite3* db);
void user_menu(sqlite3* db, int user_id);

void show_balance(sqlite3* db, int user_id) {
    const char* sql = "SELECT cash FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double balance = sqlite3_column_double(stmt, 0);
            std::cout << "Ваш баланс: " << balance << " руб." << std::endl;
        }

        sqlite3_finalize(stmt);
    }
    else {
        std::cerr << "Ошибка запроса: " << sqlite3_errmsg(db) << std::endl;
    }

    system("pause");
}

void deposit_balance(sqlite3* db, int user_id) {
    double amount;
    std::string amount_str;

    while (true) {
        std::cout << "Введите сумму пополнения: ";
        std::cin >> amount_str;

        try {
            amount = std::stod(amount_str);
            if (amount <= 0) throw std::invalid_argument("negative");
            break;
        }
        catch (...) {
            std::cout << "Некорректное значение. Попробуйте снова.\n";
        }
    }

    const char* getBalanceSQL = "SELECT cash FROM users WHERE id = ?;";
    sqlite3_stmt* getStmt;
    if (sqlite3_prepare_v2(db, getBalanceSQL, -1, &getStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(getStmt, 1, user_id);

        if (sqlite3_step(getStmt) == SQLITE_ROW) {
            double current_cash = sqlite3_column_double(getStmt, 0);
            sqlite3_finalize(getStmt);

            double new_cash = current_cash + amount;

            const char* updateSQL = "UPDATE users SET cash = ? WHERE id = ?;";
            sqlite3_stmt* updateStmt;
            if (sqlite3_prepare_v2(db, updateSQL, -1, &updateStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_double(updateStmt, 1, new_cash);
                sqlite3_bind_int(updateStmt, 2, user_id);

                if (sqlite3_step(updateStmt) == SQLITE_DONE) {
                    std::cout << "Баланс успешно пополнен на " << amount << " руб." << std::endl;
                }
                else {
                    std::cerr << "Ошибка при обновлении баланса: " << sqlite3_errmsg(db) << std::endl;
                }
                sqlite3_finalize(updateStmt);
            }
        }
        else {
            std::cerr << "Ошибка при получении текущего баланса." << std::endl;
            sqlite3_finalize(getStmt);
        }
    }
    else {
        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
    }

    system("pause");
}

void transfer_to_user(sqlite3* db, int user_id) {
    std::string recipient_first_name, recipient_last_name;
    double amount;

    std::cout << "Введите имя получателя: ";
    std::cin >> recipient_first_name;
    std::cout << "Введите фамилию получателя: ";
    std::cin >> recipient_last_name;

    std::string amount_str;

    while (true) {
        std::cout << "Введите сумму перевода: ";
        std::cin >> amount_str;

        try {
            amount = std::stod(amount_str);
            if (amount <= 0) throw std::invalid_argument("negative");
            break;
        }
        catch (...) {
            std::cout << "Некорректная сумма. Попробуйте снова.\n";
        }
    }

    double sender_balance = 0.0;
    sqlite3_stmt* stmt_check_sender;
    const char* check_sender_sql = "SELECT cash FROM users WHERE id = ?;";

    if (sqlite3_prepare_v2(db, check_sender_sql, -1, &stmt_check_sender, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_sender, 1, user_id);
        if (sqlite3_step(stmt_check_sender) == SQLITE_ROW) {
            sender_balance = sqlite3_column_double(stmt_check_sender, 0);
        }
        sqlite3_finalize(stmt_check_sender);
    }

    if (sender_balance < amount) {
        std::cout << "Недостаточно средств.\n";
        system("pause");
        return;
    }

    int recipient_id = -1;
    double recipient_balance = 0.0;
    sqlite3_stmt* stmt_recipient;
    const char* find_recipient_sql = "SELECT id, cash FROM users WHERE first_name = ? AND last_name = ?;";

    if (sqlite3_prepare_v2(db, find_recipient_sql, -1, &stmt_recipient, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt_recipient, 1, recipient_first_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_recipient, 2, recipient_last_name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt_recipient) == SQLITE_ROW) {
            recipient_id = sqlite3_column_int(stmt_recipient, 0);
            recipient_balance = sqlite3_column_double(stmt_recipient, 1);
        }
        sqlite3_finalize(stmt_recipient);
    }

    if (recipient_id == -1) {
        std::cout << "Пользователь не найден.\n";
        system("pause");
        return;
    }

    sqlite3_stmt* update_sender;
    const char* update_sender_sql = "UPDATE users SET cash = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(db, update_sender_sql, -1, &update_sender, nullptr) == SQLITE_OK) {
        sqlite3_bind_double(update_sender, 1, sender_balance - amount);
        sqlite3_bind_int(update_sender, 2, user_id);
        sqlite3_step(update_sender);
        sqlite3_finalize(update_sender);
    }

    sqlite3_stmt* update_recipient;
    const char* update_recipient_sql = "UPDATE users SET cash = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(db, update_recipient_sql, -1, &update_recipient, nullptr) == SQLITE_OK) {
        sqlite3_bind_double(update_recipient, 1, recipient_balance + amount);
        sqlite3_bind_int(update_recipient, 2, recipient_id);
        sqlite3_step(update_recipient);
        sqlite3_finalize(update_recipient);
    }

    std::cout << "Перевод выполнен успешно.\n";
    system("pause");
}

void user_menu(sqlite3* db, int user_id) {
    std::string choice;

    do {
        system("cls");
        std::cout << "----- Меню пользователя -----" << std::endl;
        std::cout << "1 - Показать баланс" << std::endl;
        std::cout << "2 - Пополнение баланса через банкомат" << std::endl;
        std::cout << "3 - Перевод пользователю" << std::endl;
        std::cout << "4 - Выйти" << std::endl;
        std::cout << ">> ";
        std::cin >> choice;

        if (choice == "1") {
            show_balance(db, user_id);
        }
        else if (choice == "2") {
            deposit_balance(db, user_id);
        }
        else if (choice == "3") {
            transfer_to_user(db, user_id);
        }
        else if (choice != "4") {
            std::cout << "Неверный синтаксис, попробуйте еще раз." << std::endl;
            system("pause");
        }
    } while (choice != "4");
}


class User {
private:
    std::string first_name;
    std::string last_name;
    std::string password;
    float cash;

public:
    User(const std::string& fname, const std::string& lname, const std::string& pwd, float money)
        : first_name(fname), last_name(lname), password(pwd), cash(money) {}

    bool saveToDB(sqlite3* db) {
        const char* insertSQL = "INSERT INTO users (first_name, last_name, password, cash) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, first_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, last_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 4, cash);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Ошибка при вставке: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        return true;
    }
};

void login_user(sqlite3* db) {
    std::string first_name, last_name, password;

    std::cout << "Введите имя: ";
    std::cin >> first_name;

    std::cout << "Введите фамилию: ";
    std::cin >> last_name;

    std::cout << "Введите пароль: ";
    std::cin >> password;

    const char* loginSQL = "SELECT id FROM users WHERE first_name = ? AND last_name = ? AND password = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, loginSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса входа: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, first_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, last_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int user_id = sqlite3_column_int(stmt, 0);
        std::cout << "Успешный вход! Добро пожаловать, " << first_name << "!" << std::endl;
        sqlite3_finalize(stmt);
        system("pause");
        user_menu(db, user_id);
    }
    else {
        std::cout << "Неверные данные! Попробуйте ещё раз." << std::endl;
        sqlite3_finalize(stmt);
    }

    system("pause");
}

void increase_balance(sqlite3* db) {
    int user_id;
    double amount;

    std::cout << "Введите ID пользователя, которому хотите увеличить баланс: ";
    std::cin >> user_id;
  
    std::cout << "Введите сумму увеличения: ";
    std::cin >> amount;

    const char* checkSQL = "SELECT cash FROM users WHERE id = ?;";
    sqlite3_stmt* checkStmt;

    if (sqlite3_prepare_v2(db, checkSQL, -1, &checkStmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(checkStmt, 1, user_id);

    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
        double current_cash = sqlite3_column_double(checkStmt, 0);
        sqlite3_finalize(checkStmt);

        double new_cash = current_cash + amount;

        const char* updateSQL = "UPDATE users SET cash = ? WHERE id = ?;";
        sqlite3_stmt* updateStmt;

        if (sqlite3_prepare_v2(db, updateSQL, -1, &updateStmt, nullptr) != SQLITE_OK) {
            std::cerr << "Ошибка подготовки UPDATE-запроса: " << sqlite3_errmsg(db) << std::endl;
            return;
        }

        sqlite3_bind_double(updateStmt, 1, new_cash);
        sqlite3_bind_int(updateStmt, 2, user_id);

        if (sqlite3_step(updateStmt) == SQLITE_DONE) {
            std::cout << "Баланс успешно обновлён." << std::endl;
        }
        else {
            std::cerr << "Ошибка обновления баланса: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_finalize(updateStmt);
    }
    else {
        std::cerr << "Пользователь с таким ID не найден." << std::endl;
        sqlite3_finalize(checkStmt);
    }
}

void delete_user_by_id(sqlite3* db) {
    int id;
    std::cout << "Введите ID пользователя, которого хотите удалить: ";
    std::cin >> id;

    std::string deleteSQL = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt* deleteStmt;
    if (sqlite3_prepare_v2(db, deleteSQL.c_str(), -1, &deleteStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(deleteStmt, 1, id);
        if (sqlite3_step(deleteStmt) == SQLITE_DONE) {
            std::cout << "Пользователь с ID " << id << " успешно удалён.\n";
        }
        else {
            std::cerr << "Ошибка при удалении пользователя: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_finalize(deleteStmt);
    }
    else {
        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Пересоздание таблицы users с переиндексацией ID
    const char* reindexSQL =
        "BEGIN TRANSACTION; "
        "CREATE TABLE IF NOT EXISTS users_temp ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "first_name TEXT NOT NULL, "
        "last_name TEXT NOT NULL, "
        "password TEXT NOT NULL, "
        "cash FLOAT); "
        "INSERT INTO users_temp (first_name, last_name, password, cash) "
        "SELECT first_name, last_name, password, cash FROM users; "
        "DROP TABLE users; "
        "ALTER TABLE users_temp RENAME TO users; "
        "COMMIT;";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, reindexSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка при пересоздании таблицы: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        std::cout << "ID переиндексированы.\n";
    }
}

void input_user(sqlite3* db) {
    const char* selectSQL = "SELECT id, first_name, last_name, password, cash FROM users;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        std::cout << "\nСписок пользователей:\n";
        std::cout << "------------------------\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);

            std::string f_name = utf8_to_cp1251(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            std::string l_name = utf8_to_cp1251(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            std::string passw = utf8_to_cp1251(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));

            double user_cash = sqlite3_column_double(stmt, 4);

            std::cout << "ID: " << id
                << " | Имя: " << f_name
                << " | Фамилия: " << l_name
                << " | Пароль: " << passw
                << " | Баланс: " << user_cash << " руб." << std::endl;
        }
        std::cout << "------------------------" << std::endl;
        sqlite3_finalize(stmt);
    }
    else {
        std::cerr << "Ошибка SELECT-запроса: " << sqlite3_errmsg(db) << std::endl;
    }
}

void register_user(sqlite3* db, bool money) {
    std::string first_name, last_name, password;
    float cash;

    std::cout << "Введите имя пользователя: ";
    std::cin >> first_name;

    std::cout << "Введите фамилию пользователя: ";
    std::cin >> last_name;

    std::cout << "Введите пароль пользователя: ";
    std::cin >> password;

    if (money) {
        std::cout << "Введите размер денежных средств (в рублях): ";
        std::cin >> cash;
    }
    else
        cash = 0;

    User newUser(first_name, last_name, password, cash);

    if (newUser.saveToDB(db)) {
        std::cout << "Пользователь успешно добавлен." << std::endl;
    }
    else {
        std::cerr << "Ошибка при добавлении пользователя." << std::endl;
    }

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
            register_user(db, true);
        }
        else if (y == "1") {
            input_user(db);
            increase_balance(db);
        }
        else if (y == "2") {
            input_user(db);
            delete_user_by_id(db);
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
        std::cout << "2 - вход в аккаунт" << std::endl;
        std::cout << "3 - выход" << std::endl;
        std::cout << ">> ";
        std::cin >> y;

        if (y == "0") {
            std::cout << std::endl << "Введите пароль >> ";
            std::cin >> y;
            if (y=="123321456654")
                admin_menu(db);
            else
                std::cout << std::endl << "Нет прав";
        }
        else if (y == "1") {
            register_user(db, false);
        }
        else if (y == "2") {
            login_user(db);
        }
        else if (y != "3") {
            std::cout << "Неверный ввод, попробуйте еще раз." << std::endl;
        }

        system("pause");
        system("cls");
    } while (y != "3");
}

int main() {
    SetConsoleCP(1251);           // Ввод по-русски
    SetConsoleOutputCP(1251);     // Вывод по-русски
    setlocale(LC_ALL, "Russian");

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

    const char* sqlCreateTransactionsTable =
        "CREATE TABLE IF NOT EXISTS transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender_first_name TEXT NOT NULL,"
        "sender_last_name TEXT NOT NULL,"
        "recipient_first_name TEXT NOT NULL,"
        "recipient_last_name TEXT NOT NULL,"
        "amount FLOAT NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    rc = sqlite3_exec(db, sqlCreateTransactionsTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка создания таблицы transactions: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return -1;
    }

    menu(db);

    sqlite3_close(db);
    return 0;
}
