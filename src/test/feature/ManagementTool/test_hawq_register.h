#ifndef TEST_HAWQ_REGISTER_H
#define TEST_HAWQ_REGISTER_H

#include <string>
#include <pwd.h>
#include "lib/hdfs_config.h"
#include "gtest/gtest.h"

class TestHawqRegister : public ::testing::Test {
    public:
        TestHawqRegister() {
            std::string user = HAWQ_USER;
            if(user.empty()) {
                struct passwd *pw;
                uid_t uid = geteuid();
                pw = getpwuid(uid);
                user.assign(pw->pw_name);
            }
            conn.reset(new hawq::test::PSQL(HAWQ_DB, HAWQ_HOST, HAWQ_PORT, user, HAWQ_PASSWORD));
        }
        ~TestHawqRegister() {}

        std::string getHdfsLocation() {
            hawq::test::HdfsConfig hc;
            std::string namenodehost = "";
            EXPECT_EQ(true, hc.getNamenodeHost(namenodehost));
            return hawq::test::stringFormat("hdfs://%s", namenodehost.c_str());
        }

        std::string getDatabaseOid() {
            const hawq::test::PSQLQueryResult &result = conn->getQueryResult(
                hawq::test::stringFormat("SELECT oid from pg_database where datname = \'%s\';", HAWQ_DB));
            std::vector<std::vector<std::string>> table = result.getRows();
            if (table.size() > 0) {
                return table[0][0];
            }

            return "";
        }

        std::string getTableOid(std::string relname) {
            const hawq::test::PSQLQueryResult &result = conn->getQueryResult(
                hawq::test::stringFormat("SELECT oid from pg_class where relname = \'%s\';", relname.c_str()));
            std::vector<std::vector<std::string>> table = result.getRows();
            if (table.size() > 0) {
                return table[0][0];
            }

            return "";
        }

        void checkPgAOSegValue(std::string relname, std::string value, std::string fmt) {
            std::string reloid = getTableOid(relname);

            const hawq::test::PSQLQueryResult &result1 = conn->getQueryResult(
                hawq::test::stringFormat("SELECT tupcount from pg_aoseg.pg_%s_%s;", fmt.c_str(), reloid.c_str()));
            std::vector<std::vector<std::string>> table = result1.getRows();
            if (table.size() > 0) {
                for (int i = 0; i < table.size(); i++) {
                    EXPECT_EQ(table[i][0], value);
                }
            }

            const hawq::test::PSQLQueryResult &result2 = conn->getQueryResult(
                hawq::test::stringFormat("SELECT varblockcount from pg_aoseg.pg_%s_%s;", fmt.c_str(), reloid.c_str()));
            table = result2.getRows();
            if (table.size() > 0) {
                for (int i = 0; i < table.size(); i++) {
                    EXPECT_EQ(table[i][0], value);
                }
            }

            const hawq::test::PSQLQueryResult &result3 = conn->getQueryResult(
                hawq::test::stringFormat("SELECT eofuncompressed from pg_aoseg.pg_%s_%s;", fmt.c_str(), reloid.c_str()));
            table = result3.getRows();
            if (table.size() > 0) {
                for (int i = 0; i < table.size(); i++) {
                    EXPECT_EQ(table[i][0], value);
                }
            }
        }

    private:
        std::unique_ptr<hawq::test::PSQL> conn;
};

#endif
