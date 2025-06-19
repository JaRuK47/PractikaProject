#pragma once
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <windows.h>




void menu(sqlite3* db);
void admin_menu(sqlite3* db);
void register_user(sqlite3* db, bool money);
void input_user(sqlite3* db);
void create_status(sqlite3* db);
void login_user(sqlite3* db);
void user_menu(sqlite3* db, int user_id);
void show_balance(sqlite3* db, int user_id);
void deposit_balance(sqlite3* db, int user_id);
void increase_balance(sqlite3* db);
void transfer_to_user(sqlite3* db, int user_id);
void display_user_transactions(sqlite3* db, int user_id);




class TransactionManager {
private:
    sqlite3* db;

public:
    explicit TransactionManager(sqlite3* database) : db(database) {}

    bool create_table() {
        const char* sqlCreateTransactionsTable =
            "CREATE TABLE IF NOT EXISTS transactions ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_id INTEGER NOT NULL,"
            "type TEXT NOT NULL,"
            "amount FLOAT NOT NULL,"
            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "description TEXT,"
            "FOREIGN KEY(user_id) REFERENCES users(id));";

        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sqlCreateTransactionsTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "Ошибка создания таблицы транзакций: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

    bool add_transaction(int user_id, const std::string& type, double amount, const std::string& description = "") {
        const char* sql = "INSERT INTO transactions (user_id, type, amount, description) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Ошибка подготовки запроса транзакции: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, amount);
        sqlite3_bind_text(stmt, 4, description.c_str(), -1, SQLITE_STATIC);

        bool success = true;
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Ошибка вставки транзакции: " << sqlite3_errmsg(db) << std::endl;
            success = false;
        }

        sqlite3_finalize(stmt);
        return success;
    }

    void get_all_transactions() {
        const char* sqlSelectAllTransactions = "SELECT * FROM transactions;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, sqlSelectAllTransactions, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
            return;
        }

        std::cout << "\n=== Все транзакции ===\n";

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            int userId = sqlite3_column_int(stmt, 1);
            const unsigned char* type = sqlite3_column_text(stmt, 2);
            float amount = static_cast<float>(sqlite3_column_double(stmt, 3));
            const unsigned char* date = sqlite3_column_text(stmt, 4);

            std::cout << "ID: " << id << ", User ID: " << userId
                << ", Тип: " << type << ", Сумма: " << amount
                << ", Дата: " << date << std::endl;
        }

        if (rc != SQLITE_DONE) {
            std::cerr << "Ошибка чтения транзакций: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_finalize(stmt);
    }
};

class User {
private:
    std::string first_name;
    std::string last_name;
    std::string password;
    float cash;
    std::string status;

public:
    User(const std::string& fname, const std::string& lname, const std::string& pwd, float money, const std::string& stat = "")
        : first_name(fname), last_name(lname), password(pwd), cash(money), status(stat) {}

    static bool create_table(sqlite3* db) {
        const char* sqlCreateUsersTable =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "first_name TEXT NOT NULL,"
            "last_name TEXT NOT NULL,"
            "password TEXT NOT NULL,"
            "cash FLOAT,"
            "status TEXT DEFAULT NULL);";

        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sqlCreateUsersTable, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "Ошибка создания таблицы пользователей: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

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





void display_user_transactions(sqlite3* db, int user_id) {
    sqlite3_stmt* stmt;
    const char* sql =
        "SELECT t.type, t.amount, t.timestamp, t.description, u.first_name, u.last_name "
        "FROM transactions t "
        "JOIN users u ON t.user_id = u.id "
        "WHERE t.user_id = ? "
        "ORDER BY t.timestamp DESC";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    std::cout << "\n--- Ваши транзакции ---\n";
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        double amount = sqlite3_column_double(stmt, 1);
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

        std::cout << "Пользователь: " << first_name << " " << last_name
            << " | Тип: " << type
            << " | Сумма: " << amount
            << " | Дата: " << timestamp
            << " | Описание: " << (desc ? desc : "") << std::endl;
        found = true;
    }

    if (!found) {
        std::cout << "У вас пока нет транзакций.\n";
    }

    sqlite3_finalize(stmt);
    system("pause");
}

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
                    TransactionManager trx(db);
                    trx.add_transaction(user_id, "deposit", amount, "Пополнение через банкомат");
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
    std::string sender_first_name, sender_last_name;

    sqlite3_stmt* stmt_check_sender;
    const char* check_sender_sql = "SELECT cash, first_name, last_name FROM users WHERE id = ?;";
    if (sqlite3_prepare_v2(db, check_sender_sql, -1, &stmt_check_sender, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_sender, 1, user_id);
        if (sqlite3_step(stmt_check_sender) == SQLITE_ROW) {
            sender_balance = sqlite3_column_double(stmt_check_sender, 0);
            sender_first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt_check_sender, 1));
            sender_last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt_check_sender, 2));
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
    const char* find_recipient_sql = "SELECT id, cash, status FROM users WHERE first_name = ? AND last_name = ?;";
    if (sqlite3_prepare_v2(db, find_recipient_sql, -1, &stmt_recipient, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt_recipient, 1, recipient_first_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_recipient, 2, recipient_last_name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt_recipient) == SQLITE_ROW) {
            const unsigned char* status_text = sqlite3_column_text(stmt_recipient, 2);
            if (status_text != nullptr && std::string(reinterpret_cast<const char*>(status_text)) == "deleted") {
                std::cout << "Нельзя перевести деньги: пользователь помечен как удалённый.\n";
                sqlite3_finalize(stmt_recipient);
                system("pause");
                return;
            }

            if (status_text != nullptr && std::string(reinterpret_cast<const char*>(status_text)) == "banned") {
                std::cout << "Нельзя перевести деньги: пользователь помечен как заблокированный.\n";
                sqlite3_finalize(stmt_recipient);
                system("pause");
                return;
            }

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

    TransactionManager trx(db);
    trx.add_transaction(user_id, "transfer_out", amount,
        "Перевод пользователю " + recipient_first_name + " " + recipient_last_name);
    trx.add_transaction(recipient_id, "transfer_in", amount,
        "Получение перевода от " + sender_first_name + " " + sender_last_name);

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
        std::cout << "4 - Показать транзакции" << std::endl;
        std::cout << "5 - Выйти" << std::endl;
        std::cout << ">> ";
        std::cin >> choice;

        if (choice == "1") {
            show_balance(db, user_id);
        }
        else if (choice == "2") {
            deposit_balance(db, user_id);
        }
        else if (choice == "3") {
            const char* statusSQL = "SELECT status FROM users WHERE id = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, statusSQL, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, user_id);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    const unsigned char* status_text = sqlite3_column_text(stmt, 0);
                    std::string status = (status_text != nullptr) ? reinterpret_cast<const char*>(status_text) : "";

                    if (status == "deleted" || status == "credited") {
                        std::cout << "Вы не можете совершать переводы. У вас задолженость.\n";
                        system("pause");
                    }
                    else {
                        transfer_to_user(db, user_id);
                    }
                }
                sqlite3_finalize(stmt);
            }
        }
        else if (choice == "4") {
            display_user_transactions(db, user_id);
        }
        else if (choice != "5") {
            std::cout << "Неверный синтаксис, попробуйте еще раз." << std::endl;
            system("pause");
        }
    } while (choice != "5");
}

void login_user(sqlite3* db) {
    std::string first_name, last_name, password;

    std::cout << "Введите имя: ";
    std::cin >> first_name;

    std::cout << "Введите фамилию: ";
    std::cin >> last_name;

    std::cout << "Введите пароль: ";
    std::cin >> password;

    const char* loginSQL = "SELECT id, status FROM users WHERE first_name = ? AND last_name = ? AND password = ?;";
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
        const unsigned char* status_text = sqlite3_column_text(stmt, 1);

        if (status_text != nullptr && std::string(reinterpret_cast<const char*>(status_text)) == "deleted") {
            std::cout << "Вход невозможен: аккаунт помечен как удалённый.\n";
            return;
        }
        if (status_text != nullptr && std::string(reinterpret_cast<const char*>(status_text)) == "banned") {
            std::cout << "Вход невозможен: аккаунт помечен как заблокированный.\n";
            return;
        }
        else {
            std::cout << "Успешный вход! Добро пожаловать, " << first_name << "!" << std::endl;
            sqlite3_finalize(stmt);
            system("pause");
            user_menu(db, user_id);
            return;
        }
    }
    else {
        std::cout << "Неверные данные! Попробуйте ещё раз." << std::endl;
    }

    sqlite3_finalize(stmt);
    system("pause");
}

void increase_balance(sqlite3* db) {
    int user_id;
    double amount;

    std::cout << "Введите ID пользователя, которому хотите увеличить баланс: ";
    std::cin >> user_id;

    std::cout << "Введите сумму увеличения: ";
    std::cin >> amount;

    const char* checkSQL = "SELECT cash, status FROM users WHERE id = ?;";
    sqlite3_stmt* checkStmt;

    if (sqlite3_prepare_v2(db, checkSQL, -1, &checkStmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(checkStmt, 1, user_id);

    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
        double current_cash = sqlite3_column_double(checkStmt, 0);
        const unsigned char* status_text = sqlite3_column_text(checkStmt, 1);
        std::string status = (status_text != nullptr) ? reinterpret_cast<const char*>(status_text) : "";

        sqlite3_finalize(checkStmt);

        if (status == "deleted") {
            std::cout << "Невозможно изменить баланс — пользователь удалён.\n";
            system("pause");
            return;
        }

        if (status == "banned") {
            std::cout << "Невозможно изменить баланс — пользователь удалён.\n";
            system("pause");
            return;
        }

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
            TransactionManager trx(db);
            trx.add_transaction(user_id, "admin_increase", amount, "Увеличение баланса администратором");
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

void create_status(sqlite3* db) {
    int id;
    std::cout << "Введите ID пользователя, которому хотите добавить статус: ";
    std::cin >> id;

    std::string status;
    std::string choice;
    while (choice != "1" && choice != "2" && choice != "3" && choice != "4") {
        system("cls");
        std::cout << "----- Статус пользователя -----" << std::endl;
        std::cout << "1 - deleted" << std::endl;
        std::cout << "2 - banned" << std::endl;
        std::cout << "3 - credited" << std::endl;
        std::cout << "4 - Отмена" << std::endl;
        std::cout << ">> ";
        std::cin >> choice;

        if (choice == "1") {
            status = "deleted";
        }
        else if (choice == "2") {
            status = "banned";
        }
        else if (choice == "3") {
            status = "credited";
        }
        else if (choice == "4") {
            status = nullptr;
        }
        else if (choice == "5") {
            return;
        }
        else {
            std::cout << "Неверный синтаксис, попробуйте еще раз." << std::endl;
            system("pause");
        }
    }


    const char* updateStatusSQL = "UPDATE users SET status = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, updateStatusSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        std::cout << "Статус пользователя с ID " << id << " успешно обновлён на \"" << status << "\".\n";
    }
    else {
        std::cerr << "Ошибка при обновлении статуса: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

void input_user(sqlite3* db) {
    const char* selectSQL = "SELECT id, first_name, last_name, password, cash, status FROM users;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        std::cout << "\nСписок пользователей:\n";
        std::cout << "------------------------\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            std::string f_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::string l_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            std::string passw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            double user_cash = sqlite3_column_double(stmt, 4);

            const unsigned char* status_text = sqlite3_column_text(stmt, 5);
            std::string status = (status_text != nullptr) ? reinterpret_cast<const char*>(status_text) : "NULL";

            std::cout << "ID: " << id
                << " | Имя: " << f_name
                << " | Фамилия: " << l_name
                << " | Пароль: " << passw
                << " | Баланс: " << user_cash << " руб."
                << " | Статус: " << status << std::endl;
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
    TransactionManager tm(db);
    std::string y;
    do {
        system("cls");
        std::cout << "-----------------Меню администратора-----------------" << std::endl;
        std::cout << "0 - Создать пользователя" << std::endl;
        std::cout << "1 - Увеличение баланса" << std::endl;
        std::cout << "2 - Добавление статуса пользователю" << std::endl;
        std::cout << "3 - Вывод всех пользователей" << std::endl;
        std::cout << "4 - Просмотреть все транзакции" << std::endl;
        std::cout << "5 - Назад" << std::endl;
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
            create_status(db);
        }
        else if (y == "3") {
            input_user(db);
        }
        else if (y == "4") {
            tm.get_all_transactions();
        }
        else if (y != "5") {
            std::cout << "Неверный синтаксис, попробуйте еще раз." << std::endl;
        }

        system("pause");
        system("cls");
    } while (y != "5");
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
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    sqlite3* db;
    int rc = sqlite3_open("bankdb.db", &db);
    if (rc) {
        std::cerr << "Невозможно открыть БД: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    if (!User::create_table(db)) {
        std::cerr << "Ошибка инициализации таблицы пользователей\n";
        return 1;
    }

    TransactionManager tm(db);
    if (!tm.create_table()) {
        std::cerr << "Ошибка инициализации таблицы транзакций\n";
        return 1;
    }

    const char* alterSQL = "ALTER TABLE users ADD COLUMN status TEXT DEFAULT NULL;";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, alterSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        // Игнорируем ошибку, если колонка уже есть
        if (std::string(errMsg).find("duplicate column name") == std::string::npos) {
            std::cerr << "Ошибка при добавлении столбца status: " << errMsg << std::endl;
        }
        sqlite3_free(errMsg);
    }
    menu(db);

    sqlite3_close(db);
    return 0;
}
