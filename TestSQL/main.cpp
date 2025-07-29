#pragma comment(lib, "sqlite3.lib")
#include <iostream>
#include <comdef.h>
#include <string>
#include <vector>
#include "sqlite3.h"


void checkSQLiteResult(int rc, sqlite3* db)
{
	if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW)
	{
		std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		exit(1);
	}
}


struct User
{
	std::string name;
	int age;
	std::string email;
};


void insertUsers(sqlite3* db, const std::vector<User>& users)
{
	char* errMsg = nullptr;
	int rc;

	rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
	checkSQLiteResult(rc, db);

	const char* insertSQL = "INSERT INTO Users (name, age, email) VALUES (?, ?, ?);";
	sqlite3_stmt* stmt;

	rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
	checkSQLiteResult(rc, db);

	for (const auto& user : users)
	{
		sqlite3_bind_text(stmt, 1, user.name.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 2, user.age);
		sqlite3_bind_text(stmt, 3, user.email.c_str(), -1, SQLITE_TRANSIENT);

		rc = sqlite3_step(stmt);
		checkSQLiteResult(rc, db);
		sqlite3_reset(stmt);	// 다음 사용자 삽입을 위해 초기화
	}
	sqlite3_finalize(stmt);

	rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
	checkSQLiteResult(rc, db);
}

void serchUsers(sqlite3* db, const std::string& keyword, int page = 1, int pageSize = 5)
{
	const char* searchSQL =
		"SELECT id, name, age, email FROM Users "
		"WHERE name LIKE ? OR email LIKE ?"
		"ORDER BY age DESC "
		"LIMIT ? OFFSET ?;";

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, searchSQL, -1, &stmt, nullptr);
	checkSQLiteResult(rc, db);

	std::string likeKeyword = "%" + keyword + "%";
	int offset = (page - 1) * pageSize;

	sqlite3_bind_text(stmt, 1, likeKeyword.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, likeKeyword.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 3, pageSize);
	sqlite3_bind_int(stmt, 4, offset);

	std::cout << "\n [검색 결과 - 페이지 " << page << "]\n";
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		int id = sqlite3_column_int(stmt, 0);
		std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		int age = sqlite3_column_int(stmt, 2);
		std::string email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

		std::cout << "ID: " << id << ", Name: " << name << ", Age: " << age << ", Email: " << email << std::endl;
	}

	sqlite3_finalize(stmt);

}

int main() {
	sqlite3* db;
	char* errMsg = nullptr;

	int rc = sqlite3_open_v2("testSQL.atc", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	checkSQLiteResult(rc, db);

	// 1. 테이블 생성
	const char* createSQL =
		"CREATE TABLE IF NOT EXISTS Users ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"name TEXT NOT NULL,"
		"age INTEGER NOT NULL,"
		"email TEXT UNIQUE NOT NULL);";
	
	rc = sqlite3_exec(db, createSQL, nullptr, nullptr, &errMsg);
	checkSQLiteResult(rc, db);

	// 2. 사용자 삽입
	std::vector<User> users = {
		{"Alica", 30, "alice@example.com"},
		{"Bob", 25, "bob@example.com"},
		{"Charlie", 35, "charlie@example.com"},
		{"Dave", 28, "dave@example.com"},
		{"Eve", 40, "eve@example.com"},
		{"Frank", 29, "frank@example.com"},
		{"Grace", 32, "grace@example.com"}
	};

	insertUsers(db, users);

	// 사용자 검색 (이름 / 이메일에 'a' 포함, 페이지 1)
	serchUsers(db, "a", 1, 3); // 3개씩 페이지

	// 4. 사용자 검색(페이지 2)
	serchUsers(db, "a", 2, 3);

	sqlite3_close(db);

	return 0;
}