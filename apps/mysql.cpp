#include "os/bch.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <mariadb/conncpp.hpp>

void deleteTask(std::unique_ptr<sql::Connection>& conn, int id) {
  try {
    std::unique_ptr<sql::PreparedStatement> stmnt(
        conn->prepareStatement("delete from tasks where id = ?"));
    stmnt->setInt(1, id);
    stmnt->executeQuery();
    std::cout << "delete count: " << stmnt->getUpdateCount() << "\n";
  } catch (sql::SQLException& e) {
    std::cerr << "Error deleting task: " << e.what() << std::endl;
  }
}

void updateTaskStatus(std::unique_ptr<sql::Connection>& conn, int id, bool completed) {
  try {
    std::unique_ptr<sql::PreparedStatement> stmnt(
        conn->prepareStatement("update tasks set completed = ? where id = ?"));
    std::cout << std::boolalpha << "id: " << id << ", completed: " << completed << "\n";
    stmnt->setBoolean(1, completed);
    stmnt->setInt(2, id);
    stmnt->executeUpdate();
    std::cout << "update count: " << stmnt->getUpdateCount() << "\n";
  } catch (sql::SQLException& e) {
    std::cerr << "Error updating task status: " << e.what() << std::endl;
  }
}

std::uint64_t lastInsertId(std::unique_ptr<sql::Connection>& conn) {
  std::unique_ptr<sql::Statement> stmnt(conn->createStatement());

  std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery("select last_insert_id()"));
  std::uint64_t                   insert_id = 0;
  if (res->next()) {
    insert_id = res->getUInt64(1);
  }
  return insert_id;
}

void addTask(std::unique_ptr<sql::Connection>& conn, const std::string& description) {
  try {
    std::unique_ptr<sql::PreparedStatement> stmnt(
        conn->prepareStatement("insert into tasks (description) values (?)"));

    stmnt->setString(1, description);

    std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery());
    std::cout << "last insert id: " << lastInsertId(conn) << "\n";

  } catch (sql::SQLException& e) {
    std::cerr << "Error inserting new task: " << e.what() << std::endl;
  }
}

void showTasks(std::unique_ptr<sql::Connection>& conn) {
  try {
    std::unique_ptr<sql::Statement> stmnt(conn->createStatement());

    std::unique_ptr<sql::ResultSet> res(
        stmnt->executeQuery("select id, description, completed from tasks limit 10"));

    while (res->next()) {
      std::cout << std::boolalpha << "id = " << res->getInt("id")
                << ", description = " << res->getString("description")
                << ", completed = " << res->getBoolean("completed") << "\n";
    }
  } catch (sql::SQLException& e) {
    std::cerr << "Error selecting tasks: " << e.what() << std::endl;
  }
}

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);

  if (args.size() == 1) {
    std::cerr << "Please provide an argument.\n";
    return EXIT_FAILURE;
  }
  try {
    sql::Driver* driver = sql::mariadb::get_driver_instance();

    sql::SQLString url("jdbc:mariadb://localhost:3306/cppexample");

    sql::Properties properties({{"user", "cppexample"},
                                {"password", "qwLK56vb"},
                                {"localSocket", "/var/run/mysqld/mysqld.sock"}});

    std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));

    if (args[1] == "showTasks") {
      showTasks(conn);
    } else if (args[1] == "addTask") {
      if (args.size() != 3) {
        std::cerr << "Invalid arguments";
        return EXIT_FAILURE;
      }
      addTask(conn, args[2]);
    } else if (args[1] == "updateTaskStatus") {
      if (args.size() != 4) {
        std::cerr << "Invalid arguments";
        return EXIT_FAILURE;
      }
      updateTaskStatus(conn, std::stoi(args[2]), std::stoi(args[3]) != 0);
    } else if (args[1] == "deleteTask") {
      if (args.size() != 3) {
        std::cerr << "Invalid arguments";
        return EXIT_FAILURE;
      }
      deleteTask(conn, std::stoi(args[2]));
    } else {
      std::cerr << "Invalid command";
      return EXIT_FAILURE;
    }

    conn->close();
  } catch (sql::SQLException& e) {
    std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
