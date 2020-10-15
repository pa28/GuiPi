//
// Created by richard on 2020-10-15.
//

#include "Settings.h"
#include <iostream>
#include <exception>

using namespace std;
using namespace soci;

guipi::Settings::Settings(const string &db_file_name)  : mDbFileName(db_file_name) {
#define X(name,type,default) name = default;
    SETTING_VALUES
#undef X
}

template<typename T>
optional<T> guipi::Settings::getDatabaseValue(session &sql, const string_view &name) {
    throw std::logic_error("Fetching Type not supported by Settings.");
}

template <>
optional<int> guipi::Settings::getDatabaseValue(session &sql, const string_view &name) {
    int count;
    int value;
    sql << "SELECT count(value) FROM settings_int WHERE name = \"" << name << '"', into(count);
    if (count) {
        sql << "SELECT value FROM settings_int WHERE name = \"" << name << '"', into(value);
        return value;
    }
    return nullopt;
}

template <>
optional<float> guipi::Settings::getDatabaseValue(session &sql, const string_view &name) {
    int count;
    double value;
    sql << "SELECT count(value) FROM settings_real WHERE name = \"" << name << '"', into(count);
    if (count) {
        sql << "SELECT value FROM settings_real WHERE name = \"" << name << '"', into(value);
        return (float)value;
    }
    return nullopt;
}

template <>
optional<string> guipi::Settings::getDatabaseValue(session &sql, const string_view &name) {
    int count;
    string value;
    sql << "SELECT count(value) FROM settings_string WHERE name = \"" << name << '"', into(count);
    if (count) {
        sql << "SELECT value FROM settings_string WHERE name = \"" << name << '"', into(value);
        return value;
    }
    return nullopt;
}

template<typename T>
void guipi::Settings::setDatabaseValue(session &sql, const string_view &name, T value) {
    sql << "INSERT OR REPLACE INTO settings_string (name,value) VALUES (\"" << name << "\",\"" << value << "\")";
}

template <>
void guipi::Settings::setDatabaseValue(session &sql, const string_view &name, int value) {
    sql << "INSERT OR REPLACE INTO settings_int (name,value) VALUES (\"" << name << "\"," << value << ')';
}

template <>
void guipi::Settings::setDatabaseValue(session &sql, const string_view &name, float value) {
    sql << "INSERT OR REPLACE INTO settings_real (name,value) VALUES (\"" << name << "\"," << (float)value << ')';
}

void guipi::Settings::initializeSettingsDatabase() {
    try {
        session sql(sqlite3, mDbFileName);
        sql << "CREATE TABLE IF NOT EXISTS settings_string ("
            << "name TEXT PRIMARY KEY,"
            << "value TEXT);";
        sql << "CREATE TABLE IF NOT EXISTS settings_int ("
            << "name TEXT PRIMARY KEY,"
            << "value INTEGER);";
        sql << "CREATE TABLE IF NOT EXISTS settings_real ("
            << "name TEXT PRIMARY KEY,"
            << "value REAL);";

        if (getDatabaseValue<std::string>(sql, "version"))
            readAllValues(sql);
        writeAllValues(sql);
#ifdef VERSION
        setDatabaseValue<std::string>(sql, std::string_view{"version"}, XSTR(VERSION));
#endif
    }
    catch (exception const &e) {
        std::cerr << e.what() << '\n';
    }
}

void guipi::Settings::writeAllValues(session &sql) {
#define X(name,type,default) setDatabaseValue<type>(sql, #name, name);
    SETTING_VALUES
#undef X
}

void guipi::Settings::readAllValues(session &sql) {
#define X(name,type,default) {auto value = getDatabaseValue<type>(sql, #name); if (value) name = value.value();}
    SETTING_VALUES
#undef X
}
