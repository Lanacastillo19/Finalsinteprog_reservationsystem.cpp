#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <limits>
using namespace std;

// -------- Exception Handling --------
class ReservationException : public exception {
    string message;
public:
    ReservationException(const string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// -------- Strategy Pattern --------
class TableAssignmentStrategy {
public:
    virtual int assignTable(const vector<bool>& tables) = 0;
    virtual ~TableAssignmentStrategy() = default;
};

class AutoAssignTable : public TableAssignmentStrategy {
public:
    int assignTable(const vector<bool>& tables) override {
        for (int i = 0; i < tables.size(); ++i) {
            if (tables[i]) return i;
        }
        throw ReservationException("No available tables.");
    }
};

// -------- Singleton Pattern --------
class ReservationManager {
private:
    vector<bool> tables;
    vector<string> reservationLogs;
    static unique_ptr<ReservationManager> instance;
    ReservationManager() : tables(10, true) {} // 10 tables
public:
    static ReservationManager& getInstance() {
        if (!instance)
            instance.reset(new ReservationManager());
        return *instance;
    }

    void viewTableAvailability() {
        for (int i = 0; i < tables.size(); ++i) {
            cout << "Table " << i + 1 << " is " << (tables[i] ? "AVAILABLE" : "BOOKED") << endl;
        }
    }

    int reserveTable(shared_ptr<TableAssignmentStrategy> strategy, const string& customerName) {
        int tableIndex = strategy->assignTable(tables);
        tables[tableIndex] = false;
        reservationLogs.push_back("Reservation by " + customerName + " at Table " + to_string(tableIndex + 1));
        return tableIndex;
    }

    void cancelReservation(int tableIndex, const string& customerName) {
        if (tableIndex < 0 || tableIndex >= tables.size()) {
            throw ReservationException("Invalid table index.");
        }
        tables[tableIndex] = true;
        reservationLogs.push_back("Cancellation by " + customerName + " for Table " + to_string(tableIndex + 1));
    }

    void viewLogs() {
        cout << "\n--- Reservation Logs ---\n";
        for (const auto& log : reservationLogs) {
            cout << log << endl;
        }
    }
};

unique_ptr<ReservationManager> ReservationManager::instance = nullptr;

// -------- Abstraction + Polymorphism --------
class User {
protected:
    string username;
public:
    User(const string& name) : username(name) {}
    virtual void showMenu() = 0;
    virtual ~User() = default;
};

// Account database
map<string, string> receptionistAccounts;
map<string, string> customerAccounts;

// -------- Inheritance for Roles --------
class Customer : public User {
public:
    Customer(const string& name) : User(name) {}
    void showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            int choice;
            cout << "\n[Customer Menu - " << username << "]\n";
            cout << "1. Reserve Table\n2. View Availability\n3. Cancel Reservation\n4. Exit\nChoice: ";
            cin >> choice;

            try {
                switch (choice) {
                    case 1: {
                        auto strategy = make_shared<AutoAssignTable>();
                        int table = ReservationManager::getInstance().reserveTable(strategy, username);
                        cout << "Reserved Table #" << table + 1 << " successfully!\n";
                        break;
                    }
                    case 2:
                        ReservationManager::getInstance().viewTableAvailability();
                        break;
                    case 3: {
                        int tableIndex;
                        cout << "Enter table number to cancel: ";
                        cin >> tableIndex;
                        ReservationManager::getInstance().cancelReservation(tableIndex - 1, username);
                        cout << "Reservation cancelled.\n";
                        break;
                    }
                    case 4:
                        isRunning = false;
                        break;
                    default:
                        cout << "Invalid choice.\n";
                }
            } catch (const ReservationException& ex) {
                cout << "Error: " << ex.what() << endl;
            }
        }
    }
};

class Receptionist : public User {
public:
    Receptionist(const string& name) : User(name) {}
    void showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            int choice;
            cout << "\n[Receptionist Menu - " << username << "]\n";
            cout << "1. View Reservations\n2. View Table Availability\n3. Exit\nChoice: ";
            cin >> choice;

            switch (choice) {
                case 1:
                    ReservationManager::getInstance().viewLogs();