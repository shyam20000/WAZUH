#ifndef _CMD_KVDB_HPP
#define _CMD_KVDB_HPP

#include <string>

#include <CLI/CLI.hpp>
#include <base/utils/wazuhProtocol/wazuhProtocol.hpp>
#include <cmds/apiclnt/client.hpp>

namespace cmd::kvdb
{

const uint32_t DEFAULT_CLI_PAGE = 1;
const uint32_t DEFAULT_CLI_RECORDS = 50;

namespace details
{
constexpr auto ORIGIN_NAME = "engine_integrated_kvdb_api";

/* KVDB api command (endpoints) */
constexpr auto API_KVDB_CREATE_SUBCOMMAND {"create"};
constexpr auto API_KVDB_DELETE_SUBCOMMAND {"delete"};
constexpr auto API_KVDB_DUMP_SUBCOMMAND {"dump"};
constexpr auto API_KVDB_GET_SUBCOMMAND {"get"};
constexpr auto API_KVDB_INSERT_SUBCOMMAND {"insert"};
constexpr auto API_KVDB_LIST_SUBCOMMAND {"list"};
constexpr auto API_KVDB_REMOVE_SUBCOMMAND {"remove"};
constexpr auto API_KVDB_SEARCH_SUBCOMMAND {"search"};

} // namespace details

void runList(std::shared_ptr<apiclnt::Client> client, const std::string& kvdbName, bool loaded);
void runCreate(std::shared_ptr<apiclnt::Client> client,
               const std::string& kvdbName,
               const std::string& kvdbInputFilePath);
void runDump(std::shared_ptr<apiclnt::Client> client,
             const std::string& kvdbName,
             const uint32_t page,
             const uint32_t records);
void runDelete(std::shared_ptr<apiclnt::Client> client, const std::string& kvdbName);
void runGetKV(std::shared_ptr<apiclnt::Client> client, const std::string& kvdbName, const std::string& kvdbKey);
void runInsertKV(std::shared_ptr<apiclnt::Client> client,
                 const std::string& kvdbName,
                 const std::string& kvdbKey,
                 const std::string& kvdbValue);
void runRemoveKV(std::shared_ptr<apiclnt::Client> client, const std::string& kvdbName, const std::string& kvdbKey);
void runSearch(std::shared_ptr<apiclnt::Client> client, const std::string& kvdbName, const std::string& prefix);
void configure(const CLI::App_p& app);

} // namespace cmd::kvdb

#endif // _CMD_KVDB_HPP
