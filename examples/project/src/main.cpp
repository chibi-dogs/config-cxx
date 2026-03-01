#include <config-cxx/config.h>
#include <filesystem>
#include <iostream>
#include <ranges>

// default initialization for Order struct might generally be a good idea to avoid uninitialized members.
// struct Order{
//     int id{};
//     std::string customer{};
//     double amount{};
//     bool priority{false};
// };
//
// bool checkIfFileIsValid(const std::filesystem::path& path){
//     return (path.has_filename() && exists(path) && is_regular_file(path));
//
// }
//
// std::expected<int, std::string> parseInt(std::string_view input)
// {
//     int result{};
//     auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), result);
//     if (ec != std::errc{} && ptr != input.data() + input.size())
//     {
//         return std::unexpected("Input could not be parsed into an int");
//     }
//     return result;
// }
//
// std::expected<double, std::string> parseDouble(std::string_view input)
// {
//     double result{};
//     auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), result, std::chars_format::general );
//     if (ec != std::errc{} && ptr != input.data() + input.size())
//     {
//         return std::unexpected("Input could not be parsed into double");
//     }
//     return result;
// }
//
// auto tokenize(std::string_view input)
// {
//     return input | std::views::split(',');
// }
//
// std::expected<void, std::string> parseLineIntoOrder(std::string_view inputLine, Order& order)
// {
//     auto tokens = tokenize(inputLine);
//     std::vector<std::string_view> orderFields;
//     for (auto&& token : tokens)
//     {
//         orderFields.emplace_back(token);
//     }
//     if (orderFields.size() != 4)
//     {
//         return std::unexpected("Data is malformed");
//     }
//     const auto id = parseInt(orderFields[0]);
//     if (!id)
//     {
//         return std::unexpected(id.error());
//     }
//     order.id = *id;
//     order.customer = std::string(orderFields[1]);
//     const auto amount = parseDouble(orderFields[2]);
//     if (!amount)
//     {
//         return std::unexpected(amount.error());
//     }
//     order.amount = *amount;
//     return {};
// }

// std::vector<Order> createOrders(std::istream& input)
// {
//     std::vector<Order> orders;
//     std::string line;
//
//     while (std::getline(input, line)) {
//         if (line.empty()) continue;
//         // parseLineIntoOrder(line, order)
//         try {
//             Order order;
//             std::string token;
//             // std::getline(line, token, ',');
//             // order.id = std::stoi(token);
//
//             std::getline(line, token, ',');
//             order.customer = token;
//
//             std::getline(line, token, ',');
//             order.amount = std::stod(token);
//
//             std::getline(line, token, ',');
//             order.priority = (token == "1");
//
//             orders.push_back(order);
//         } catch (...) {
//             std::cerr << "Warning: Input contains malformed line." << "\n";
//         }
//     }
//     return orders;
// }
int main()
{
    // auto order = Order{};
    // std::istringstream fake_input("1,Alice,22.3,0\nline2\n");
    // std::string line;
    // std::getline(fake_input, line);
    // auto parseResult = parseLineIntoOrder(line, order);
    config::Config config;

    const std::string dbHostKey = "db.host";
    const std::string dbPortKey = "db.port";
    const std::string awsAccountIdKey = "aws.accountId";
    const std::string awsAccountKeyKey = "aws.accountKey";
    const std::string awsRegionKey = "aws.region";
    const std::string authExpiresInKey = "auth.expiresIn";
    const std::string authEnabledKey = "auth.enabled";
    const std::string authRolesKey = "auth.roles";

    const auto dbHostValue = config.get<std::string>(dbHostKey);                    // "localhost"
    const auto dbPortValue = config.get<int>(dbPortKey);                            // 2000
    const auto awsAccountIdValue = config.get<std::string>(awsAccountIdKey);        // "0987654321"
    const auto awsAccountKeyValue = config.get<std::string>(awsAccountKeyKey);      // "321"
    const auto awsRegionValue = config.get<std::string>(awsRegionKey);              // "eu-central-2"
    const auto authExpiresInValue = config.get<int>(authExpiresInKey);              // 7200
    const auto authEnabledValue = config.get<bool>(authEnabledKey);                 // false
    const auto authRolesValue = config.get<std::vector<std::string>>(authRolesKey); // ["anonymous", "user"]

}
