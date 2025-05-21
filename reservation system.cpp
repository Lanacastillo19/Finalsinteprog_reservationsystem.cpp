#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <limits>
#include <sstream>
#include <regex>
#include <fstream>
#include <climits>
#include <algorithm>
using namespace std;

const string CURRENT_DATE = "2025-05-22";
const int CURRENT_HOUR = 22;
const int CURRENT_MINUTE = 19;

// -------- Helper Function for Case-Insensitive Handling --------
string toUpperCase(const string& str) {
    string upper = str;
    transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return upper;
}

// -------- Exception Handling --------
class ReservationException : public exception {
    string message;
public:
    ReservationException(const string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// -------- Reservation Struct --------
struct Reservation {
    string id;
    string customerName;
    string phoneNumber;
    int partySize;
    string date;
    string time;
    int tableNumber;

    Reservation(const string& id, const string& name, const string& phone, int size, const string& date, const string& time, int table)
        : id(toUpperCase(id)), customerName(name), phoneNumber(phone), partySize(size), date(date), time(time), tableNumber(table) {}
};

// -------- Validation Functions --------
bool validatePhoneNumber(const string& phone) {
    regex phoneRegex("\\d{3}-\\d{3}-\\d{4}");
    return regex_match(phone, phoneRegex);
}

bool validateDate(const string& date) {
    regex dateRegex("\\d{4}-\\d{2}-\\d{2}");
    if (!regex_match(date, dateRegex)) {
        return false;
    }
    int year, month, day;
    sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day);
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }
    string currentDate = CURRENT_DATE;
    if (date < currentDate) {
        return false;
    }
    return true;
}

bool validateTime(const string& time, const string& date) {
    regex timeRegex("\\d{2}:\\d{2}");
    if (!regex_match(time, timeRegex)) {
        return false;
    }
    int hour, minute;
    sscanf(time.c_str(), "%d:%d", &hour, &minute);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return false;
    }
    if (date == CURRENT_DATE) {
        if (hour < CURRENT_HOUR || (hour == CURRENT_HOUR && minute <= CURRENT_MINUTE)) {
            return false;
        }
    }
    return true;
}

bool validatePartySize(int size) {
    return size >= 1;
}

bool validateReservationId(const string& id) {
    string upperId = toUpperCase(id);
    regex idRegex("ID \\d+A");
    return regex_match(upperId, idRegex);
}

bool validateNumericInput(const string& input, int& result, int minVal, int maxVal) {
    if (input.empty() || !all_of(input.begin(), input.end(), ::isdigit)) {
        return false;
    }
    try {
        size_t pos;
        result = stoi(input, &pos);
        if (pos != input.length()) {
            return false;
        }
        if (result < minVal || result > maxVal) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// -------- Singleton Pattern --------
class ReservationManager {
private:
    vector<bool> tables;
    vector<Reservation> reservations;
    static unique_ptr<ReservationManager> instance;
    int nextReservationId;

    ReservationManager() : tables(10, true), nextReservationId(1) {
        loadReservations();
    }

    string getCurrentTimestamp() {
        ostringstream oss;
        oss << CURRENT_DATE << " " << (CURRENT_HOUR < 10 ? "0" : "") << CURRENT_HOUR << ":"
            << (CURRENT_MINUTE < 10 ? "0" : "") << CURRENT_MINUTE << ":00";
        return oss.str();
    }

    void writeLogToFile(const string& logEntry) {
        ofstream logFile("logs.txt", ios::app);
        if (logFile.is_open()) {
            logFile << logEntry << "\n\n";
            logFile.close();
        } else {
            throw ReservationException("Unable to open log file.");
        }
    }

    void saveReservations() {
        ofstream resFile("reservations.txt");
        if (!resFile.is_open()) {
            throw ReservationException("Unable to open reservations file for writing.");
        }
        for (const auto& res : reservations) {
            resFile << res.id << "|" << res.customerName << "|" << res.phoneNumber << "|"
                    << res.partySize << "|" << res.date << "|" << res.time << "|"
                    << res.tableNumber << "\n";
        }
        resFile.close();

        ofstream idFile("next_id.txt");
        if (!idFile.is_open()) {
            throw ReservationException("Unable to open next_id file for writing.");
        }
        idFile << nextReservationId << "\n";
        idFile.close();
    }

    void loadReservations() {
        ifstream resFile("reservations.txt");
        if (resFile.is_open()) {
            string line;
            while (getline(resFile, line)) {
                stringstream ss(line);
                string id, customerName, phoneNumber, date, time;
                int partySize, tableNumber;
                getline(ss, id, '|');
                getline(ss, customerName, '|');
                getline(ss, phoneNumber, '|');
                ss >> partySize;
                ss.ignore(1);
                getline(ss, date, '|');
                getline(ss, time, '|');
                ss >> tableNumber;

                if (tableNumber >= 0 && tableNumber < tables.size()) {
                    tables[tableNumber] = false;
                }

                reservations.emplace_back(id, customerName, phoneNumber, partySize, date, time, tableNumber);

                // Extract numeric part of ID (e.g., "1" from "ID 1A")
                if (validateReservationId(id)) {
                    string numStr = id.substr(3, id.length() - 4);
                    try {
                        int idNum = stoi(numStr);
                        nextReservationId = max(nextReservationId, idNum + 1);
                    } catch (...) {
                        // Skip invalid IDs
                    }
                }
            }
            resFile.close();
        }

        ifstream idFile("next_id.txt");
        if (idFile.is_open()) {
            int savedId;
            if (idFile >> savedId) {
                nextReservationId = max(nextReservationId, savedId);
            }
            idFile.close();
        }
    }

public:
    bool reservationIdExists(const string& id, const string& excludeId = "") {
        string upperId = toUpperCase(id);
        string upperExcludeId = toUpperCase(excludeId);
        for (const auto& res : reservations) {
            if (res.id == upperId && res.id != upperExcludeId) {
                return true;
            }
        }
        return false;
    }

    static ReservationManager& getInstance() {
        if (!instance)
            instance.reset(new ReservationManager());
        return *instance;
    }

    void logLogin(const string& role, const string& username, const string& password) {
        string timestamp = getCurrentTimestamp();
        ostringstream logEntry;
        logEntry << "Account Log: (" << timestamp << ", N/A) | User: " << username
                 << " | Password: " << password;
        writeLogToFile(logEntry.str());
    }

    void logReservationAction(const string& role, const string& username, const string& action, const string& details,
                              const string& id = "", const string& customerName = "", const string& phoneNumber = "",
                              int partySize = 0, const string& date = "", const string& time = "", int tableNumber = -1) {
        ostringstream logEntry;
        logEntry << "Reservation Log\n"
                 << "Action: " << action << " by " << role << ": " << username << "\n"
                 << "Details: " << details;
        if (!id.empty() || !customerName.empty() || !phoneNumber.empty() || partySize > 0 ||
            !date.empty() || !time.empty() || tableNumber >= 0) {
            logEntry << "\n"
                     << "ID: " << (id.empty() ? "N/A" : id) << " | "
                     << "Name: " << (customerName.empty() ? "N/A" : customerName) << " | "
                     << "Contact: " << (phoneNumber.empty() ? "N/A" : phoneNumber) << " | "
                     << "Party-Size: " << (partySize > 0 ? to_string(partySize) : "N/A") << " | "
                     << "Date: " << (date.empty() ? "N/A" : date) << " | "
                     << "Time: " << (time.empty() ? "N/A" : time) << " | "
                     << "Table: " << (tableNumber >= 0 ? to_string(tableNumber + 1) : "N/A");
        }
        writeLogToFile(logEntry.str());
    }

    void logError(const string& role, const string& username, const string& action, const string& errorMsg,
                  const string& id = "", const string& customerName = "", const string& phoneNumber = "",
                  int partySize = 0, const string& date = "", const string& time = "", int tableNumber = -1) {
        ostringstream logEntry;
        logEntry << "Reservation Error Log\n"
                 << "Action: " << action << " by " << role << ": " << username << "\n"
                 << "Error: " << errorMsg;
        if (!id.empty() || !customerName.empty() || !phoneNumber.empty() || partySize > 0 ||
            !date.empty() || !time.empty() || tableNumber >= 0) {
            logEntry << "\n"
                     << "ID: " << (id.empty() ? "N/A" : id) << " | "
                     << "Name: " << (customerName.empty() ? "N/A" : customerName) << " | "
                     << "Contact: " << (phoneNumber.empty() ? "N/A" : phoneNumber) << " | "
                     << "Party-Size: " << (partySize > 0 ? to_string(partySize) : "N/A") << " | "
                     << "Date: " << (date.empty() ? "N/A" : date) << " | "
                     << "Time: " << (time.empty() ? "N/A" : time) << " | "
                     << "Table: " << (tableNumber >= 0 ? to_string(tableNumber + 1) : "N/A");
        }
        writeLogToFile(logEntry.str());
    }

    void viewTableAvailability() {
        for (int i = 0; i < tables.size(); ++i) {
            cout << "Table " << i + 1 << " is " << (tables[i] ? "AVAILABLE" : "BOOKED") << endl;
        }
    }

    bool hasReservations(const string& customerName) {
        for (const auto& res : reservations) {
            if (res.customerName == customerName) {
                return true;
            }
        }
        return false;
    }

    vector<Reservation> getAllReservations() const {
        return reservations;
    }

    int reserveTable(const string& customerName, const string& phoneNumber,
                    int partySize, const string& date, const string& time, int tableNumber) {
        if (!validatePhoneNumber(phoneNumber)) {
            throw ReservationException("Invalid phone number format. Use XXX-XXX-XXXX.");
        }
        if (!validatePartySize(partySize)) {
            throw ReservationException("Party size must be at least 1.");
        }
        if (!validateDate(date)) {
            throw ReservationException("Invalid date format (use YYYY-MM-DD) or date is in the past.");
        }
        if (!validateTime(time, date)) {
            throw ReservationException("Invalid time format (use HH:MM) or time is in the past for today.");
        }
        if (tableNumber < 0 || tableNumber >= tables.size()) {
            throw ReservationException("Invalid table number. Must be between 1 and 10.");
        }
        if (!tables[tableNumber]) {
            throw ReservationException("Selected table is already booked.");
        }
        tables[tableNumber] = false;

        // Generate new reservation ID
        string reservationId = "ID " + to_string(nextReservationId) + "A";
        while (reservationIdExists(reservationId)) {
            nextReservationId++;
            reservationId = "ID " + to_string(nextReservationId) + "A";
        }
        nextReservationId++; // Increment for the next reservation

        reservations.emplace_back(reservationId, customerName, phoneNumber, partySize, date, time, tableNumber);
        saveReservations();
        logReservationAction("Customer", customerName, "Reserved table",
                            "#" + to_string(tableNumber + 1) + " for " + to_string(partySize) + " on " + date + " at " + time,
                            reservationId, customerName, phoneNumber, partySize, date, time, tableNumber);
        return tableNumber;
    }

    void cancelReservation(const string& reservationId, const string& customerName) {
        string upperId = toUpperCase(reservationId);
        if (!validateReservationId(upperId)) {
            throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
        }
        bool hasReservation = false;
        int tableIndex = -1;
        string phoneNumber, date, time;
        int partySize;
        for (const auto& res : reservations) {
            if (res.id == upperId) {
                hasReservation = true;
                tableIndex = res.tableNumber;
                phoneNumber = res.phoneNumber;
                partySize = res.partySize;
                date = res.date;
                time = res.time;
                break;
            }
        }
        if (!hasReservation) {
            throw ReservationException("No reservation to cancel.");
        }
        tables[tableIndex] = true;
        auto it = reservations.begin();
        while (it != reservations.end()) {
            if (it->id == upperId) {
                it = reservations.erase(it);
            } else {
                ++it;
            }
        }
        saveReservations();
        logReservationAction("Customer", customerName, "Cancelled reservation", "ID " + upperId,
                            upperId, customerName, phoneNumber, partySize, date, time, tableIndex);
    }

    void viewCustomerReservations(const string& customerName) {
        cout << "\n--- Your Reservations ---\n";
        bool hasReservations = false;
        for (const auto& res : reservations) {
            if (res.customerName == customerName) {
                cout << "ID: " << res.id << ", Name: " << res.customerName
                     << ", Contact: " << res.phoneNumber << ", Party Size: " << res.partySize
                     << ", Date: " << res.date << ", Time: " << res.time
                     << ", Table: " << res.tableNumber + 1 << endl;
                hasReservations = true;
            }
        }
        if (!hasReservations) {
            cout << "No reservation to view.\n";
        }
    }

    void updateReservation(const string& reservationId, const string& customerName,
                           const string& newId, const string& newName, const string& newPhone, int newPartySize,
                           const string& newDate, const string& newTime, int newTableIndex) {
        string upperId = toUpperCase(reservationId);
        string upperNewId = newId == "0" ? "0" : toUpperCase(newId);
        if (!validateReservationId(upperId)) {
            throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
        }
        bool hasReservation = false;
        for (const auto& res : reservations) {
            if (res.id == upperId) {
                hasReservation = true;
                break;
            }
        }
        if (!hasReservation) {
            throw ReservationException("No reservation to update.");
        }

        if (upperNewId != "0") {
            if (!validateReservationId(upperNewId)) {
                throw ReservationException("Invalid new reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
            }
            if (reservationIdExists(upperNewId, upperId)) {
                throw ReservationException("New reservation ID already exists. Choose a different ID.");
            }
        }
        if (newPhone != "0" && !validatePhoneNumber(newPhone)) {
            throw ReservationException("Invalid phone number format. Use XXX-XXX-XXXX.");
        }
        if (newPartySize != 0 && !validatePartySize(newPartySize)) {
            throw ReservationException("Party size must be at least 1.");
        }
        if (newDate != "0" && !validateDate(newDate)) {
            throw ReservationException("Invalid date format (use YYYY-MM-DD) or date is in the past.");
        }
        if (newTime != "0" && !validateTime(newTime, newDate != "0" ? newDate : CURRENT_DATE)) {
            throw ReservationException("Invalid time format (use HH:MM) or time is in the past for today.");
        }

        int oldTableIndex = -1;
        for (auto& res : reservations) {
            if (res.id == upperId) {
                oldTableIndex = res.tableNumber;
                break;
            }
        }
        if (newTableIndex != -1) {
            if (newTableIndex < 0 || newTableIndex >= tables.size()) {
                throw ReservationException("Invalid new table index.");
            }
            tables[oldTableIndex] = true;
            if (!tables[newTableIndex]) {
                tables[oldTableIndex] = false;
                throw ReservationException("Selected table is already booked.");
            }
            tables[newTableIndex] = false;
        } else {
            newTableIndex = oldTableIndex;
        }

        string finalId = upperId;
        string finalName = customerName;
        string finalPhone = "";
        int finalPartySize = 0;
        string finalDate = "";
        string finalTime = "";
        for (auto& res : reservations) {
            if (res.id == upperId) {
                finalPhone = res.phoneNumber;
                finalPartySize = res.partySize;
                finalDate = res.date;
                finalTime = res.time;
                if (upperNewId != "0") {
                    res.id = upperNewId;
                    finalId = upperNewId;
                }
                if (newName != "0") {
                    res.customerName = newName;
                    finalName = newName;
                }
                if (newPhone != "0") {
                    res.phoneNumber = newPhone;
                    finalPhone = newPhone;
                }
                if (newPartySize != 0) {
                    res.partySize = newPartySize;
                    finalPartySize = newPartySize;
                }
                if (newDate != "0") {
                    res.date = newDate;
                    finalDate = newDate;
                }
                if (newTime != "0") {
                    res.time = newTime;
                    finalTime = newTime;
                }
                res.tableNumber = newTableIndex;
                break;
            }
        }
        saveReservations();
        logReservationAction("Customer", customerName, "Updated reservation", "ID " + upperId,
                            finalId, finalName, finalPhone, finalPartySize, finalDate, finalTime, newTableIndex);
    }

    void viewLogs() {
        cout << "--- System Logs ---\n\n";
        ifstream logFile("logs.txt");
        if (logFile.is_open()) {
            string line;
            while (getline(logFile, line)) {
                cout << line << "\n";
            }
            logFile.close();
        } else {
            cout << "Unable to open log file.\n";
        }
    }
};

unique_ptr<ReservationManager> ReservationManager::instance = nullptr;

// -------- Abstraction + Polymorphism --------
class User {
protected:
    string username;
    string role;
public:
    User(const string& name, const string& r, const string& password) : username(name), role(r) {
        ReservationManager::getInstance().logLogin(role, name, password);
    }
    virtual bool showMenu() = 0;
    virtual ~User() = default;
};

// Account database
map<string, string> receptionistAccounts;
map<string, string> customerAccounts;

// -------- Helper Functions for Customer Accounts --------
void saveCustomerAccounts(const map<string, string>& accounts) {
    ofstream accountsFile("customer_accounts.txt");
    if (!accountsFile.is_open()) {
        cerr << "Error: Unable to open customer_accounts.txt for writing." << endl;
        return;
    }
    for (const auto& account : accounts) {
        accountsFile << account.first << "|" << account.second << "\n";
    }
    accountsFile.close();
}

void loadCustomerAccounts(map<string, string>& accounts) {
    ifstream accountsFile("customer_accounts.txt");
    if (accountsFile.is_open()) {
        string line;
        while (getline(accountsFile, line)) {
            stringstream ss(line);
            string username, password;
            getline(ss, username, '|');
            getline(ss, password);
            accounts[username] = password;
        }
        accountsFile.close();
    }
}

// -------- Inheritance for Roles --------
class Customer : public User {
public:
    Customer(bool isNewAccount) : User("", "Customer", "") {
        string name, password;
        if (isNewAccount) {
            bool usernameValid = false;
            while (!usernameValid) {
                cout << "Enter username: ";
                getline(cin, name);
                if (customerAccounts.count(name)) {
                    cout << "Account already exists. Please choose a different username.\n";
                    continue;
                }
                usernameValid = true;
            }
            cout << "Enter password: ";
            getline(cin, password);
            customerAccounts[name] = password;
            saveCustomerAccounts(customerAccounts);
            cout << "Customer account created.\n";
            ReservationManager::getInstance().logLogin("Customer", name, password);
            username = name;
        } else {
            bool credentialsValid = false;
            while (!credentialsValid) {
                cout << "Enter username: ";
                getline(cin, name);
                cout << "Enter password: ";
                getline(cin, password);
                if (customerAccounts.count(name) && customerAccounts[name] == password) {
                    credentialsValid = true;
                    ReservationManager::getInstance().logLogin("Customer", name, password);
                    username = name;
                } else {
                    cout << "Invalid credentials. Please try again.\n";
                }
            }
        }
    }

    bool showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            string input;
            int choice;
            cout << "\n[Customer Menu - " << username << "]\n";
            cout << "1. View My Reservations\n";
            cout << "2. View Availability\n";
            cout << "3. Reserve Table\n";
            cout << "4. Update Reservation\n";
            cout << "5. Cancel Reservation\n";
            cout << "6. Exit\nChoice: ";
            getline(cin, input);

            if (!validateNumericInput(input, choice, 1, 6)) {
                cout << "Invalid choice. Please enter a single number between 1 and 6.\n";
                continue;
            }

            switch (choice) {
                case 1:
                    ReservationManager::getInstance().viewCustomerReservations(username);
                    break;
                case 2:
                    ReservationManager::getInstance().viewTableAvailability();
                    break;
                case 3: {
                    string phoneNumber, date, time, partySizeInput, tableInput;
                    int partySize, tableNumber;

                    while (true) {
                        cout << "Enter your phone number (e.g., 123-456-7890): ";
                        getline(cin, phoneNumber);
                        if (validatePhoneNumber(phoneNumber)) {
                            break;
                        }
                        cout << "Error: Invalid phone number format. Use XXX-XXX-XXXX.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                  "Invalid phone number format.", "", username, phoneNumber);
                    }

                    while (true) {
                        cout << "Enter party size (must be at least 1): ";
                        getline(cin, partySizeInput);
                        if (!validateNumericInput(partySizeInput, partySize, 1, INT_MAX)) {
                            cout << "Error: Invalid party size. Must be a single number >= 1 (e.g., 2, not 2a, 2.1, or 2 2).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                      "Invalid party size.", "", username, phoneNumber);
                            continue;
                        }
                        if (!validatePartySize(partySize)) {
                            cout << "Error: Party size must be at least 1.\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                      "Party size must be at least 1.", "", username, phoneNumber, partySize);
                            continue;
                        }
                        break;
                    }

                    while (true) {
                        cout << "Enter reservation date (e.g., YYYY-MM-DD, must be on or after " << CURRENT_DATE << "): ";
                        getline(cin, date);
                        if (validateDate(date)) {
                            break;
                        }
                        cout << "Error: Invalid date format (use YYYY-MM-DD) or date is in the past.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                 "Invalid date format or date is in the past.",
                                                                 "", username, phoneNumber, partySize, date);
                    }

                    while (true) {
                        cout << "Enter reservation time (e.g., HH:MM in 24-hour format, must be after "
                             << (CURRENT_HOUR < 10 ? "0" : "") << CURRENT_HOUR << ":"
                             << (CURRENT_MINUTE < 10 ? "0" : "") << CURRENT_MINUTE << " if today): ";
                        getline(cin, time);
                        if (validateTime(time, date)) {
                            break;
                        }
                        cout << "Error: Invalid time format (use HH:MM) or time is in the past for today.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                 "Invalid time format or time is in the past.",
                                                                 "", username, phoneNumber, partySize, date, time);
                    }

                    bool reservationComplete = false;
                    while (!reservationComplete) {
                        cout << "Available tables:\n";
                        ReservationManager::getInstance().viewTableAvailability();
                        cout << "Enter table number to reserve (1-10, or 0 to cancel): ";
                        getline(cin, tableInput);

                        if (tableInput == "0") {
                            cout << "Reservation cancelled.\n";
                            reservationComplete = true;
                            break;
                        }

                        if (!validateNumericInput(tableInput, tableNumber, 1, 10)) {
                            cout << "Error: Invalid table number. Must be a single number between 1 and 10 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                     "Invalid table number.",
                                                                     "", username, phoneNumber, partySize, date, time);
                            continue;
                        }
                        tableNumber--;

                        try {
                            int table = ReservationManager::getInstance().reserveTable(username, phoneNumber, partySize, date, time, tableNumber);
                            cout << "Reserved Table #" << table + 1 << " successfully!\n";
                            reservationComplete = true;
                        } catch (const ReservationException& ex) {
                            if (string(ex.what()) == "Selected table is already booked.") {
                                cout << "Error: Selected table is already booked. Please choose a different table.\n";
                                ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                         ex.what(), "", username, phoneNumber, partySize, date, time, tableNumber);
                            } else {
                                cout << "Error: " << ex.what() << endl;
                                ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table",
                                                                         ex.what(), "", username, phoneNumber, partySize, date, time, tableNumber);
                                cout << "Reservation failed. Returning to menu.\n";
                                reservationComplete = true;
                            }
                        }
                    }
                    break;
                }
                case 4: {
                    if (!ReservationManager::getInstance().hasReservations(username)) {
                        cout << "No reservations.\n";
                        break;
                    }

                    string reservationId, newId = "0", newName, newPhone, newDate, newTime, newPartySizeInput, newTableChoiceInput;
                    int newPartySize = 0, newTableChoice = 0, newTableIndex = -1;

                    while (true) {
                        cout << "Enter reservation ID to update (e.g., ID 1A): ";
                        getline(cin, reservationId);
                        reservationId = toUpperCase(reservationId);
                        try {
                            if (!validateReservationId(reservationId)) {
                                throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
                            }
                            bool hasReservation = false;
                            vector<Reservation> allRes = ReservationManager::getInstance().getAllReservations();
                            for (const auto& res : allRes) {
                                if (res.id == reservationId && res.customerName == username) {
                                    hasReservation = true;
                                    break;
                                }
                            }
                            if (!hasReservation) {
                                throw ReservationException("No reservation to update.");
                            }
                            break;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                     ex.what(), reservationId, username);
                        }
                    }

                    while (true) {
                        cout << "Enter new name (or 0 to keep current): ";
                        getline(cin, newName);
                        break;
                    }

                    while (true) {
                        cout << "Enter new phone number (e.g., 123-456-7890, or 0 to keep current): ";
                        getline(cin, newPhone);
                        if (newPhone == "0") break;
                        if (validatePhoneNumber(newPhone)) break;
                        cout << "Error: Invalid phone number format. Use XXX-XXX-XXXX.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                 "Invalid phone number format.", reservationId, username, newPhone);
                    }

                    while (true) {
                        cout << "Enter new party size (must be at least 1, or 0 to keep current): ";
                        getline(cin, newPartySizeInput);
                        if (newPartySizeInput == "0") {
                            newPartySize = 0;
                            break;
                        }
                        if (!validateNumericInput(newPartySizeInput, newPartySize, 1, INT_MAX)) {
                            cout << "Error: Invalid party size. Must be a single number >= 1 (e.g., 2, not 2a, 2.1, or 2 2).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                     "Invalid party size.", reservationId, username, newPhone, newPartySize);
                            continue;
                        }
                        if (!validatePartySize(newPartySize)) {
                            cout << "Error: Party size must be at least 1.\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                     "Party size must be at least 1.", reservationId, username, newPhone, newPartySize);
                            continue;
                        }
                        break;
                    }

                    while (true) {
                        cout << "Enter new date (e.g., YYYY-MM-DD, must be on or after " << CURRENT_DATE << ", or 0 to keep current): ";
                        getline(cin, newDate);
                        if (newDate == "0") break;
                        if (validateDate(newDate)) break;
                        cout << "Error: Invalid date format (use YYYY-MM-DD) or date is in the past.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                 "Invalid date format or date is in the past.",
                                                                 reservationId, username, newPhone, newPartySize, newDate);
                    }

                    while (true) {
                        cout << "Enter new time (e.g., HH:MM in 24-hour format, must be after "
                             << (CURRENT_HOUR < 10 ? "0" : "") << CURRENT_HOUR << ":"
                             << (CURRENT_MINUTE < 10 ? "0" : "") << CURRENT_MINUTE << " if today, or 0 to keep current): ";
                        getline(cin, newTime);
                        if (newTime == "0") break;
                        if (validateTime(newTime, newDate != "0" ? newDate : CURRENT_DATE)) break;
                        cout << "Error: Invalid time format (use HH:MM) or time is in the past for today.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                 "Invalid time format or time is in the past.",
                                                                 reservationId, username, newPhone, newPartySize, newDate, newTime);
                    }

                    while (true) {
                        cout << "Table options: 0 to keep current, or enter a specific table number (1-10):\n";
                        ReservationManager::getInstance().viewTableAvailability();
                        cout << "Choice: ";
                        getline(cin, newTableChoiceInput);
                        if (!validateNumericInput(newTableChoiceInput, newTableChoice, 0, 10)) {
                            cout << "Error: Invalid table choice. Must be a single number between 0 and 10 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                     "Invalid table choice.",
                                                                     reservationId, username, newPhone, newPartySize, newDate, newTime);
                            continue;
                        }
                        break;
                    }

                    string confirm;
                    cout << "Confirm update? (Y/N or Yes/No): ";
                    getline(cin, confirm);
                    if (confirm != "Yes" && confirm != "yes" && confirm != "Y" && confirm != "y") {
                        cout << "Update cancelled.\n";
                        break;
                    }

                    try {
                        if (newTableChoice != 0) {
                            newTableIndex = newTableChoice - 1;
                        }
                        ReservationManager::getInstance().updateReservation(reservationId, username,
                                                                            newId, newName, newPhone, newPartySize,
                                                                            newDate, newTime, newTableIndex);
                        cout << "Reservation updated successfully.\n";
                    } catch (const ReservationException& ex) {
                        cout << "Error: " << ex.what() << endl;
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation",
                                                                 ex.what(), reservationId, username, newPhone, newPartySize, newDate, newTime, newTableIndex);
                        cout << "Update failed. Returning to menu.\n";
                    }
                    break;
                }
                case 5: {
                    if (!ReservationManager::getInstance().hasReservations(username)) {
                        cout << "No reservations.\n";
                        break;
                    }

                    bool processComplete = false;
                    string reservationId;
                    while (!processComplete) {
                        try {
                            cout << "Enter reservation ID to cancel (e.g., ID 1A): ";
                            getline(cin, reservationId);
                            reservationId = toUpperCase(reservationId);

                            ReservationManager::getInstance().viewCustomerReservations(username);

                            string confirm;
                            cout << "Confirm cancellation? (Y/N or Yes/No): ";
                            getline(cin, confirm);
                            if (confirm != "Yes" && confirm != "yes" && confirm != "Y" && confirm != "y") {
                                cout << "Cancellation aborted.\n";
                                processComplete = true;
                                break;
                            }

                            ReservationManager::getInstance().cancelReservation(reservationId, username);
                            cout << "Reservation cancelled.\n";
                            processComplete = true;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Customer", username, "Failed to cancel reservation",
                                                                     ex.what(), reservationId, username);
                            cout << "Please try again.\n";
                        }
                    }
                    break;
                }
                case 6: {
                    string logout;
                    cout << "Logout? (Y/N or Yes/No): ";
                    getline(cin, logout);
                    if (logout == "Yes" || logout == "yes" || logout == "Y" || logout == "y") {
                        return true;
                    }
                    break;
                }
            }
        }
        return false;
    }
};

class Receptionist : public User {
public:
    bool isValidCredential(const string& input) const {
        return !input.empty() && all_of(input.begin(), input.end(), [](char c) {
            return isalnum(c);
        });
    }

public:
    Receptionist(const string& name, const string& password) : User(name, "Receptionist", password) {}
    bool showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            string input;
            int choice;
            cout << "\n[Receptionist Menu - " << username << "]\n";
            cout << "1. View Reservations\n2. View Table Availability\n3. Exit\nChoice: ";
            getline(cin, input);

            if (!validateNumericInput(input, choice, 1, 3)) {
                cout << "Invalid choice. Please enter a single number between 1 and 3.\n";
                continue;
            }

            switch (choice) {
                case 1: {
                    cout << "\n--- Current Reservations ---\n";
                    vector<Reservation> allReservations = ReservationManager::getInstance().getAllReservations();
                   
                    if (allReservations.empty()) {
                        cout << "No reservations found.\n";
                    } else {
                        cout << "ID\t\tCustomer\tParty\tDate\t\tTime\tContact\t\tTable\n";
                        for (const auto& res : allReservations) {
                            cout << res.id << "\t"
                                 << res.customerName << "\t"
                                 << res.partySize << "\t"
                                 << res.date << "\t"
                                 << res.time << "\t"
                                 << res.phoneNumber << "\t"
                                 << (res.tableNumber + 1) << endl;
                        }
                    }
                    break;
                }
                case 2:
                    ReservationManager::getInstance().viewTableAvailability();
                    break;
                case 3: {
                    string logout;
                    cout << "Logout? (Y/N or Yes/No): ";
                    getline(cin, logout);
                    if (logout == "Yes" || logout == "yes" || logout == "Y" || logout == "y") {
                        return true;
                    }
                    break;
                }
            }
        }
        return false;
    }
};

class Admin : public User {
public:
    Admin(const string& name, const string& password) : User(name, "Admin", password) {}
    bool showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            string input;
            int choice;
            cout << "\n[Admin Menu - " << username << "]\n";
            cout << "1. View Logs\n";
            cout << "2. View Customer Reservations\n";
            cout << "3. View Table Availability\n";
            cout << "4. Update Reservation\n";
            cout << "5. Cancel Reservation\n";
            cout << "6. Create Receptionist Account\n";
            cout << "7. Log Out\nChoice: ";
            getline(cin, input);

            if (!validateNumericInput(input, choice, 1, 7)) {
                cout << "Invalid choice. Please enter a single number between 1 and 7.\n";
                continue;
            }

            switch (choice) {
                case 1:
                    ReservationManager::getInstance().viewLogs();
                    break;
                case 2: {
                    cout << "\n--- Current Reservations ---\n";
                    vector<Reservation> allReservations = ReservationManager::getInstance().getAllReservations();
                   
                    if (allReservations.empty()) {
                        cout << "No reservations found.\n";
                    } else {
                        cout << "ID\t\tCustomer\tParty\tDate\t\tTime\tContact\t\tTable\n";
                        for (const auto& res : allReservations) {
                            cout << res.id << "\t"
                                 << res.customerName << "\t"
                                 << res.partySize << "\t"
                                 << res.date << "\t"
                                 << res.time << "\t"
                                 << res.phoneNumber << "\t"
                                 << (res.tableNumber + 1) << endl;
                        }
                    }
                    break;
                }
                case 3:
                    ReservationManager::getInstance().viewTableAvailability();
                    break;
                case 4: {
                    vector<Reservation> allReservations = ReservationManager::getInstance().getAllReservations();
                    if (allReservations.empty()) {
                        cout << "No reservations.\n";
                        break;
                    }

                    string reservationId, newId, newName, newPhone, newDate, newTime, newPartySizeInput, newTableChoiceInput;
                    int newPartySize = 0, newTableChoice = 0, newTableIndex = -1;
                    string customerName;

                    while (true) {
                        cout << "Enter reservation ID to update (e.g., ID 1A): ";
                        getline(cin, reservationId);
                        reservationId = toUpperCase(reservationId);
                        try {
                            if (!validateReservationId(reservationId)) {
                                throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
                            }
                            bool hasReservation = false;
                            for (const auto& res : allReservations) {
                                if (res.id == reservationId) {
                                    hasReservation = true;
                                    customerName = res.customerName;
                                    break;
                                }
                            }
                            if (!hasReservation) {
                                throw ReservationException("Reservation ID not found.");
                            }
                            cout << "\n--- Reservation to Update ---\n";
                            cout << "ID\t\tCustomer\tParty\tDate\t\tTime\tContact\t\tTable\n";
                            for (const auto& res : allReservations) {
                                if (res.id == reservationId) {
                                    cout << res.id << "\t"
                                         << res.customerName << "\t"
                                         << res.partySize << "\t"
                                         << res.date << "\t"
                                         << res.time << "\t"
                                         << res.phoneNumber << "\t"
                                         << (res.tableNumber + 1) << endl;
                                    break;
                                }
                            }
                            break;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                     ex.what(), reservationId);
                        }
                    }

                    while (true) {
                        cout << "Enter new ID (e.g., ID 2A, or 0 to keep current): ";
                        getline(cin, newId);
                        newId = toUpperCase(newId);
                        if (newId == "0") break;
                        try {
                            if (!validateReservationId(newId)) {
                                throw ReservationException("Invalid new reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
                            }
                            if (ReservationManager::getInstance().reservationIdExists(newId, reservationId)) {
                                throw ReservationException("New reservation ID already exists. Choose a different ID.");
                            }
                            break;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                     ex.what(), reservationId);
                        }
                    }

                    while (true) {
                        cout << "Enter new name (or 0 to keep current): ";
                        getline(cin, newName);
                        break;
                    }

                    while (true) {
                        cout << "Enter new phone number (e.g., 123-456-7890, or 0 to keep current): ";
                        getline(cin, newPhone);
                        if (newPhone == "0") break;
                        if (validatePhoneNumber(newPhone)) break;
                        cout << "Error: Invalid phone number format. Use XXX-XXX-XXXX.\n";
                        ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                 "Invalid phone number format.", reservationId, newName, newPhone);
                    }

                    while (true) {
                        cout << "Enter new party size (must be at least 1, or 0 to keep current): ";
                        getline(cin, newPartySizeInput);
                        if (newPartySizeInput == "0") {
                            newPartySize = 0;
                            break;
                        }
                        if (!validateNumericInput(newPartySizeInput, newPartySize, 1, INT_MAX)) {
                            cout << "Error: Invalid party size. Must be a single number >= 1 (e.g., 2, not 2a, 2.1, or 2 2).\n";
                            ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                     "Invalid party size.", reservationId, newName, newPhone, newPartySize);
                            continue;
                        }
                        if (!validatePartySize(newPartySize)) {
                            cout << "Error: Party size must be at least 1.\n";
                            ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                     "Party size must be at least 1.", reservationId, newName, newPhone, newPartySize);
                            continue;
                        }
                        break;
                    }

                    while (true) {
                        cout << "Enter new date (e.g., YYYY-MM-DD, must be on or after " << CURRENT_DATE << ", or 0 to keep current): ";
                        getline(cin, newDate);
                        if (newDate == "0") break;
                        if (validateDate(newDate)) break;
                        cout << "Error: Invalid date format (use YYYY-MM-DD) or date is in the past.\n";
                        ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                 "Invalid date format or date is in the past.",
                                                                 reservationId, newName, newPhone, newPartySize, newDate);
                    }

                    while (true) {
                        cout << "Enter new time (e.g., HH:MM in 24-hour format, must be after "
                             << (CURRENT_HOUR < 10 ? "0" : "") << CURRENT_HOUR << ":"
                             << (CURRENT_MINUTE < 10 ? "0" : "") << CURRENT_MINUTE << ", or 0 to keep current): ";
                        getline(cin, newTime);
                        if (newTime == "0") break;
                        if (validateTime(newTime, newDate != "0" ? newDate : CURRENT_DATE)) break;
                        cout << "Error: Invalid time format (use HH:MM) or time is in the past for today.\n";
                        ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                 "Invalid time format or time is in the past.",
                                                                 reservationId, newName, newPhone, newPartySize, newDate, newTime);
                    }

                    while (true) {
                        cout << "Table options: 0 to keep current, or enter a specific table number (1-10):\n";
                        ReservationManager::getInstance().viewTableAvailability();
                        cout << "Choice: ";
                        getline(cin, newTableChoiceInput);
                        if (!validateNumericInput(newTableChoiceInput, newTableChoice, 0, 10)) {
                            cout << "Error: Invalid table choice. Must be a single number between 0 and 10 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                            ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                     "Invalid table choice.",
                                                                     reservationId, newName, newPhone, newPartySize, newDate, newTime);
                            continue;
                        }
                        break;
                    }

                    string confirm;
                    cout << "Confirm update? (Y/N or Yes/No): ";
                    getline(cin, confirm);
                    if (confirm != "Yes" && confirm != "yes" && confirm != "Y" && confirm != "y") {
                        cout << "Update cancelled.\n";
                        break;
                    }

                    try {
                        if (newTableChoice != 0) {
                            newTableIndex = newTableChoice - 1;
                        }
                        ReservationManager::getInstance().updateReservation(reservationId, customerName,
                                                                            newId, newName, newPhone, newPartySize,
                                                                            newDate, newTime, newTableIndex);
                        cout << "Reservation updated successfully.\n";
                        ReservationManager::getInstance().logReservationAction("Admin", username, "Updated reservation",
                                                                             "ID " + reservationId);
                    } catch (const ReservationException& ex) {
                        cout << "Error: " << ex.what() << endl;
                        ReservationManager::getInstance().logError("Admin", username, "Failed to update reservation",
                                                                 ex.what(), reservationId, newName, newPhone, newPartySize, newDate, newTime, newTableIndex);
                        cout << "Update failed. Returning to menu.\n";
                    }
                    break;
                }
                case 5: {
                    vector<Reservation> allReservations = ReservationManager::getInstance().getAllReservations();
                    if (allReservations.empty()) {
                        cout << "No reservations.\n";
                        break;
                    }

                    bool processComplete = false;
                    string reservationId;
                    while (!processComplete) {
                        try {
                            string customerName;
                            cout << "Enter reservation ID to cancel (e.g., ID 1A): ";
                            getline(cin, reservationId);
                            reservationId = toUpperCase(reservationId);

                            if (!validateReservationId(reservationId)) {
                                throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
                            }
                            bool found = false;
                            for (const auto& res : allReservations) {
                                if (res.id == reservationId) {
                                    customerName = res.customerName;
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                throw ReservationException("Reservation ID not found.");
                            }

                            cout << "\n--- Reservation to Cancel ---\n";
                            cout << "ID\t\tCustomer\tParty\tDate\t\tTime\tContact\t\tTable\n";
                            for (const auto& res : allReservations) {
                                if (res.id == reservationId) {
                                    cout << res.id << "\t"
                                         << res.customerName << "\t"
                                         << res.partySize << "\t"
                                         << res.date << "\t"
                                         << res.time << "\t"
                                         << res.phoneNumber << "\t"
                                         << (res.tableNumber + 1) << endl;
                                    break;
                                }
                            }

                            string confirm;
                            cout << "Confirm cancellation? (Y/N or Yes/No): ";
                            getline(cin, confirm);
                            if (confirm != "Yes" && confirm != "yes" && confirm != "Y" && confirm != "y") {
                                cout << "Cancellation aborted.\n";
                                processComplete = true;
                                break;
                            }

                            ReservationManager::getInstance().cancelReservation(reservationId, customerName);
                            cout << "Reservation cancelled.\n";
                            ReservationManager::getInstance().logReservationAction("Admin", username, "Cancelled reservation",
                                                                                 "ID " + reservationId);
                            processComplete = true;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Admin", username, "Failed to cancel reservation",
                                                                     ex.what(), reservationId);
                            cout << "Please try again.\n";
                        }
                    }
                    break;
                }
                case 6: {
                    string recUsername, recPassword;
                    while (true) {
                        cout << "Enter new receptionist username: ";
                        getline(cin, recUsername);
                        if (receptionistAccounts.count(recUsername)) {
                            cout << "Username already exists. Please choose a different username.\n";
                            continue;
                        }
                        break;
                    }
                    cout << "Enter password: ";
                    getline(cin, recPassword);
                    receptionistAccounts[recUsername] = recPassword;
                    cout << "Receptionist account created.\n";
                    ReservationManager::getInstance().logReservationAction("Admin", username, "Created receptionist account",
                                                                         "Username: " + recUsername);
                    break;
                }
                case 7: {
                    string logout;
                    cout << "Logout? (Y/N or Yes/No): ";
                    getline(cin, logout);
                    if (logout == "Yes" || logout == "yes" || logout == "Y" || logout == "y") {
                        return true;
                    }
                    break;
                }
            }
        }
        return false;
    }
};

// -------- Main Driver --------
int main() {
    const string adminUsername = "admin";
    const string adminPassword = "admin123";

    loadCustomerAccounts(customerAccounts);

    bool isRunning = true;
    while (isRunning) {
        string input;
        int roleChoice;
        cout << "\n[Role Selection]\n1. Receptionist\n2. Customer\n3. Admin\n4. Exit\nChoose role: ";
        getline(cin, input);

        if (!validateNumericInput(input, roleChoice, 1, 4)) {
            cout << "Invalid choice. Please enter a single number between 1 and 4.\n";
            continue;
        }

        unique_ptr<User> user;

        switch (roleChoice) {
            case 1: {
                bool credentialsValid = false;
                string username, password;
                while (!credentialsValid) {
                    cout << "Enter Receptionist username: ";
                    getline(cin, username);
                    Receptionist temp(username, "");
                    if (!temp.isValidCredential(username)) {
                        cout << "Invalid username. Use letters and numbers only (no spaces or special characters).\n";
                        continue;
                    }
                    cout << "Enter password: ";
                    getline(cin, password);
                    if (!temp.isValidCredential(password)) {
                        cout << "Invalid password. Use letters and numbers only (no spaces or special characters).\n";
                        continue;
                    }
                    if (receptionistAccounts.count(username) && receptionistAccounts[username] == password) {
                        user = unique_ptr<Receptionist>(new Receptionist(username, password));
                        credentialsValid = true;
                    } else {
                        cout << "Invalid receptionist credentials. Please try again.\n";
                    }
                }
                break;
            }
            case 2: {
                int custOption;
                string custInput;
                while (true) {
                    cout << "\n1. Create Customer Account\n2. Login to Customer Account\nChoice: ";
                    getline(cin, custInput);
                    if (validateNumericInput(custInput, custOption, 1, 2)) {
                        break;
                    }
                    cout << "Invalid choice. Please enter a single number between 1 and 2.\n";
                }

                if (custOption == 1) {
                    user = unique_ptr<Customer>(new Customer(true));
                } else if (custOption == 2) {
                    user = unique_ptr<Customer>(new Customer(false));
                }
                break;
            }
            case 3: {
                bool credentialsValid = false;
                string username, password;
                while (!credentialsValid) {
                    cout << "Enter Admin username: ";
                    getline(cin, username);
                    cout << "Enter Admin password: ";
                    getline(cin, password);
                    if (username == adminUsername && password == adminPassword) {
                        user = unique_ptr<Admin>(new Admin(username, password));
                        credentialsValid = true;
                    } else {
                        cout << "Invalid admin credentials. Please try again.\n";
                    }
                }
                break;
            }
            case 4:
                isRunning = false;
                continue;
        }

        if (user) {
            bool logout = user->showMenu();
            if (logout) {
                user.reset();
                continue;
            }
        }
    }

    return 0;
}
